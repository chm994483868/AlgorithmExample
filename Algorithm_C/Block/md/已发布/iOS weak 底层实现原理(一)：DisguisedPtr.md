# iOS weak 底层实现原理(一)：DisguisedPtr

> 为了全面透彻的理解 `weak` 关键字的工作原理，现在从最底层的数据结构开始挖掘，力求构建一个完整的认知体系。

## template <typename T> class DisguisedPtr
&emsp;`template <typename T> class DisguisedPtr` 是在 `Project Headers/objc-private.h` 中定义的一个模版工具类，主要的功能是把 `T` 指针指向的地址直接转化为一个 `unsigned long`，实现**指针到整数的相互映射**，起到把**指针伪装**起来的作用，使指针隐藏于系统工具（如 `leaks` 工具）。在 `objc4-781` 全局搜索 `DisguisedPtr` 发现 `T` 仅作为 `objc_object` 类型使用。
 
> &emsp;DisguisedPtr<T> acts like pointer type T*, except the stored value is disguised to hide it from tools like `leaks`. nil is disguised as itself so zero-filled memory works as expected, which means 0x80..00 is also disguised as itself but we don't care. Note that weak_entry_t knows about this encoding.
   
> &emsp;DisguisedPtr 的作用类似于指针类型 T *，只是存储的值被伪装成对诸如 “leaks” 之类的工具隐藏。nil 本身是伪装的，因此 0 值的内存可以按预期工作，让  nil 指针像 non-nil 指针那样正常运行它的操作，而不会让程序崩溃。这意味着 0x80..00 本身也伪装了，但我们不在乎。请注意，weak_entry_t 知道此编码。

```c++
template <typename T>
class DisguisedPtr {
    // unsigned long 类型的 value 足够保存转化为整数的内存地址
    uintptr_t value;

    static uintptr_t disguise(T* ptr) {
        // 相当于直接把 T 指针指向的地址转化为 unsigned long 并取负值
        return -(uintptr_t)ptr;
    }

    static T* undisguise(uintptr_t val) {
        // 把 unsigned long 类型的 val 转为指针地址，对应上面的 disguise 函数
        return (T*)-val;
    }

 public:
    DisguisedPtr() { } // 构造函数
    
    // 初始化列表 ptr 初始化 value 成员变量
    DisguisedPtr(T* ptr) : value(disguise(ptr)) { }
    
    // 复制构造函数
    DisguisedPtr(const DisguisedPtr<T>& ptr) : value(ptr.value) { }

    // 重载操作符：
    // T* 赋值函数，把一个 T 指针赋值给 DisguisedPtr<T> 类型变量时，直接发生地址到整数的转化
    DisguisedPtr<T>& operator = (T* rhs) {
        value = disguise(rhs);
        return *this;
    }
    
    // DisguisedPtr<T>& 引用赋值函数
    DisguisedPtr<T>& operator = (const DisguisedPtr<T>& rhs) {
        value = rhs.value;
        return *this;
    }

    // ()
    operator T* () const {
        // unsigned long value 转回指针
        return undisguise(value);
    }
    
    // ->
    T* operator -> () const { 
        // unsigned long value 转回指针
        return undisguise(value);
    }
    
    // ()
    T& operator * () const { 
        // 转化为指针并取出该指针指向的内容
        return *undisguise(value);
    }
    
    // []
    T& operator [] (size_t i) const {
        // unsigned long value 转回指针，再找到指定下标 i 位置的值
        return undisguise(value)[i];
    }

    // pointer arithmetic operators omitted 
    // because we don't currently use them anywhere
    // 省略的指针算术运算符，因为目前我们不在任何地方使用它。
};

// fixme type id is weird and not identical to objc_object*
// fixme id 类型很奇怪，与 objc_object * 不同（id ? => typedef struct objc_object *id）
// ==
static inline bool operator == (DisguisedPtr<objc_object> lhs, id rhs) {
    return lhs == (objc_object *)rhs;
}
// !=
static inline bool operator != (DisguisedPtr<objc_object> lhs, id rhs) {
    return lhs != (objc_object *)rhs;
}
```
## template <typename T> class StripedMap

> &emsp;StripedMap<T> is a map of void* -> T, sized appropriately for cache-friendly lock striping. For example, this may be used as StripedMap<spinlock_t> or as StripedMap<SomeStruct> where SomeStruct stores a spin lock.

