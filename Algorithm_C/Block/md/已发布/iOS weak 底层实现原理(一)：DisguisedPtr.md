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

&emsp;`SideTables` 类型：`StripedMap<SideTable>`。`SideTables` 的使用：`SideTable *table = &SideTables()[obj]` 它的作用正是根据 `objc_object` 的指针计算出哈希值，然后从 `SideTables` 这张全局哈希表中找到 `obj` 所对应的 `SideTable`。

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
        // reinterpret_cast<new_type> (expression) C++ 里的强制类型转换符
        uintptr_t addr = reinterpret_cast<uintptr_t>(p);
        
        // addr 右移 4 位的值与 addr 右移 9 位的值做异或操作，
        // 然后对 StripeCount 取模，防止越界
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
    
    // 把 this 转化为 StripedMap<T>，然后调用上面的 []，得到 T&
    const T& operator[] (const void *p) const {
        // 这里 const_cast<StripedMap<T>>(this) 有必要吗，本来就是读取值，并不会修改 StripedMap 的内容鸭
        return const_cast<StripedMap<T>>(this)[p]; 
    }

    // Shortcuts for StripedMaps of locks.
    
    // 循环给 array 中的元素的 value 加锁
    // 以 iPhone 下 SideTables 为例的话，循环对 8 张 SideTable 加锁，
    // struct SideTable 成员变量: spinlock_t slock，lock 函数实现是： void lock() { slock.lock(); }
    void lockAll() {
        for (unsigned int i = 0; i < StripeCount; i++) {
            array[i].value.lock();
        }
    }
    
    // 同上，解锁
    void unlockAll() {
        for (unsigned int i = 0; i < StripeCount; i++) {
            array[i].value.unlock();
        }
    }
    
    // 同上，重置 Lock
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

    // T 是 spinlock_t 时，根据指定下标从 StripedMap<spinlock_t> -> array 中取得 spinlock_t  
    const void *getLock(int i) {
        if (i < StripeCount) return &array[i].value;
        else return nil;
    }
    
    // 构造函数，在 DEBUG 模式下会验证 T 是否是 64 内存对齐的
