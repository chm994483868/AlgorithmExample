# iOS weak 底层实现原理(五)：weak_table_t

> 定义位于 `Project Headers/objc-weak.h` Line 119。

## 简介
`weak_table_t weak_table` 是用来存储对象弱引用的相关信息。
  通过上节我们知道了 `SideTables` 中 `PaddedT array[StripeCount]`，而 `StripeCount` 只能是 8/64，即我们的 `SideTable` 表只能有 8/64 张，而在我们的程序中，绝对不止 8/64 个对象，因此多个对象一定会重用一个 `SideTable`，也就是说一个 `weak_table` 会存储多个对象的弱引用信息。
  因此在一个`SideTable` 中，又会通过 `weak_table` 作为 `hash` 表再次分散存储多个对象的弱引用信息。

## `weak_table_t` 定义
`weak_table_t` 定义如下:
```c++
/*
The weak table is a hash table governed by a single spin lock.
weak table 是由单个 spin lock 控制的哈希表。

An allocated blob of memory, most often an object, but under GC any
such allocation, may have its address stored in a __weak marked storage
location through use of compiler generated write-barriers or hand coded
uses of the register weak primitive. 

Associated with the registration can be a callback block for
the case when one of the allocated chunks of memory is reclaimed. 
The table is hashed on the address of the allocated memory.  
When __weak marked memory changes its reference, we count on
the fact that we can still see its previous reference.

So, in the hash table, indexed by the weakly referenced item,
is a list of all locations where this address is currently being stored.

For ARC, we also keep track of whether an arbitrary object is
being deallocated by briefly placing it in the table just prior to
invoking dealloc, and removing it via objc_clear_deallocating just prior to memory reclamation.
*/
```
```c++
/**
 * The global weak references table. 
 * 全局的弱引用表（哈希表）
 *
 * Stores object ids as keys,
 * 以对象的 ids 作为 key,
 *
 * and weak_entry_t structs as their values.
 * 以 struct weak_entry_t 作为 Value.
 *
 */
struct weak_table_t {
    // weak_entry_t 指针，作为 hash 数组，用来存储弱引用对象的相关信息 weak_entry_t
    weak_entry_t *weak_entries;
    // 目前保存的 weak_entry_t 的数量，hsah 数组中的元素个数
    size_t    num_entries;
    
    // 值是 hash 数组总长度减 1，会参与 hash 函数计算
    // weak_table_t 的总容量
    uintptr_t mask;
    
    // 记录所有项的最大偏移量，即发生 hash 冲突的最大次数
    // 用于判断是否出现了逻辑错误，hash 表中的冲突次数绝对不会超过这个值，
    // 下面关于 entry 的操作函数中会看到这个成员变量的使用，这里先对它有一些了解即可
    // 因为会有 hash 碰撞的情况，而 weak_table_t 采用了开放寻址法来解决，
    // 所以某个 entry 实际存储的位置并不一定是 hash 函数计算出来的位置
    
    uintptr_t max_hash_displacement;
};
```
`weak_table_t` 是一个典型的 `hash` 结构。其中 `weak_entry_t *weak_entries;` 是一个动态数组，用来存储 `weak_table_t` 的数据元素 `weak_entry_t`。而剩下的三个元素则用于 `hash` 表的相关操作。

