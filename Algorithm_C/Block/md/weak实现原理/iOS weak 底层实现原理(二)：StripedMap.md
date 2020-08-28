#  iOS weak 底层实现原理(二)：StripedMap
## 摘要
首先 `StripedMap` 是一个模版类型：`template<typename T> class StripedMap`，阅读其内部实现然后从数据结构角度看的话，它其实是作为一个 `Key` 是 `void *`，`Value` 是 `T` 的 `hash` 表来用的。我们使用的 `SideTables` 对应类型是: `StripedMap<SideTable>`，阅读代码时我们能发现多处类似: `SideTable *table = &SideTables()[obj]` 这样的调用，它的作用正是根据对象的指针从 `SideTables` 这张 `hash` 表中找到 `obj` 所对应的 `SideTable`。
`StripedMap` 代码的实现并不复杂，下面一起来看下吧：
## StripedMap 源码
```c++
// StripedMap<T> is a map of void* -> T, 
// sized appropriately for cache-friendly lock striping. 
// StripedMap<T> 是一个 void * -> T 的 map，
// 即 Key 为 void *，value 是 T 的 hash 表。

// For example, this may be used as StripedMap<spinlock_t> 
// or as StripedMap<SomeStruct> where SomeStruct stores a spin lock.
// 例如，它可能用作 StripedMap<spinlock_t> 或者 StripedMap<SomeStruct>，
// 此时 SomeStruct 保存了一个 spin lock.

// 它的主要功能就是把自旋锁的锁操作从类中分离出来，
// 而且类中必须要有一个自旋锁属性。（？）
// StripMap 内部实质是一个开放寻址法生成哈希键值的哈希表.

enum { CacheLineSize = 64 };

template<typename T>
class StripedMap {
// 针对程序运行不同平台，定义下面 array 的长度
#if TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR
    enum { StripeCount = 8 };
#else
    enum { StripeCount = 64 };
#endif

    struct PaddedT {
        // CacheLineSize 值为定值 64
        // T value 64 字节对齐
        // 结构体只有一个 T 类型的 value 成员变量
        // 且这个 value 是遵循 64 字节内存对齐原则
        // 这个是保证每个 SideTable 64 个字节内存占用 
        T value alignas(CacheLineSize);
    };
    
    // 所有 PaddedT struct 类型数据被存储在 array 数组中
    // StripeCount == 8/64
    // 长度为 8/64 的 PaddedT 数组
    // 其实是 hash 表数据，正是我们的 8/64 张 SideTable 
    PaddedT array[StripeCount];
    
    // 该方法以 void * 作为 key 来
    // 获取 void * 对应在 StripedMap 的 array 中的位置
    // hash 函数
    static unsigned int indexForPointer(const void *p) {
        // typedef unsigned long uintptr_t;
        uintptr_t addr = reinterpret_cast<uintptr_t>(p);
        
        // addr 右移 4 位的值与 addr 右移 9 位的值做异或操作，
        // 然后对 StripeCount 取模 
        // 最后取模，防止 index 越界
        // 保证返回值在 [0, 7] 或 [0, 63] 区间内
        return ((addr >> 4) ^ (addr >> 9)) % StripeCount;
    }

 public:
    // 根据指针 p 得出 index，返回 array 数组 index 
    // 处的 PaddedT 的 value 成员变量
    // 操作符重载
    // 即可以使用 StripMap<xxx>[objcPtr] 访问 
    // 即可以使用 SideTables[p] 得到我们的对象所在 SideTable
    T& operator[] (const void *p) { 
        return array[indexForPointer(p)].value; 
    }
    
    // const_cast 该运算符用来修改类型的 const 或volatile属性。
    // 除了 const 或volatile 修饰之外， 
    // type_id 和 expression的类型是一样的。
    // 一、常量指针被转化成非常量的指针，并且仍然指向原来的对象；
    // 二、常量引用被转换成非常量的引用，并且仍然指向原来的对象；
    // 三、const_cast一般用于修改底指针。如const char *p形式。
    // -- 以上来自百度百科
    
    // 调用上面的 []，得到 T&
    // 然后转为 StripedMap<T>
    const T& operator[] (const void *p) const { 
        return const_cast<StripedMap<T>>(this)[p]; 
    }

    // Shortcuts for StripedMaps of locks.
    // StripedMaps 中锁的便捷操作：
    // 循环给 array 中的元素的 value 加锁
    void lockAll() {
        for (unsigned int i = 0; i < StripeCount; i++) {
            array[i].value.lock();
        }
    }
    
    // 循环给 array 中的元素的 value 解锁
    void unlockAll() {
        for (unsigned int i = 0; i < StripeCount; i++) {
            array[i].value.unlock();
        }
    }
    
    // 循环 array 中的元素的 value 执行 forceReset
    // 强制重置吗？
    void forceResetAll() {
        for (unsigned int i = 0; i < StripeCount; i++) {
            array[i].value.forceReset();
        }
    }
    
    // 对 array 中元素的 value 的 lock 定义锁定顺序？
    void defineLockOrder() {
        for (unsigned int i = 1; i < StripeCount; i++) {
            lockdebug_lock_precedes_lock(&array[i-1].value, &array[i].value);
        }
    }
    
    void precedeLock(const void *newlock) {
        // assumes defineLockOrder is also called
        // 假定 defineLockOrder 函数已经被调用过
        lockdebug_lock_precedes_lock(&array[StripeCount-1].value, newlock);
    }

    void succeedLock(const void *oldlock) {
        // assumes defineLockOrder is also called
        // 假定 defineLockOrder 函数已经被调用过
        lockdebug_lock_precedes_lock(oldlock, &array[0].value);
    }

    const void *getLock(int i) {
        if (i < StripeCount) return &array[i].value;
        else return nil;
    }
    
#if DEBUG
    StripedMap() {
        // Verify alignment expectations.
        // 验证是不是按 CacheLineSize（值为 64）个字节内存对齐的
        
        uintptr_t base = (uintptr_t)&array[0].value;
        uintptr_t delta = (uintptr_t)&array[1].value - base;
        ASSERT(delta % CacheLineSize == 0);
        ASSERT(base % CacheLineSize == 0);
    }
#else
    constexpr StripedMap() {}
#endif
};
```
## `reinterpret_cast`
延展: `reinterpret_cast` 是 `C++` 里的强制类型转换符。此处的用法是把指针转化为一个整数。
```c++
reinterpret_cast<new_type> (expression)
```
## `hash` 函数
`hash` 定位的算法，把 `void *` 指针转化为整数，然后右移 4 位和右移 9 位的值做异或操作，然后对 `StripedMap`(值为 8/64) 取模，防止 `index` 越界，保证返回值在 `[0, 7]/[0, 63]` 区间之间。
```c++
// 该方法以 void * 作为 key 来获取 void * 
// 对应在 StripedMap 的 array 中的索引
static unsigned int indexForPointer(const void *p) {
    // typedef unsigned long uintptr_t;
    // 把 void * 指针转化为 unsigned long
    uintptr_t addr = reinterpret_cast<uintptr_t>(p);
    
    // addr 右移 4 位的值与 addr 右移 9 位的值做异或操作，然后对 StripeCount 取模 
    return ((addr >> 4) ^ (addr >> 9)) % StripeCount; // 最后取余，防止 index 越界
}
```
## 数据封装 `PaddedT`
`StripedMap` 中的 `T` 类型数据作为结构体仅有的成员变量 `T value` 放在了 `struct PaddedT` 中，且指明 `value` 遵循 64 字节内存对齐，即为每张 `SideTable` 分配 64 字节内存。
```c++
struct PaddedT {
    // CacheLineSize 值为定值 64
    // T value 64 字节对齐
    // 表示结构体只有一个 T 类型的 value 成员变量
    // 且指明这个 value 是遵循 64 字节内存对齐原则
    T value alignas(CacheLineSize);
};
```
之所以再次封装数据到 `PaddedT` 中，是为了字节对齐，估计是存取 `hash` 值时的效率考虑。
接下来 `struct PaddedT` 被放在数组 `array` 中：
```c++
// 所有 struct PaddedT 类型数据被存储在 array 数组中
// TARGET_OS_IPHONE 设备且非模拟器的情况下 StripeCount == 8
// 长度为 8 的 PaddedT 数组
PaddedT array[StripeCount];
```
## `array` 中取数据（`SideTable`）
接下来是从 `array` 数组公共的取值方法，（此处是很奇怪的，只是定义了取值函数，并没有定义构造函数。）主要调用 `indexForPointer` 函数，使得外部传入的 **对象地址指针** 通过 `hash` 函数得到其在 `array` 数组中的索引，然后返回该索引元素的 `value` 值，即返回对应的 `SideTable`：
```c++
// 根据指针 p 得出 index，返回 array 数组 index 
// 处的 PaddedT 的 value 成员变量
T& operator[] (const void *p) { 
    return array[indexForPointer(p)].value; 
}
const T& operator[] (const void *p) const { 
    return const_cast<StripedMap<T>>(this)[p]; 
}
```
## `Lock` 操作
接下来是一系列锁的操作，循环加锁、循环解锁、定义锁定顺序等等，由于 `SideTables` 是一个全局的 `hash` 表，因此必须要带锁访问。通过源码看到所有的 `StripedMap` 锁操作，最终调用的都是 `array[i].value` 的相关操作，而 `value` 是模版的抽象数据 `T` 类型，因此 `T` 必须具备相关的 `lock` 操作接口。在讲 《iOS weak 底层实现原理(四)：SideTables和SideTable》一节中我们都能看到对应的实现。

因此，要用 `StripedMap` 作为模版 `hash` 表，对于 `T` 类型是有要求的，而在 `SideTables` 中 `T` 即为 `SideTable` 类型，等下我们会看 `SideTable` 是怎么符合 `StripedMap` 的数据类型要求的。

## 结尾
分析完 `StripedMap` 就分析完了 `SideTables` 这个全局的大 `hash` 表，下面我们继续来分析 `SideTables` 中存储的数据: `SideTable`。

## 参考链接
**参考链接:🔗**
+ [Object Runtime -- Weak](https://cloud.tencent.com/developer/article/1408976)
+ [OC Runtime之Weak(2)---weak_entry_t](https://www.jianshu.com/p/045294e1f062)
+ [iOS 关联对象 - DisguisedPtr](https://www.jianshu.com/p/cce56659791b)
+ [Objective-C运行时-动态特性](https://zhuanlan.zhihu.com/p/59624358)
+ [Objective-C runtime机制(7)——SideTables, SideTable, weak_table, weak_entry_t](https://blog.csdn.net/u013378438/article/details/82790332)
