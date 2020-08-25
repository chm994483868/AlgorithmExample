#  iOS weak_table_t

## `weak_table_t`

```
/*
The weak table is a hash table governed by a single spin lock.
weak table 是由单个 spin lock 控制的哈希表。

An allocated blob of memory, most often an object, but under GC any such allocation, may have its address stored in a __weak marked storage location through use of compiler generated write-barriers or hand coded uses of the register weak primitive. 

Associated with the registration can be a callback block for the case when one of the allocated chunks of memory is reclaimed. 
The table is hashed on the address of the allocated memory.  
When __weak marked memory changes its reference, we count on the fact that we can still see its previous reference.

So, in the hash table, indexed by the weakly referenced item, is a list of all locations where this address is currently being stored.

For ARC, we also keep track of whether an arbitrary object is being deallocated by briefly placing it in the table just prior to invoking dealloc, and removing it via objc_clear_deallocating just prior to memory reclamation.
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
    weak_entry_t *weak_entries; // weak_entry_t 指针
    size_t    num_entries; // 目前保存的 weak_entry_t 的数量
    
    uintptr_t mask; // weak_table_t 的总容量
    uintptr_t max_hash_displacement; // 记录所有项的最大偏移量
    // 因为会有 hash 碰撞的情况，而 weak_table_t 采用了开放寻址法来解决，
    // 所以某个 entry 实际存储的位置并不一定是 hash 函数计算出来的位置
};

/// Adds an (object, weak pointer) pair to the weak table.
/// 添加一对（object, weak pointer）到弱引用表里
id weak_register_no_lock(weak_table_t *weak_table, id referent, 
                         id *referrer, bool crashIfDeallocating);

/// Removes an (object, weak pointer) pair from the weak table.
/// 从弱引用表里移除一对（object, weak pointer）
void weak_unregister_no_lock(weak_table_t *weak_table, id referent, id *referrer);

#if DEBUG
/// Returns true if an object is weakly referenced somewhere.
/// 如果一个对象在弱引用表的到某处，即该对象被保存在弱引用表里，则返回 true.
bool weak_is_registered_no_lock(weak_table_t *weak_table, id referent);
#endif

/// Called on object destruction. Sets all remaining weak pointers to nil.
/// 当对象销毁的时候该函数被调用。设置所有剩余的 __weak 指针为 nil.
/// 此处正对应了，__weak 变量在它指向的对象销毁后它会被置为 nil 的机制
void weak_clear_no_lock(weak_table_t *weak_table, id referent);
```
**参考链接:🔗**
[Object Runtime -- Weak](https://cloud.tencent.com/developer/article/1408976)
[OC Runtime之Weak(2)---weak_entry_t](https://www.jianshu.com/p/045294e1f062)
[iOS 关联对象 - DisguisedPtr](https://www.jianshu.com/p/cce56659791b)
[Objective-C运行时-动态特性](https://zhuanlan.zhihu.com/p/59624358)
[Objective-C runtime机制(7)——SideTables, SideTable, weak_table, weak_entry_t](https://blog.csdn.net/u013378438/article/details/82790332)