## `weak_entry_for_referent` 函数定位 `weak_entry_t`
下面拓展一下，找到 `Source/objc-weak.mm` 文件，我们来看一下 `weak_table` 的 `hash` 定位操作 `weak_entry_for_referent` 函数： 
```c++
/** 
 * Return the weak reference table entry for the given referent. 
 * 返回给定的 referent 在弱引用表中的 entry. 
 * 即从 weak_table_t 的 weak_entries 数组中返回对应的 weak_entry_t
 * If there is no entry for referent, return NULL. 
 * 如果不存在则返回 NULL
 * Performs a lookup.
 *
 * @param weak_table 入参是 weak_table，通过 "weak_able = &SideTables()[referent];"
 * 从全局的 SideTables 中找到 referent 所处的 weak_table_t
 * @param referent The object. Must not be nil.
 * 
 * @return The table of weak referrers to this object. 
 */
static weak_entry_t *
weak_entry_for_referent(weak_table_t *weak_table, objc_object *referent)
{
    // 如果 referent 为空则执行断言
    ASSERT(referent);
    
    // 取得 hash 数组的入口
    weak_entry_t *weak_entries = weak_table->weak_entries;
    
    // 判空
    if (!weak_entries) return nil;
    
    // hash 函数：hash_pointer 函数返回值与 mask 做与操作，防止 index 越界
    // 这里的与操作很巧妙，下面会进行详细分析
    size_t begin = hash_pointer(referent) & weak_table->mask;
    size_t index = begin;
    size_t hash_displacement = 0;
    while (weak_table->weak_entries[index].referent != referent) {
        index = (index+1) & weak_table->mask;
        
        // 触发 bad_weak_table
        if (index == begin) bad_weak_table(weak_table->weak_entries);
        hash_displacement++;
        
        // 当 hash 冲突超过了 weak_table 的 max_hash_displacement 时，
        // 说明 referent 在 hash 表中没有对应的 weak_entry_t，返回 nil
        if (hash_displacement > weak_table->max_hash_displacement) {
            return nil;
        }
    }

    return &weak_table->weak_entries[index];
}
```
### `hash_pointer`_`hash` 函数
```c++
// hash 函数，与 mask 做与操作，防止 index 越界
size_t begin = hash_pointer(referent) & weak_table->mask;
```
 `hash_pointer` 尝试确定 `referent` 的初始位置，后面的 `& weak_table->mask;` 位操作来确保得到的 `begin` 不会越界，同我们使用的取模操作是一样的功能，只是改为了位操作，提升了效率。
 
 #### `mask` & 操作确保 `index` 不越界
> 这里的与运算其实很巧妙，首先是 `mask` 的值一直是 2 的 N 次方减 1 ，根据 `weak_grow_maybe` 函数，我们会看到 `hash` 数组（`weak_entry_t *weak_entries;`）长度最小是 64，即 2 的 6 次方（N >= 6），以后的每次扩容是之前的容量乘以 2，即总容量永远是 2 的 N 次方，然后 `mask` 是 2 的 N 次方减 1，转为二进制的话：`mask`一直是: `0x0111111(64 - 1，N = 6)`、`0x01111111(128 -1，N = 7)....`, 即 `mask` 的二进制表示中后 N 位总是 1，之前的位总是 0，所以任何数与 `mask` 做与操作的结果总是在 [0, mask] 这个区间内，例如任何数与 `0x0111111(64 - 1，N = 6)` 做与操作的话结果总是在 `[0, 63]` 这个区间内。而这个正是 `weak_entry_t *weak_entries` 数组的合理下标范围。

然后，从 `begin` 开始对比 `hash` 表中的数据是否与 `referent` 相等，如果不相等则 `index + 1`，直到 `index == begin`（绕了一圈）或超过了可能的**hash 冲突最大值**。
以上就是 `weak_table_t` 如何进行 `hash` 定位的相关操作。
这里看一下 `hash` 函数 `hash_pointer(referent)`:
```c++
/** 
 * Unique hash function for object pointers only.
 * 
 * @param key The object pointer
 * 
 * @return Size unrestricted hash of pointer.
 */
static inline uintptr_t hash_pointer(objc_object *key) {
    // typedef unsigned long uintptr_t;
    return ptr_hash((uintptr_t)key);
}

// 连续的函数调用
#if __LP64__
static inline uint32_t ptr_hash(uint64_t key)
{
    key ^= key >> 4; // key 右移 4 位，然后与原始 key 做异或位操作
    
    key *= 0x8a970be7488fda55; // 定值 与 key 做乘运算
    
    key ^= __builtin_bswap64(key); // 翻转64位数各字节
    
    return (uint32_t)key;
}
#else
static inline uint32_t ptr_hash(uint32_t key)
{
    key ^= key >> 4;
    key *= 0x5052acdb;
    key ^= __builtin_bswap32(key);
    
    return key;
}
#endif
```
## 添加、移除 `referrer` 到 `weak_entry_t` 及 `weak` 变量置 `nil` 函数定义
`weak_table_t` 下面是三个函数定义，这里我们只要看下它们的作用就好，具体实现过程参考 `weak` 原理那篇。