> &emsp;StripedMap 是 void *-> T 的映射，其大小适合于 **缓存友好** 的 lock striping。例如，它可用作 StripedMap<spinlock_t> 或 StripedMap<SomeStruct>，其中 SomeStruct 存储 spin lock。**cache-friendly:** 那么按照高速缓存的工作原理，可以发现局部性良好的程序，缓存命中的概率更高，从这个意义上来讲，程序也会更快。我们称这样的程序，是高速缓存友好（cache-friendly）的程序。

&emsp;`template<typename T> class StripedMap` 从数据结构角度看的话，它是作为一个 `Key` 是 `void *` `Value` 是 `T` 的 `hash` 表来用的。在 `objc4-781` 中全局搜索 `StripedMap` 发现 `T` 作为 `SideTable` 和 `spinlock_t` 类型使用。
&emsp;`SideTables` 的类型正是 `StripedMap<SideTable>`。`SideTables` 的使用：`SideTable *table = &SideTables()[obj]` 它的作用正是根据 `objc_object` 的指针计算出哈希值，然后从 `SideTables` 这张全局哈希表中找到 `obj` 所对应的 `SideTable`。
&emsp;`StripedMap<spinlock_t> PropertyLocks`：当使用 `atomic` 属性时，`objc_getProperty` 时会从通过 `PropertyLocks[slot]` 获得一把锁并夹锁保证 `id value = objc_retain(*slot)` 线程安全。`StripedMap<spinlock_t> StructLocks`：用于提供锁保证 `objc_copyStruct` 函数调用时 `atomic` 参数为 `true` 时的线程安全。`StripedMap<spinlock_t> CppObjectLocks`：保证 `objc_copyCppObjectAtomic` 函数调用时的线程安全。
&emsp;根据下面的源码实现 `Lock` 的部分，发现抽象类型 `T` 必须支持 `lock`、`unlock`、`forceReset`、`lockdebug_lock_precedes_lock` 函数接口。已知 `struct SideTable` 都有提供。

```c++
enum { CacheLineSize = 64 };

template<typename T>
class StripedMap {

#if TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR
    enum { StripeCount = 8 }; // iPhone，同时也表明了 SideTables 中只有 8 张 SideTable
#else
    enum { StripeCount = 64 }; // mac/simulators，有 64 张 SideTable
#endif

    struct PaddedT {
        // CacheLineSize 值为定值 64
        // T value 64 字节对齐，（表示一张表至少 64 个字节吗？）
        T value alignas(CacheLineSize);
    };
    
    // 长度是 8/64 的 PaddedT 数组
    PaddedT array[StripeCount];
    
    // hash 函数
    static unsigned int indexForPointer(const void *p) {
        // 把 p 指针强转为 unsigned long
        uintptr_t addr = reinterpret_cast<uintptr_t>(p);
        
        // addr 右移 4 位的值与 addr 右移 9 位的值做异或操作，
        // 然后对 StripeCount 取模，防止越界。
        return ((addr >> 4) ^ (addr >> 9)) % StripeCount;
    }

 public:
    // hash 取值
    T& operator[] (const void *p) { 
        return array[indexForPointer(p)].value; 
    }
    
    // 原型：const_cast<type_id> (expression)
    // const_cast 该运算符用来修改类型的 const 或 volatile 属性。
    // 除了 const 或 volatile 修饰之外，type_id 和 expression的类型是一样的。
    // 即把一个不可变类型转化为可变类型（const int b => int b1）
    // 1. 常量指针被转化成非常量的指针，并且仍然指向原来的对象；
    // 2. 常量引用被转换成非常量的引用，并且仍然指向原来的对象；
    // 3. const_cast一般用于修改底指针。如const char *p形式。
    
    // 调用上面的 []，得到 T&
    // 然后转为 StripedMap<T>
    const T& operator[] (const void *p) const {
        // 这里 const_cast<StripedMap<T>>(this) 有必要吗，本来就是读取值，并不会修改 StripedMap 的内容鸭
        return const_cast<StripedMap<T>>(this)[p]; 
    }

    // Shortcuts for StripedMaps of locks.
    
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

**参考链接:🔗**
+ [使用intptr_t和uintptr_t](https://www.jianshu.com/p/03b7d56bf80f)
+ [Object Runtime -- Weak](https://cloud.tencent.com/developer/article/1408976)
+ [OC Runtime之Weak(2)---weak_entry_t](https://www.jianshu.com/p/045294e1f062)
+ [iOS 关联对象 - DisguisedPtr](https://www.jianshu.com/p/cce56659791b)
+ [Objective-C运行时-动态特性](https://zhuanlan.zhihu.com/p/59624358)
+ [Objective-C runtime机制(7)——SideTables, SideTable, weak_table, weak_entry_t](https://blog.csdn.net/u013378438/article/details/82790332)