#if DEBUG
    StripedMap() {
        // Verify alignment expectations.
        // 验证 value（T）是不是按 CacheLineSize（值为 64）内存对齐的
        
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
## 
> 定义位于: `Project Headers/objc-weak.h` Line 80，此文件只有 144 行，基本所有的内容都是围绕 `struct weak_entry_t` 和 `struct weak_table_t`。

## `weak_referrer_t`
`weak_referrer_t` 定义，可以看到它是一个 `DisguisedPtr` 模版类，且它的 T 是 `objc_object *`：
```c++
// The address of a __weak variable.
// __weak 变量的地址（指针的指针）

// These pointers are stored disguised so memory analysis tools 
// don't see lots of interior pointers from the weak table into objects.
// 这些指针是伪装的，因此内存分析工具看不到从 weak table 到对象的大量内部指针。
// 这里 T 是 objc_object *，那么 DisguisedPtr 里的 T* 就是 objc_object**，即为指针的指针

typedef DisguisedPtr<objc_object *> weak_referrer_t;
```

## `PTR_MINUS_2`
`PTR_MINUS_2` 宏定义，用于标记 `num_refs` 位域长度。
```c++
#if __LP64__
#define PTR_MINUS_2 62 // 当前是 __LP64__
#else
#define PTR_MINUS_2 30
#endif
```

## `WEAK_INLINE_COUNT`
`WEAK_INLINE_COUNT` 宏定义，
```c++
/**
 * The internal structure stored in the weak references table. 
 * internal structure(内部结构) 存储在弱引用表中。
 
 * It maintains and stores a hash set of weak
 * references pointing to an object.
 *
 * 它维护并存储了指向对象的弱引用的哈希表。
 *（对象只有一个，指向该对象的 __weak 变量可以有多个, 
 * 这些 __weak 变量统一放在一个数组里面）
 
 * If out_of_line_ness != REFERRERS_OUT_OF_LINE then
 * the set is instead a small inline array.
 *
 * 如果 out_of_line_ness != REFERRERS_OUT_OF_LINE(0x10)的话，
 * 数据用一个长度为 4 的内部数组存放，否则用 hash 数组存放 
 * 数组里存放的和哈希表 value 值的类型都是上面的 weak_referrer_t 
 */
 
#define WEAK_INLINE_COUNT 4 // 这个值固定为 4 表示内部小数组的长度是 4
```

## `REFERRERS_OUT_OF_LINE`
`REFERRERS_OUT_OF_LINE` 宏定义（0x10），这个值是用来标记在 `weak_entry_t` 中是用那个长度为 4 的定长数组存放 `weak_referrer_t（__weak 变量的指针）`，还是用 `hash` 数组来存放数据。
```c++
// out_of_line_ness field overlaps with the
// low two bits of inline_referrers[1].
// 
// out_of_line_ness 字段与 inline_referrers[1] 
// 的低两位内存空间重叠，下面会详细分析。
// 它们共用 32 字节内存空间。

// inline_referrers[1] is a DisguisedPtr of a pointer-aligned address.
// inline_referrers[1] 是一个遵循指针对齐的 DisguisedPtr。

// The low two bits of a pointer-aligned DisguisedPtr will
// always be 0b00 (disguised nil or 0x80..00) or 0b11 (any other address).
// 且指针对齐的 DisguisedPtr 的低两位将始终为
// 0b00(如 disguised nil or 0x80..00) 或 0b11（任何其他地址）

// Therefore out_of_line_ness == 0b10 is used to mark the out-of-line state.
// 因此我们可以使用 out_of_line_ness == 0b10 用于标记
// out-of-line 状态，表示使用的是定长数组还是动态数组。

#define REFERRERS_OUT_OF_LINE 2 
```

## `struct weak_entry_t`
下面正式进入 `weak_entry_t` ⛽️，`weak_entry_t` 的结构和 `weak_table_t` 很像，同样是一个 `hash` 表。`weak_entry_t` 的 `hash` 数组存储的数据是 `weak_referrer_t`，实质上是弱引用该对象的指针的指针，即 `objc_object **new_referrer`，通过操作指针的指针，就可以使得 `weak` 引用的指针在对象析构后，指向 `nil`。
`struct weak_entry_t` 定义:
```c++
struct weak_entry_t {
    // T 为 objc_object 的 DisguisedPtr
    // 那么 Disguised 中存放的就是化身为整数的 objc_object 实例的地址
    // 被 __weak 变量弱引用的对象
    DisguisedPtr<objc_object> referent;
    
    // 引用该对象的 __weak 变量的指针列表
    // 当引用该对象的 weak 变量个数小于等于 4 时用
    // weak_referrer_t inline_referrers[WEAK_INLINE_COUNT] 数组，
    // 大于 4 时用 hash 数组 weak_referrer_t *referrers
    union {
        // 两个结构体占用内存都是 32 个字节
        struct {
            weak_referrer_t *referrers; // 弱引用该对象的对象指针地址的 hash 数组
            
            // out_of_line_ness 和 num_refs 构成位域存储，共占 64 位
            // 标记是否使用动态 hash 数组
            uintptr_t        out_of_line_ness : 2;
            // PTR_MINUS_2 值为 30/62
            uintptr_t        num_refs : PTR_MINUS_2;
            
            // hash 数组长度减 1，会参与 hash 函数计算
            uintptr_t        mask;
            
            // 当发生 hash 冲突时，采用了开放寻址法
            // 可能会发生 hash 冲突的最大次数，用于判断是否出现了逻辑错误
            //（hash 表中的冲突次数绝对不会超过该值）
            // 该值在新建 entry 和插入新的 weak_referrer_t 时会被更新，
            // 它一直记录的都是最大偏移值
            uintptr_t        max_hash_displacement;
        };
        struct {
            // out_of_line_ness field is low bits of inline_referrers[1]
            // out_of_line_ness 字段是 inline_referrers[1] 的低位
            // 长度为 4 的 weak_referrer_t（Dsiguised<objc_object *>）数组
            // 存放的就是那些 __weak 变量的指针（__weak 变量实质是指向原始对象类型的指针）
            weak_referrer_t  inline_referrers[WEAK_INLINE_COUNT];
        };
    };
    
    // 返回 true 表示用 hash 数组存放 __weak 变量的指针
    // 返回 false 表示用那个长度为 4 的小数组存 __weak 变量的指针
    bool out_of_line() {
        return (out_of_line_ness == REFERRERS_OUT_OF_LINE);
    }
    
    // 赋值操作，直接使用 memcpy 拷贝内存地址里面的内容，并返回 *this
    weak_entry_t& operator=(const weak_entry_t& other) {
        memcpy(this, &other, sizeof(other));
        return *this;
    }

    // struct weak_entry_t 的构造函数
    // newReferent，是我们的原始对象的指针
    // newReferrer，则是我们的 __weak 变量的指针，即 objc_object 的指针的指针
    //（这里总是觉的说 __weak 变量，好像缺点什么，其实只要谨记它本质也是一个 objc_object 指针就好了）
    // 初始化列表直接把 newReferent 赋值给 referent
    // 此时会调用: DisguisedPtr(T* ptr) : value(disguise(ptr)) { } 构造函数
    // 调用 disguise 函数把 newReferent 地址转化为一个整数赋值给 value
    weak_entry_t(objc_object *newReferent, objc_object **newReferrer)
        : referent(newReferent)
    {
        // newReferrer 放在数组 0 位，并把其他位置为 nil
        inline_referrers[0] = newReferrer;
        for (int i = 1; i < WEAK_INLINE_COUNT; i++) {
            inline_referrers[i] = nil;
        }
    }
};
```
`weak_entry_t` 的结构比较清晰:
+ `DisguisedPtr<objc_object> referent;` 弱引用对象的地址转为整数并取负。
+ `union` 有两种形式，长度为 4 的固定数组：`weak_referrer_t  inline_referrers[WEAK_INLINE_COUNT]` 和 动态数组 `weak_referrer_t *referrers` ，这两个数组用来存储弱引用该对象弱引用指针的指针，同样使用 `DisguisedPtry` 的形式存储。当弱引用该对象的 `weak` 变量的数目小于等于 `WEAK_INLINE_COUNT` 时，使用 `inline_referrers`，否则使用动态数组，并且把定长数组中的元素都转移到动态数组中，并之后都是使用动态数组存储。
+ `bool out_of_line()` 该方法用来判断当前的 `weak_entry_t` 是使用定长数组还是动态数组。返回 `true` 是动态数组，返回 `false` 是定长数组，而且看到 `out_of_line_ness` 的内存地址是和 `inline_referrers` 数组的第二个元素低两位内存地址是重合的（`inline_referrers[1]` 低两位正由此来），这里涉及到了位域的概念。
+ `weak_entry_t& operator=(const weak_entry_t& other)` 赋值函数则直接调用了 `memcpy` 对内存空间直接拷贝。
+ `weak_entry_t(objc_object *newReferent, objc_object **newReferrer)` 构造函数，则把定长数组空位初始化位 `nil`。

**之所以使用定长/动态数组的切换，应该是考虑到某对象弱引用的个数一般不会超过 `WEAK_INLINE_COUNT` 个，这时候使用定长数组不需要动态的申请内存空间，（`union` 中两个结构体共用 32  个字节内存）而是一次分配一块连续的内存空间，这会得到运行效率上的提升。**

## `out_of_line`
关于 `weak_entry_t` 是使用定长数组还是动态 `hash` 数组:
```c++
bool out_of_line() {
    // #define REFERRERS_OUT_OF_LINE 2
    return (out_of_line_ness == REFERRERS_OUT_OF_LINE);
}
```

## 赋值和构造函数
赋值操作，直接使用 `memcpy` 拷贝内存地址里面的内容，并返回 `*this`，而不是用复制构造函数什么的形式实现，应该也是为了提高效率考虑。
构造函数则是 `referent(newReferent)` 初始化 `referent`，并把第一个 `weak` 变量地址放在`inline_referrers` 首位，然后一个循环把 `inline_referrers` 后三个元素置为 `nil`。



**参考链接:🔗**
+ [使用intptr_t和uintptr_t](https://www.jianshu.com/p/03b7d56bf80f)
+ [Object Runtime -- Weak](https://cloud.tencent.com/developer/article/1408976)
+ [OC Runtime之Weak(2)---weak_entry_t](https://www.jianshu.com/p/045294e1f062)
+ [iOS 关联对象 - DisguisedPtr](https://www.jianshu.com/p/cce56659791b)
+ [Objective-C运行时-动态特性](https://zhuanlan.zhihu.com/p/59624358)
+ [Objective-C runtime机制(7)——SideTables, SideTable, weak_table, weak_entry_t](https://blog.csdn.net/u013378438/article/details/82790332)