### `weak_register_no_lock` 
```
/// Adds an (object, weak pointer) pair to the weak table.
/// 添加一对（object, weak pointer）到弱引用表里
id weak_register_no_lock(weak_table_t *weak_table, id referent, 
                         id *referrer, bool crashIfDeallocating);
```
### `weak_unregister_no_lock`
```
/// Removes an (object, weak pointer) pair from the weak table.
/// 从弱引用表里移除一对（object, weak pointer）
void weak_unregister_no_lock(weak_table_t *weak_table, id referent, id *referrer);
```
### `weak_is_registered_no_lock`
```
#if DEBUG
/// Returns true if an object is weakly referenced somewhere.
/// 如果一个对象在弱引用表的到某处，即该对象被保存在弱引用表里，则返回 true.
bool weak_is_registered_no_lock(weak_table_t *weak_table, id referent);
#endif
```
### `weak_clear_no_lock`
```
/// Called on object destruction. Sets all remaining weak pointers to nil.
/// 当对象销毁的时候该函数被调用。设置所有剩余的 __weak 指针为 nil.
/// 此处正对应了，__weak 变量在它指向的对象销毁后它会被置为 nil 的机制
void weak_clear_no_lock(weak_table_t *weak_table, id referent);
```
## `weak_table_t` 调整大小
`weak_table_t` 调用 `weak_grow_maybe` 和 `weak_compact_maybe` 这两个函数，用来在 `weak_table_t` 过满或者过空的情况下及时的调整其大小，优化内存的使用效率，并提高效率。
这两个函数都通过调用 `weak_resize` 函数来调整 `weak_table_t` 的大小。

### `weak_grow_maybe`
此函数会在创建 `weak_entry_t` 和把 `new_entry` 添加到 `weak_table_t` 之间调用，下面看下它的实现: 
```c++
// Grow the given zone's table of weak references if it is full.
// 如果给定区域的弱引用表已满，则对其进行扩展。
static void weak_grow_maybe(weak_table_t *weak_table)
{
    // #define TABLE_SIZE(entry) (entry->mask ? entry->mask + 1 : 0)
    // mask + 1 表示当前 weak_table 总容量
    // old_size = mask + 1;
    size_t old_size = TABLE_SIZE(weak_table);

    // Grow if at least 3/4 full.
    // 如果目前存储的 entry 数量超过了总容量的 3/4，则进行扩容
    if (weak_table->num_entries >= old_size * 3 / 4) {
        // 如果是 weak_table 是空的，则初始长度为 64
        // 如果是非空，则扩容为之前的两倍
        weak_resize(weak_table, old_size ? old_size*2 : 64);
    }
}
```
该函数用于扩充 `weak_table_t` 的 `weak_entry_t *weak_entries` 的空间，扩充条件是 `num_entries` 超过了 `mask + 1` 的 3/4。看到 `weak_entries` 的初始化长度是 `64`，每次扩充的长度则是 `mask + 1` 的 2 倍，扩容完毕后会把原 `weak_entry_t` 重新通过 `hash` 函数计算索引插入到新空间内，并更新 `weak_tabl_t` 各成员变量。占据的内存空间的总容量则是: `(mask + 1) * sizeof(weak_entry_t)`。
综上 `mask + 1` 总是 2 的 `N` 次方。（`N >= 6`）

### `weak_compact_maybe`
此函数会在 `weak_entry_remove` 函数中调用，旨在 `weak_entry_t` 从 `weak_table_t` 中移除后，缩小 `weak_entry_t *weak_entries` 的空间，下面看下它的实现：
```c++
// Shrink the table if it is mostly empty.
// 即当 weak_table_t 的 `weak_entry_t *weak_entries;` 数组大部分为空的情况下，缩小 weak_entries
static void weak_compact_maybe(weak_table_t *weak_table)
{
    // #define TABLE_SIZE(entry) (entry->mask ? entry->mask + 1 : 0)
    // old_size = mask + 1;
    size_t old_size = TABLE_SIZE(weak_table);

    // Shrink if larger than 1024 buckets and at most 1/16 full.
    // old_size 超过了 1024 并且 低于 1/16 的空间占用则进行缩小
    
    if (old_size >= 1024  && old_size / 16 >= weak_table->num_entries) {
        // 缩小容量为 ols_size 的 1/8
        weak_resize(weak_table, old_size / 8);
        
        // 缩小为 1/8 和上面的空间占用少于 1/16，两个条件合并在一起，保证缩小后的容量占用少于 1/2
        // leaves new table no more than 1/2 full
    }
}
```
缩小 `weak_entry_t *weak_entries` 容量的条件是目前的总长度**超过了 1024** 并且容量**占用小于 1/16**，缩小后的空间为当前空间的 **1/8**。

### `weak_resize`
下面看一下扩大和缩小空间都会调用的 `weak_resize` 函数，下面是它的函数定义:
```c++
static void weak_resize(weak_table_t *weak_table, size_t new_size)
{
    // old_size = mask + 1; 表示原容量
    size_t old_size = TABLE_SIZE(weak_table);
    
    // 旧的 weak_entries 数组起始地址
    weak_entry_t *old_entries = weak_table->weak_entries;
    
    // 新的 weak_entries 数组起始地址
    // 内存空间总容量为: new_size * sizeof(weak_entry_t)
    weak_entry_t *new_entries = (weak_entry_t *)
        calloc(new_size, sizeof(weak_entry_t));
        
    // mask 仍是总容量减 1
    weak_table->mask = new_size - 1;
    // 更新 hash 数组起始地址
    weak_table->weak_entries = new_entries;
    
    // 以下两个成员变量会在下面的 weak_entry_insert 函数中得到更新
    // hash 冲突偏移
    weak_table->max_hash_displacement = 0;
    // 当前容量占用
    weak_table->num_entries = 0;  // restored by weak_entry_insert below
    
    // 如果有旧 weak_entry_t 需要放到新空间内 
    if (old_entries) {
        weak_entry_t *entry;
        weak_entry_t *end = old_entries + old_size;
        // 循环调用 weak_entry_insert 把 weak_entry_t 插入到新空间内
        for (entry = old_entries; entry < end; entry++) {
            if (entry->referent) {
                weak_entry_insert(weak_table, entry);
            }
        }
        
        // 最后释放旧空间
        free(old_entries);
    }
}
```
### `weak_entry_insert`
```c++
/** 
 * Add new_entry to the object's table of weak references.
 * 添加 new_entry 到对象的弱引用表中。
 * Does not check whether the referent is already in the table.
 * 不检查引用对象是否已在表中。
 */
static void weak_entry_insert(weak_table_t *weak_table, weak_entry_t *new_entry)
{
    weak_entry_t *weak_entries = weak_table->weak_entries;
    ASSERT(weak_entries != nil);
    
    // 调用 hash 函数找到 new_entry 在 weak_table_t 的 hash 数组中的位置
    // 可能会发生 hash 冲突
    size_t begin = hash_pointer(new_entry->referent) & (weak_table->mask);
    size_t index = begin;
    size_t hash_displacement = 0;
    while (weak_entries[index].referent != nil) {
        index = (index+1) & weak_table->mask;
        if (index == begin) bad_weak_table(weak_entries);
        hash_displacement++;
    }

    weak_entries[index] = *new_entry;
    weak_table->num_entries++;
    
    // 此步操作正记录了 weak_table_t 的最大偏移值
    if (hash_displacement > weak_table->max_hash_displacement) {
        weak_table->max_hash_displacement = hash_displacement;
    }
}
```
综合 `weak_entry_insert` 函数可知  `weak_resize` 函数的整体作用，该函数具体执行了 `hash` 数组的扩大和缩小，首先根据`new_size`申请相应大小的内存，`new_entries`指针指向这块新申请的内存。设置`weak_table`的`mask`为`new_size - 1`。此处`mask`的作用是记录`weak_table`实际占用的内存边界，此外`mask`还用在 `hash` 函数中保证不会造成 `hash` 数组越界。
`HashTable`可能会有`hash碰撞`，而`weak_table_t`使用了**开放寻址法**来处理碰撞。如果发生碰撞的话，将寻找相邻（如果已经到最尾端的话，则从头开始）的下一个空位。`max_hash_displacement`记录当前`weak_table`最大的偏移值，即`hash`函数计算的位置和实际存储位置的最大偏差。此值会在其他地方用到，例如：`weak_entry_for_referent`函数，寻找给定的 `referent` 的在弱引用表中的 `entry`时，如果在循环过程中 `hash_displacement` 的值超过了 `weak_table->max_hash_displacement` 则表示，不存在要找的 `weak_entry_t`。

## 尾声
其他后续的与 `weak_table_t` 相关的函数放在 `weak` 原理那篇。

## 参考链接
**参考链接:🔗**
+ [Object Runtime -- Weak](https://cloud.tencent.com/developer/article/1408976)
+ [OC Runtime之Weak(1)---weak_table_t](https://www.jianshu.com/p/751265a03324)
+ [iOS 关联对象 - DisguisedPtr](https://www.jianshu.com/p/cce56659791b)
+ [Objective-C运行时-动态特性](https://zhuanlan.zhihu.com/p/59624358)
+ [Objective-C runtime机制(7)——SideTables, SideTable, weak_table, weak_entry_t](https://blog.csdn.net/u013378438/article/details/82790332)
+ [Objective](https://www.dazhuanlan.com/2019/10/27/5db5750952155/)
+ [Objective-C 引用计数原理](http://yulingtianxia.com/blog/2015/12/06/The-Principle-of-Refenrence-Counting/)
