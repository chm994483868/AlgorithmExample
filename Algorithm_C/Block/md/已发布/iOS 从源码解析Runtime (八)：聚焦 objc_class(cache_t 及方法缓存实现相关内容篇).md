# iOS 从源码解析Runtime (七)：聚焦 objc_class(cache_t 及方法缓存实现相关内容篇)

> 前面连续几篇我们已经详细分析了 `objc_object` 的相关的所有源码，接下来几篇则开始分析定义于 `objc-runtime-new.h` 中的 `objc_class`，本篇先从 `struct objc_class : objc_object` 的 `cache_t cache` 开始，`cache_t` 主要实现方法缓存，帮助我们更快的找到方法地址进行调用。
  纵览 `objc-runtime-new.h` 文件真的超长，那我们就分块来学习，一起 ⛽️⛽️ 吧！

```c++
struct objc_class : objc_object {
// Class ISA; // objc_class 继承自 objc_object，所以其第一个成员变量其实是 isa_t isa 
Class superclass; // 父类指针
cache_t cache; // formerly cache pointer and vtable 以前缓存指针和虚函数表
...
};
```
> `typedef uintptr_t SEL;` 在 `Project Headers/objc-runtime-new.h` 的 `198` 行，看到 `SEL`。 

&emsp;`cache` 是 `objc_class` 的第三个成员变量，类型是 `cache_t`。从数据结构角度及使用方法来看 `cache_t` 的话，它是一个 `SEL`  作为 `Key` ，`SEL + IMP(bucket_t)` 作为 `Value` 的散列表。为了对方法缓存先有一个大致的了解，我们首先解读一下 `Source/objc-cache.mm` 文件开头的一大段注释内容。
```c++
/*
objc-cache.m

1. Method cache management 方法缓存管理
2. Cache flushing 缓存刷新
3. Cache garbage collection 缓存垃圾回收
4. Cache instrumentation 缓存检测
5. Dedicated allocator for large caches 大型缓存的专用分配器 (?)

/*
Method cache locking (GrP 2001-1-14)
For speed, objc_msgSend does not acquire any locks when it reads method caches. 
为了提高速度，objc_msgSend 在读取方法缓存时不获取任何锁。

Instead, all cache changes are performed
so that any objc_msgSend running concurrently with the cache mutator 
will not crash or hang or get an incorrect result from the cache. 
相反，将执行所有缓存更改，以便与缓存 mutator 并发运行的任何 objc_msgSend 都不会崩溃或挂起，
或者从缓存中获得不正确的结果。（以 std::atomic 完成所有的原子操作）

When cache memory becomes unused (e.g. the old cache after cache expansion), 
it is not immediately freed, because a concurrent objc_msgSend could still be using it. 
当缓存未使用时，（例如：缓存扩展后的旧缓存），它不会立即释放，因为并发的 objc_msgSend 可能仍在使用它。

Instead, the memory is disconnected from the data structures and placed on a garbage list. 
相反，内存与数据结构断开连接，并放在垃圾列表中。

The memory is now only accessible to instances of objc_msgSend that were
running when the memory was disconnected; 
内存现在只能访问断开内存时运行的 objc_msgSend 实例；

any further calls to objc_msgSend will not see the garbage memory because
the other data structures don't point to it anymore.
对 objc_msgSend 的任何进一步调用都不会看到垃圾内存，因为其他数据结构不再指向它。

The collecting_in_critical function checks the PC of all threads and returns FALSE when
all threads are found to be outside objc_msgSend. 
collecting_in_critical 函数检查所有线程的PC，当发现所有线程都在 objc_msgSend 之外时返回 FALSE。

This means any call to objc_msgSend that could have had access to the garbage has
finished or moved past the cache lookup stage, so it is safe to free the memory. 
这意味着可以访问垃圾的对 objc_msgSend 的任何调用都已完成或移动到缓存查找阶段，因此可以安全地释放内存。

All functions that modify cache data or structures must acquire the cacheUpdateLock
to prevent interference from concurrent modifications. 
所有修改缓存数据或结构的函数都必须获取 cacheUpdateLock，以防止并发修改的干扰。

The function that frees cache garbage must acquire the cacheUpdateLock and use
collecting_in_critical() to flush out cache readers.
释放缓存垃圾的函数必须获取 cacheUpdateLock，并使用 collecting_in_critical() 清除缓存读取器

The cacheUpdateLock is also used to protect the custom allocator used for large method cache blocks.
Cache readers (PC-checked by collecting_in_critical())
cacheUpdateLock 还用于保护用于大型方法缓存块的自定义分配器。缓存读取器（由collecting_in_critical() 进行 PC 检查）

objc_msgSend
cache_getImp

Cache writers (hold cacheUpdateLock while reading or writing; not PC-checked)
(读取或写入时获取 cacheUpdateLock，不使用 PC-checked)

cache_fill         (acquires lock)(获取锁)
cache_expand       (only called from cache_fill)(仅从 cache_fill 调用)
cache_create       (only called from cache_expand)(仅从 cache_expand 调用)
bcopy               (only called from instrumented cache_expand)(仅从已检测的 cache_expand 调用)
flush_caches        (acquires lock)(获取锁)
cache_flush        (only called from cache_fill and flush_caches)(仅从 cache_fill 和 flush_caches 调用)
cache_collect_free (only called from cache_expand and cache_flush)(仅从 cache_expand 和 cache_flush 调用)

UNPROTECTED cache readers (NOT thread-safe; used for debug info only)(不是线程安全的；仅用于调试信息)
cache_print cache 打印
_class_printMethodCaches
_class_printDuplicateCacheEntries
_class_printMethodCacheStatistics
*/
```
&emsp;到这里就看完注释了，有点懵，下面还是把源码一行一行看完，然后再回顾上面的内容到底指的是什么。

## `CACHE_IMP_ENCODING/CACHE_MASK_STORAGE`
&emsp;在进入 `cache_t/bucket_t` 内容之前，首先看两个宏定义，`CACHE_IMP_ENCODING` 表示在 `bucket_t` 中 `IMP` 的存储方式，`CACHE_MASK_STORAGE` 表示 `cache_t` 中掩码的位置。`struct bucket_t` 和 `struct cache_t` 里面的不同实现部分正是根据这两个宏来判断的。
我们最关注的 `x86_64(mac)` 和 `arm64(iphone)` 两个平台下 `bucket_t` 中 `IMP` 都是以 `ISA` 与 `IMP` 异或的值存储。而掩码位置的话 `x86_64` 下是 `CACHE_MASK_STORAGE_OUTLINED` 没有掩码，`buckets` 散列数组和 `_mask` 以两个成员变量分别表示。在 `arm64` 下则是 `CACHE_MASK_STORAGE_HIGH_16` 高 `16` 位为掩码，散列数组和 `mask` 共同保存在 `_maskAndBuckets` 中。 
```c++
// 三种方法缓存存储 IMP 的方式：（bucket_t 中 _imp 成员变量存储 IMP 的方式）
// Determine how the method cache stores IMPs.
// 确定方法缓存如何存储 IMPs. (IMP 是函数实现的指针，保存了函数实现的地址，根据 IMP 可以直接执行函数...)

// Method cache contains raw IMP. 方法缓存包含原始的 IMP（bucket_t 中 _imp 为 IMP）
#define CACHE_IMP_ENCODING_NONE 1 

// Method cache contains ISA ^ IMP. 方法缓存包含 ISA 与 IMP 的异或（bucket_t 中 _imp 是 IMP ^ ISA）
#define CACHE_IMP_ENCODING_ISA_XOR 2 

// Method cache contains ptrauth'd IMP. 
// 方法缓存包含指针验证的 IMP (bucket_t 中 _imp 是 ptrauth_auth_and_resign 函数计算的值)
#define CACHE_IMP_ENCODING_PTRAUTH 3 

// 上述三种方式的各在什么平台使用：

#if __PTRAUTH_INTRINSICS__ // 未找到该宏定义的值

// Always use ptrauth when it's supported. 当平台支持 __PTRAUTH_INTRINSICS__ 时总是使用指针验证
// 此时 CACHE_IMP_ENCODING 为 CACHE_IMP_ENCODING_PTRAUTH

#define CACHE_IMP_ENCODING CACHE_IMP_ENCODING_PTRAUTH

#elif defined(__arm__)

// 32-bit ARM uses no encoding. 32位 ARM 下不进行编码，直接使用原始 IMP 
#define CACHE_IMP_ENCODING CACHE_IMP_ENCODING_NONE

#else

// Everything else uses ISA ^ IMP. 其它情况下在方法缓存中存储 ISA 与 IMP 异或的值
#define CACHE_IMP_ENCODING CACHE_IMP_ENCODING_ISA_XOR

#endif
```
```c++
// CACHE 中掩码位置
#define CACHE_MASK_STORAGE_OUTLINED 1 // 没有掩码
#define CACHE_MASK_STORAGE_HIGH_16 2 // 高 16 位
#define CACHE_MASK_STORAGE_LOW_4 3 // 低 4 位

#if defined(__arm64__) && __LP64__ // 如果是 64 位 ARM 平台（iPhone 自 5s 起都是）

// 掩码存储在高 16 位
#define CACHE_MASK_STORAGE CACHE_MASK_STORAGE_HIGH_16 

#elif defined(__arm64__) && !__LP64__ // ARM 平台 非 64 位系统架构

// 掩码存储在低 4 位
#define CACHE_MASK_STORAGE CACHE_MASK_STORAGE_LOW_4

#else

// 不使用掩码的方式（_buckets 与 _mask 是两个变量）
#define CACHE_MASK_STORAGE CACHE_MASK_STORAGE_OUTLINED

#endif
```
## `bucket_t`
&emsp;看到 `bucket_t` 一下想起了 `RefcountMap refcnts` 中保存对象引用计数时使用的数据结构 `typename BucketT = detail::DenseMapPair<KeyT, ValueT>>` 用于保存对象的地址和对象的引用计数。`bucket_t` 基本也是大致相同的作用，这里是把函数的 `SEL`  和函数的实现地址 `IMP` 保存在 `bucket_t` 这个结构体中。这里先看一下 `bucket_t` 定义的 `private` 部分:
```c++
struct bucket_t {
private:
    // 为了优化性能，针对 __arm64__ 和其它平台调换 _imp 和 _sel 两个成员变量的顺序
    
    // IMP-first is better for arm64e ptrauth and no worse for arm64.
    // IMP 在前时对 arm64e 的指针验证机制更好，对 arm64 也不差。
    
    // SEL-first is better for armv7* and i386 and x86_64.
    // SEL 在前时对 armv7* i386 x86_64 更好。
    
    // typedef unsigned long uintptr_t;
    // template <typename T> struct explicit_atomic : public std::atomic<T> { ... };
    // 禁止隐式转换，T 操作为原子操作，避免多线程竞争
    
#if __arm64__
    explicit_atomic<uintptr_t> _imp;  
    explicit_atomic<SEL> _sel;
#else
    explicit_atomic<SEL> _sel;
    explicit_atomic<uintptr_t> _imp;
#endif
    
    // Compute the ptrauth signing modifier from &_imp, newSel, and cls.
    // 从 ＆_imp、newSel 和 cls 计算 ptrauth 签名修饰符。
    uintptr_t modifierForSEL(SEL newSel, Class cls) const {
        // 连续异或
        return (uintptr_t)&_imp ^ (uintptr_t)newSel ^ (uintptr_t)cls;
    }

    // Sign newImp, with &_imp, newSel, and cls as modifiers.
    // 签署 newImp，使用 ＆_imp、newSel 和 cls 作修改。
    uintptr_t encodeImp(IMP newImp, SEL newSel, Class cls) const {
        // 如果 newImp 为 nil，返回 0
        if (!newImp) return 0;
#if CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_PTRAUTH
    // 如果 IMP 编码使用指针验证机制
        return (uintptr_t)
            ptrauth_auth_and_resign(newImp,
                                    ptrauth_key_function_pointer, 0,
                                    ptrauth_key_process_dependent_code,
                                    modifierForSEL(newSel, cls));
                                    
#elif CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_ISA_XOR

        // IMP 与 Class 作异或的值
        return (uintptr_t)newImp ^ (uintptr_t)cls;
        
#elif CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_NONE

        // 直接使用原始 IMP
        return (uintptr_t)newImp;
        
#else

#error Unknown method cache IMP encoding. // 未知方式

#endif
    }
...
};
```
`bucket_t` 定义的 `public` 部分:
```c++
public:
    // 原子读取 _sel
    inline SEL sel() const { return _sel.load(memory_order::memory_order_relaxed); }

    // 根据 cls 从 bucket_t 实例中取得 _imp
    inline IMP imp(Class cls) const {
        // 首先原子读取 _imp
        uintptr_t imp = _imp.load(memory_order::memory_order_relaxed);
        
        // 如果 imp 不存在，则返回 nil
        if (!imp) return nil;
        
#if CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_PTRAUTH
        // 原子读取 _sel
        SEL sel = _sel.load(memory_order::memory_order_relaxed);
        // 计算返回 IMP
        return (IMP)
            ptrauth_auth_and_resign((const void *)imp,
                                    ptrauth_key_process_dependent_code,
                                    modifierForSEL(sel, cls),
                                    ptrauth_key_function_pointer, 0);
#elif CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_ISA_XOR

        // imp 与 cls 再进行一次异或，返回原值，得到 encodeImp 传入的 newImp
        return (IMP)(imp ^ (uintptr_t)cls);
        
#elif CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_NONE

        // 原始 IMP
        return (IMP)imp;
#else

#error Unknown method cache IMP encoding. // 未知

#endif
    }

    // enum Atomicity { Atomic = true, NotAtomic = false };
    // enum IMPEncoding { Encoded = true, Raw = false };
    
    // set 函数 的声明，按住 command 点击 set，可看到 set 函数定义在 objc-cache.mm 中
    template <Atomicity, IMPEncoding>
    void set(SEL newSel, IMP newImp, Class cls);
};
```
### `set`
&emsp;`set` 函数完成的功能是以原子方式完成 `bucket_t` 实例 `_imp` 和 `_sel` 成员变量的设置。 

`memory_order` 的值可参考: [《如何理解 C++11 的六种 memory order？》](https://www.zhihu.com/question/24301047)
```c++
// 非 __arm64__ 平台下(x86_64 下):
template<Atomicity atomicity, IMPEncoding impEncoding>
void bucket_t::set(SEL newSel, IMP newImp, Class cls)
{
    // 当前 bucket_t 实例的 _sel 为 0 或者与传入的 newSel 相同
    // DEBUG 下 bucket_t 的 _sel 和 newSel 不同就会执行断言 ？
    
    ASSERT(_sel.load(memory_order::memory_order_relaxed) == 0 ||
           _sel.load(memory_order::memory_order_relaxed) == newSel);

    // objc_msgSend uses sel and imp with no locks.
    // objc_msgSend 使用 sel 和 imp 不会加锁。
    
    // It is safe for objc_msgSend to see new imp but NULL sel 
    // objc_msgssend 可以安全地看到新的 imp 和 NULL 的 sel。
    
    // (It will get a cache miss but not dispatch to the wrong place.
    //  它将导致缓存未命中，但不会分派到错误的位置。)
    
    // It is unsafe for objc_msgSend to see old imp and new sel.
    // objc_msgSend 查看旧的 imp 和新的 sel 是不安全的。
    
    // Therefore we write new imp, wait a lot, then write new sel.
    // 因此，我们首先写入新的 imp，等一下，然后再写入新的 sel。
    
    // 根据 impEncoding 判断 新 IMP 是需要做异或求值还是直接使用
    uintptr_t newIMP = (impEncoding == Encoded
                        ? encodeImp(newImp, newSel, cls)
                        : (uintptr_t)newImp);

    if (atomicity == Atomic) {
        // 如果是 Atomic
        // 首先把 newIMP 存储到 _imp
        _imp.store(newIMP, memory_order::memory_order_relaxed);
        
        if (_sel.load(memory_order::memory_order_relaxed) != newSel) {
        // 如果当前 _sel 与 newSel 不同，则根据不同的平台来设置 _sel
        
#ifdef __arm__
            // barrier
            mega_barrier();
            _sel.store(newSel, memory_order::memory_order_relaxed);
#elif __x86_64__ || __i386__
            _sel.store(newSel, memory_order::memory_order_release);
#else

// 不知道如何在此架构上执行 bucket_t::set。
#error Don't know how to do bucket_t::set on this architecture.

#endif
        }
    } else {
        // 原子保存 _imp
        _imp.store(newIMP, memory_order::memory_order_relaxed);
        // 原子保存 _sel
        _sel.store(newSel, memory_order::memory_order_relaxed);
    }
}
```
&emsp;首先要把 `newImp` 写入，`__arm64__` 下 `set` 函数的实现涉及一个 `__asm__` 好像涉及到 `ARM` 的内存排序内存屏障啥的看不懂😭。
`struct bucket_t` 到这里就结束了，主要用来保存函数的 `SEL` 和 `IMP`（`IMP` 根据不同的编码方式来保存）。 

## `cache_t`
&emsp;`cache_t` 是作为一个散列数组来缓存方法的。先看下 `cache_t` 定义的 `private` 部分:
### `mask_t`:
```c++
#if __LP64__

// x86_64 & arm64 asm are less efficient with 16-bits
// x86_64 和 arm64 asm 的 16 位效率较低

typedef uint32_t mask_t; // 32 位 4 字节 int
#else

typedef uint16_t mask_t; // 16 位 2 字节 int
#endif
```
### `struct cache_t private`
```c++
struct cache_t {
#if CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_OUTLINED
    // 如果没有掩码的话
    
    // _buckets 是 struct bucket_t 类型的数组
    // 方法的缓存数组（以散列表的形式存储 bucket_t）
    explicit_atomic<struct bucket_t *> _buckets;
    
    // _buckets 的数组长度 -1（容量的临界值）
    explicit_atomic<mask_t> _mask;
    
#elif CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_HIGH_16
    // 高 16 位是掩码的平台（iPhone 5s 以后的真机）
    
    // 掩码和 Buckets 指针共同保存在 uintptr_t 类型的 _maskAndBuckets 中
    explicit_atomic<uintptr_t> _maskAndBuckets;
    
    mask_t _mask_unused; // 未使用的容量
    
    // How much the mask is shifted by.
    // 高 16 位是 mask，即 _maskAndBuckets 右移 48 位得到 mask
    static constexpr uintptr_t maskShift = 48;
    
    // Additional bits after the mask which must be zero. 
    // msgSend takes advantage of these additional bits to construct the value `mask << 4` from `_maskAndBuckets` in a single instruction.
    // 掩码后的其他位必须为零。
    // msgSend 利用这些额外的位在单个指令中从 _maskAndBuckets 构造了值 mask<< 4
    static constexpr uintptr_t maskZeroBits = 4;
    
    // The largest mask value we can store.
    // 我们可以保存的最大的 mask 值。
    // (64 - maskShiift) 即掩码位数，然后 1 左移掩码位数后再 减 1 即 16 位能保存的最大二进制值
    //（16 位 1，其余位都是 0 的数值）
    static constexpr uintptr_t maxMask = ((uintptr_t)1 << (64 - maskShift)) - 1;
    
    // The mask applied to `_maskAndBuckets` to retrieve the buckets pointer.
    // 应用于 _maskAndBuckets 的掩码，以获取 buckets 指针。
    // 1 左移 44(48 - 4) 位后再 减 1（44 位 1，其余都是 0 的数值）
    static constexpr uintptr_t bucketsMask = ((uintptr_t)1 << (maskShift - maskZeroBits)) - 1;
    
    // Ensure we have enough bits for the buckets pointer.
    // 确保我们有足够的位用于存储 buckets 指针。
    static_assert(bucketsMask >= MACH_VM_MAX_ADDRESS, "Bucket field doesn't have enough bits for arbitrary pointers.");
    
#elif CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_LOW_4
    // _maskAndBuckets stores the mask shift in the low 4 bits, and the buckets pointer in the remainder of the value. 
    // The mask shift is the value where (0xffff >> shift) produces the correct mask.
    // This is equal to 16 - log2(cache_size).
    // _maskAndBuckets 将掩码移位存储在低 4 位中，并将 buckets 指针存储在该值的其余部分中。
    // 掩码 shift 是（0xffff >> shift）产生正确掩码的值。
    // 等于 16 - log2(cache_size)
    
    // 几乎同上
    explicit_atomic<uintptr_t> _maskAndBuckets;
    mask_t _mask_unused;

    static constexpr uintptr_t maskBits = 4;
    static constexpr uintptr_t maskMask = (1 << maskBits) - 1;
    static constexpr uintptr_t bucketsMask = ~maskMask;
#else

#error Unknown cache mask storage type. // 未知掩码存储类型

#endif

#if __LP64__
    // 如果是 64 位环境的话会多一个 _flags 标志位
    uint16_t _flags;
#endif

    uint16_t _occupied; // 缓存数组的已占用量
...
};
```
### `struct cache_t public`
`cache_t` 定义的 `public` 部分:
&emsp;`cache_t` 的实现部分也是涉及到不同的平台下不同的实现，这里只分析 `CACHE_MASK_STORAGE_OUTLINED(x86_64)` 和 `CACHE_MASK_STORAGE_HIGH_16 (__arm64__ && __LP64__)` 两个平台的实现。

#### `emptyBuckets`
&emsp;一个指向 `_objc_empty_cache` 的 `bucket_t` 指针，用来指示当前类的缓存指向空缓存。（`_objc_empty_cache` 是一个外联变量）
```c++
// OBJC2 不可见
struct objc_cache {
    unsigned int mask /* total = mask + 1 */                 OBJC2_UNAVAILABLE;
    unsigned int occupied                                    OBJC2_UNAVAILABLE;
    Method _Nullable buckets[1]                              OBJC2_UNAVAILABLE;
};

OBJC_EXPORT struct objc_cache _objc_empty_cache OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);

//位于 objc-cache-old.mm 中
// 静态的空缓存，所有类最初都指向此缓存。
// 发送第一条消息时，它在缓存中未命中，当缓存新增时，它会检查这种情况，并使用 malloc 而不是 realloc。
// 这避免了在 Messenger 中检查 NULL 缓存的需要。
struct objc_cache _objc_empty_cache =
{
    0,        // mask
    0,        // occupied
    { NULL }  // buckets
};

// CACHE_MASK_STORAGE_OUTLINED 下
struct bucket_t *cache_t::emptyBuckets()
{
    // 直接使用 & 取 _objc_empty_cache 的地址并返回，
    // _objc_empty_cache 是一个全局变量，用来标记当前类的缓存是一个空缓存
    return (bucket_t *)&_objc_empty_cache;
}

// CACHE_MASK_STORAGE_HIGH_16 下（看到和 OUTLINED 完全一样）
struct bucket_t *cache_t::emptyBuckets()
{
    return (bucket_t *)&_objc_empty_cache;
}
```
#### `buckets`
&emsp;散列表数组的起始地址。
```c++
// CACHE_MASK_STORAGE_OUTLINED
// 没有任何 "锁头"，原子加载 _buckets 并返回
struct bucket_t *cache_t::buckets() 
{
    // 原子加载 _buckets 并返回
    return _buckets.load(memory_order::memory_order_relaxed);
}

// CACHE_MASK_STORAGE_HIGH_16 
// 也是没有任何 "锁头"，原子加载 _maskAndBuckets，然后与 bucketsMask 掩码与操作并把结果返回
struct bucket_t *cache_t::buckets()
{
    // 原子加载 _maskAndBuckets
    uintptr_t maskAndBuckets = _maskAndBuckets.load(memory_order::memory_order_relaxed);
    // 然后与 bucketsMask 做与操作并返回结果。
    //（bucketsMask 的值是 低 44 位是 1，其它位全部是 0，与操作取出 maskAndBuckets 低 44 位的值）
    return (bucket_t *)(maskAndBuckets & bucketsMask);
}
```
#### `mask`
&emsp;`_buckets` 的数组长度 -1（容量的临界值）。
```c++
// CACHE_MASK_STORAGE_OUTLINED
mask_t cache_t::mask() 
{
    // 没有任何锁头，原子加载 _mask 并返回
    return _mask.load(memory_order::memory_order_relaxed);
}

// CACHE_MASK_STORAGE_HIGH_16
mask_t cache_t::mask()
{
    // 原子加载 _maskAndBuckets
    uintptr_t maskAndBuckets = _maskAndBuckets.load(memory_order::memory_order_relaxed);
    // maskAndBuckets 左移 48 位即得到了 mask，并返回此值。（高 16 位保存 mask）
    return maskAndBuckets >> maskShift;
}

#if __LP64__
typedef uint32_t mask_t;  // x86_64 & arm64 asm are less efficient with 16-bits
#else
typedef uint16_t mask_t;
#endif

typedef uintptr_t SEL;
```
&emsp;这里有一个点，在 `CACHE_MASK_STORAGE_HIGH_16` 时是 `__LP64__` 平台，`mask_t` 在 `__LP64__` 下是 `uint32_t`，多出了 `16` 位空间，`mask` 只需要 `16` 位就足够保存。注释给出的解释是: " `x86_64` 和 `arm64` `asm` 的 `16` 位效率较低。"

#### `occupied/incrementOccupied`
```c++
mask_t cache_t::occupied() 
{
    return _occupied; // 返回 _occupied
}

void cache_t::incrementOccupied() 
{
    _occupied++; // _occupied 自增
}
```
#### `setBucketsAndMask`
&emsp;设置 `_buckets` 与 `_mask` 的值，`CACHE_MASK_STORAGE_OUTLINED` 模式只需要分别以原子方式设置两个成员变量的值即可，`CACHE_MASK_STORAGE_HIGH_16` 模式需要把两个值做位操作合并在一起然后以原子方式保存在 `_maskAndBuckets` 中。同时以上两种情况都会顺便把 `_occupied` 设置为 `0`。
```c++
// CACHE_MASK_STORAGE_OUTLINED
void cache_t::setBucketsAndMask(struct bucket_t *newBuckets, mask_t newMask)
{
    // objc_msgSend uses mask and buckets with no locks.
    // objc_msgSend 使用 mask 和 buckets 不会进行加锁。
    
    // It is safe for objc_msgSend to see new buckets but old mask.
    // 对于 objc_msgSend 来说，看到新的 buckets 和旧的 mask 是安全的。
    
    // (It will get a cache miss but not overrun the buckets' bounds).
    // (它将获得缓存未命中，但不会超出存储桶的界限。)
    
    // It is unsafe for objc_msgSend to see old buckets and new mask.
    // objc_msgSend 查看旧 buckets 和新 mask 是不安全的。
    
    // Therefore we write new buckets, wait a lot, then write new mask.
    // 所以我们先写入新的 buckets，写入完成后，再写入新的 mask。
    
    // objc_msgSend reads mask first, then buckets.
    // objc_msgSend 首先读取 mask，然后读取 buckets。

#ifdef __arm__
    // ensure other threads see buckets contents before buckets pointer
    // 确保其他线程在 buckets 指针之前看到 buckets 内容
    mega_barrier();

    _buckets.store(newBuckets, memory_order::memory_order_relaxed);
    
    // ensure other threads see new buckets before new mask
    // 确保其他线程在新 mask 之前看到新 buckets
    mega_barrier();
    
    _mask.store(newMask, memory_order::memory_order_relaxed);
    _occupied = 0; // _occupied 置为 0
#elif __x86_64__ || i386
    // ensure other threads see buckets contents before buckets pointer
    // 确保其他线程在 buckets 指针之前看到 buckets 内容
    // 以原子方式保存 _buckets
    _buckets.store(newBuckets, memory_order::memory_order_release);
    
    // ensure other threads see new buckets before new mask
    // 确保其他线程在新 mask 之前看到新 buckets
    // 以原子方式保存 _mask
    _mask.store(newMask, memory_order::memory_order_release);
    _occupied = 0; // _occupied 置为 0
#else

// 不知道如何在此架构上执行 setBucketsAndMask。
#error Don't know how to do setBucketsAndMask on this architecture.

#endif
}

// CACHE_MASK_STORAGE_HIGH_16
void cache_t::setBucketsAndMask(struct bucket_t *newBuckets, mask_t newMask)
{
    // 转为 unsigned long
    uintptr_t buckets = (uintptr_t)newBuckets;
    uintptr_t mask = (uintptr_t)newMask;
    
    // 断言: buckets 小于等于 buckets 的掩码（bucketsMask 的值低 44 位全为 1，其它位是 0）
    ASSERT(buckets <= bucketsMask);
    // 断言: mask 小于等于 mask 的最大值（maxMask 的值低 16 位全为 1，其它位是 0）
    ASSERT(mask <= maxMask);
    
    // newMask 左移 48 位然后与 newBuckets 做或操作，
    // 因为 newBuckets 高 16 位全部是 0，所以 newMask 左移 16 的值与 newBuckets 做或操作时依然保持不变
    // 把结果以原子方式保存在 _maskAndBuckets 中  
    _maskAndBuckets.store(((uintptr_t)newMask << maskShift) | (uintptr_t)newBuckets, std::memory_order_relaxed);
    // 把 _occupied 置为 0
    _occupied = 0;
}
```
#### `initializeToEmpty`
```c++
// bzero
// 头文件：#include <string.h>
// 函数原型：void bzero (void *s, int n);
// 功能：将字符串 s 的前 n 个字节置为 0，一般来说 n 通常取 sizeof(s)，将整块空间清零

// CACHE_MASK_STORAGE_OUTLINED
void cache_t::initializeToEmpty()
{
    // 把 this 的内存清零
    bzero(this, sizeof(*this));
    // 以原子方式把 _objc_empty_cache 的地址存储在 _buckets 中
    _buckets.store((bucket_t *)&_objc_empty_cache, memory_order::memory_order_relaxed);
}

// CACHE_MASK_STORAGE_HIGH_16
void cache_t::initializeToEmpty()
{
    // 把 this 的内存清零
    bzero(this, sizeof(*this));
    // 把 _objc_empty_cache 的地址转换为 uintptr_t然后以原子方式把其存储在 _maskAndBuckets 中 
    _maskAndBuckets.store((uintptr_t)&_objc_empty_cache, std::memory_order_relaxed);
}
```
&emsp;两种模式下都是把 `_objc_empty_cache` 的地址取出用于设置 `_buckets/_maskAndBuckets`，两种模式下也都对应上面的 `emptyBuckets` 函数，取出 `(bucket_t *)&_objc_empty_cache` 返回。  
#### `canBeFreed`
&emsp;`canBeFreed` 函数只有下面一种实现，看名字我们大概也能猜出此函数的作用，正式判断能不能释放 `cache_t`。
```c++
bool cache_t::canBeFreed()
{
    // 调用 isConstantEmptyCache 函数，如果它返回 true，
    // 则表明 cache_t 的 buckets 当前正是那些准备的标记 emptyBuckets 的静态值
    //（应该估计都是 cache_t::emptyBuckets() 全局的 (bucket_t *)&_objc_empty_cache 值），则不能进行释放，
    // 否则是我们自己申请的有效的方法缓存内容，可进行释放。
    return !isConstantEmptyCache();
}
```
#### `isConstantEmptyCache`
&emsp;看完下面的 `emptyBucketsForCapacity` 实现才知道 `isConstantEmptyCache` 中 `Constant` 的含义。
```c++
bool cache_t::isConstantEmptyCache()
{
    // occupied() 函数很简单就是获取 _occupied 成员变量的值然后直接返回，
    // _occupied 表示散列表中已占用的容量
    // 此处要求 occupied() 为 0 并且 buckets() 等于 emptyBucketsForCapacity(capacity(), false)
    // emptyBucketsForCapacity 函数则是根据 capacity() 去找其对应的 emptyBuckets，
    // 且这些 emptyBuckets 地址都是固定的，
    // 它们是作标记用的静态值，如果此时 buckets 正是这些个静态值，说明此时 cache_t 是一个空缓存
    return 
        occupied() == 0  &&  
        buckets() == emptyBucketsForCapacity(capacity(), false); 
        // 且这里用了 false 则下面不执行 emptyBucketsList 相关的申请空间的逻辑，会直接 if (!allocate) return nil;
}
```
#### `capacity`
```c++
// mask 是临界值，加 1 后就是散列表的容量
unsigned cache_t::capacity()
{
    return mask() ? mask()+1 : 0; 
}
```
### `emptyBucketsForCapacity`
&emsp;根据入参 `capacity`，返回一个指定 `capacity` 容量的空的散列表，返回的这个 `bucket_t *` 是  `static bucket_t **emptyBucketsList` 这个静态变量指定下标的值，当 `capacity` 位于指定的区间时，返回的 `bucket_t *` 都是相同的。
例如：`capacity` 值在 `[8，15]` 之内时，通过 `index = log2u(capacity)` 计算的 `index` 值都是相同的，那么调用 `emptyBucketsForCapacity` 函数返回的都是相同的 `emptyBucketsList[index]`。由于这里有 `EMPTY_BYTES` 限制，所以至少 `capacity`  大于 `9/1025` 才会使用到 `emptyBucketsList` 相关的逻辑，内部 `index` 是从 `3/10` 开始的。其它的情况则一律返回 `cache_t::emptyBuckets()`。
```c++
bucket_t *emptyBucketsForCapacity(mask_t capacity, bool allocate = true)
{
#if CONFIG_USE_CACHE_LOCK
    cacheUpdateLock.assertLocked();
#else
    runtimeLock.assertLocked(); // 走此分支，加锁（加锁失败会执行断言）
#endif

    // buckets 总字节占用（sizeof(bucket_t) * capacity）
    size_t bytes = cache_t::bytesForCapacity(capacity);
    
    // Use _objc_empty_cache if the buckets is small enough.
    // 如果 buckets 足够小的话使用 _objc_empty_cache。
    if (bytes <= EMPTY_BYTES) {
        // 小于 ((8+1)*16) 主要针对的是 DEBUG 模式下：
        // 小于 ((1024+1)*16) 非 DEBUG 模式（后面的乘以 16 是因为 sizeof(bucket_t) == 16）
        //（觉得这个 1025 的容量就已经很大大了，可能很难超过，大概率这里就直接返回 cache_t::emptyBuckets() 了）
        return cache_t::emptyBuckets();
    }

    // Use shared empty buckets allocated on the heap.
    // 使用在堆上分配的 shared empty buckets。
    
    // 静态的 bucket_t **，下次再进入 emptyBucketsForCapacity 函数的话依然依然是保持上次的值
    // 且返回值正是 emptyBucketsList[index]，就是说调用 emptyBucketsForCapacity 获取就是一个静态的定值
    static bucket_t **emptyBucketsList = nil;
    
    // 静态的 mask_t (uint32_t)，下次再进入 emptyBucketsForCapacity 函数的话依然依然是保持上次的值
    static mask_t emptyBucketsListCount = 0;
    
    // ⚠️
    // template <typename T>
    // static inline T log2u(T x) {
    //     return (x<2) ? 0 : log2u(x>>1)+1;
    // }
    
    // log2u 计算的是小于等于 x 的最大的 2 幂的指数
    // x 在 [8，15] 区间内，大于等于 2^3，所以返回值为 3
    // x 在 [16, 31] 区间内，大于等于 2^4, 所以返回值为 4
    
    mask_t index = log2u(capacity);


    if (index >= emptyBucketsListCount) {
        if (!allocate) return nil;

        // index + 1 此值还没有看出来是什么意思
        mask_t newListCount = index + 1;
        
        // 申请 bytes 个字节的容量，并赋值为 1
        // capacity 大于 9/1026 那么 bytes 大于 9 * 16/1026 * 16，16 Kb也可太大了
        bucket_t *newBuckets = (bucket_t *)calloc(bytes, 1);
        
        // ⚠️
        // extern void *realloc(void *mem_address, unsigned int newsize);
        //（数据类型*）realloc（要改变内存大小的指针名，新的大小）
        // 新的大小可大可小，如果新的大小大于原内存大小，则新分配部分不会被初始化；如果新的大小小于原内存大小，可能会导致数据丢失。
        // 注意事项: 重分配成功旧内存会被自动释放，旧指针变成了野指针。
        // 返回值: 如果重新分配成功则返回指向被分配内存的指针，否则返回空指针 NULL。
        
        // 先判断当前的指针是否有足够的连续空间，如果有，扩大 mem_address 指向的地址，并且将 mem_address 返回，
        // 如果空间不够，先按照 newsize 指定的大小分配空间，将原有数据从头到尾拷贝到新分配的内存区域，
        // 而后释放原来 mem_address 所指内存区域（注意：原来指针是自动释放，不需要使用 free），同时返回新分配的内存区域的首地址，即重新分配存储器块的地址。
        
        // 对 emptyBucketsList 进行扩容
        emptyBucketsList = (bucket_t**)
            realloc(emptyBucketsList, newListCount * sizeof(bucket_t *));
            
        // Share newBuckets for every un-allocated size smaller than index.
        // 对于每个小于索引的未分配大小，Share newBucket。
        // The array is therefore always fully populated.
        // 因此，array 始终总是完全填充。
        
        // 把新扩容的 emptyBucketsList 的 新位置上都放上 newBuckets
        for (mask_t i = emptyBucketsListCount; i < newListCount; i++) {
            // 把新扩容的 emptyBucketsList 的 新位置上都放上 newBuckets
            emptyBucketsList[i] = newBuckets;
        }
        
        // 更新 emptyBucketsListCount，且 emptyBucketsListCount 是函数内的静态局部变量，函数进来 emptyBucketsListCount 都保持上次的值
        emptyBucketsListCount = newListCount;
        
        // OPTION( PrintCaches, OBJC_PRINT_CACHE_SETUP, "log processing of method caches")
        if (PrintCaches) {
            // 如果开启了 OBJC_PRINT_CACHE_SETUP 则打印 
            _objc_inform("CACHES: new empty buckets at %p (capacity %zu)", 
                         newBuckets, (size_t)capacity);
        }
    }

    return emptyBucketsList[index];
}
```
#### `CONFIG_USE_CACHE_LOCK`
&emsp;`emptyBucketsForCapacity` 函数的入口就是一个 `CONFIG_USE_CACHE_LOCK` 宏定义，它是用来标志 `emptyBucketsForCapacity` 函数使用 `cacheUpdateLock` 还是 `runtimeLock`，注意这里针对的是 `Objective-C` 的版本，`__OBJC2__` 下使用的是 `runtimeLock` 否则使用 `cacheUpdateLock`。
```c++
// OBJC_INSTRUMENTED controls whether message dispatching is dynamically monitored.
// OBJC_INSTRUMENTED 控制是否动态监视消息调度。

// Monitoring introduces substantial overhead.
// 监控会带来大量开销。

// NOTE: To define this condition, do so in the build command, NOT by uncommenting the line here.  
// NOTE: 若要定义此条件，请在 build 命令中执行此操作，而不是取消下面 OBJC_INSTRUMENTED 的注释。

// This is because objc-class.h heeds this condition, but objc-class.h can not #include this file (objc-config.h) because objc-class.h is public and objc-config.h is not.
// 这是因为 objc-class.h 注意到了这个条件，但是 objc-class.h 不能包括这个文件（objc-config.h），因为 objc-class.h 是公共的，而 objc-config.h 不是。

//#define OBJC_INSTRUMENTED

// --------------- 以上与 CONFIG_USE_CACHE_LOCK 无关

// In __OBJC2__, the runtimeLock is a mutex always held hence the cache lock is redundant and can be elided.
// 在 __OBJC2__ 中，runtimeLock 是 "始终保持" 的互斥锁，因此 cache lock 是多余的，可以忽略。(始终保持那里的意思是指 runtime Lock 始终是一个互斥锁吗？)

// If the runtime lock ever becomes a rwlock again, the cache lock would need to be used again.
// 如果 runtime lock 再次变为 rwlock，则需要再次使用 cache lock。

// 可在 objc-runtime-new.mm 中看到如下定义，表明 cacheUpdateLock 只是一个在旧版本中使用的锁
// #if CONFIG_USE_CACHE_LOCK
// mutex_t cacheUpdateLock;
// #endif

#if __OBJC2__

#define CONFIG_USE_CACHE_LOCK 0 // Objective-C 2.0 下是 0 

#else

#define CONFIG_USE_CACHE_LOCK 1 //  其他情况下是 1

#endif

// 对应与如下调用：
#if CONFIG_USE_CACHE_LOCK
    cacheUpdateLock.assertLocked();
#else
    // extern mutex_t runtimeLock; 在 objc-locks-new.h 中一个外联声明
    // mutex_t runtimeLock; 在 objc-runtime-new.mm Line 75 定义（类型是互斥锁） 
    
    // assertLocked 进行加锁，如果加锁失败是导致断言
    runtimeLock.assertLocked(); // 在 __OBJC2__ 下是使用 runtimeLock
#endif
```
#### `bytesForCapacity`
&emsp;`bucket_t` 散列数组的总的内存占用（以字节为单位）。
```c++
size_t cache_t::bytesForCapacity(uint32_t cap)
{   
    // 总容量乘以每个 bucket_t 的字节大小
    return sizeof(bucket_t) * cap;
}
```
#### `EMPTY_BYTES`
```c++
// EMPTY_BYTES includes space for a cache end marker bucket.
// EMPTY_BYTES 是包括 缓存结束标记 bucket 的空间。

// This end marker doesn't actually have the wrap-around pointer because cache scans always find an empty bucket before they might wrap.
// 这个结束标记实际上没有 wrap-around 指针，因为缓存扫描总是在可能进行换行之前找到一个空的bucket。(因为 buckets 的扩容机制)

// 1024 buckets is fairly common.
// 1024 bukcets 很常见。

// 一个 bucket_t 的实例变量的大小应该是 16 字节

#if DEBUG
    // Use a smaller size to exercise heap-allocated empty caches.
    // 使用较小的容量来执行堆分配的空缓存。
#   define EMPTY_BYTES ((8+1)*16)
#else
#   define EMPTY_BYTES ((1024+1)*16)
#endif
```
### `getBit/setBit/clearBit`
&emsp;针对 `__LP64__` 平台下的 `_flags` 的操作。[atomic_fetch_or/atomic_fetch_and](https://en.cppreference.com/w/cpp/atomic/atomic_fetch_or)
```c++
#if __LP64__
    bool getBit(uint16_t flags) const {
        return _flags & flags;
    }
    void setBit(uint16_t set) {
        __c11_atomic_fetch_or((_Atomic(uint16_t) *)&_flags, set, __ATOMIC_RELAXED);
    }
    void clearBit(uint16_t clear) {
        __c11_atomic_fetch_and((_Atomic(uint16_t) *)&_flags, ~clear, __ATOMIC_RELAXED);
    }
#endif
```
### `FAST_CACHE_ALLOC_MASK`
```c++
// Fast Alloc fields:
// This stores the word-aligned size of instances + "ALLOC_DELTA16", or 0 if the instance size doesn't fit.
// 它存储实例的字对齐大小 + "ALLOC_DELTA16"，如果实例大小不适合，则存储0。

// These bits occupy the same bits than in the instance size, so that the size can be extracted with a simple mask operation.
// 这些位占用与实例大小相同的位，因此可以通过简单的掩码操作提取大小。

// FAST_CACHE_ALLOC_MASK16 allows to extract the instance size rounded rounded up to the next 16 byte boundary, which is a fastpath for _objc_rootAllocWithZone()
// FAST_CACHE_ALLOC_MASK16 允许提取四舍五入到下一个 16 字节边界的实例大小，这是 _objc_rootAllocWithZone() 的快速路径

#define FAST_CACHE_ALLOC_MASK         0x1ff8 // 0b0001 1111 1111 1000
#define FAST_CACHE_ALLOC_MASK16       0x1ff0 // 0b0001 1111 1111 0000
#define FAST_CACHE_ALLOC_DELTA16      0x0008 // 0b0000 0000 0000 1000
```
### `hasFastInstanceSize/fastInstanceSize/setFastInstanceSize`
&emsp;在 `__LP64__` 平台下，`cache_t` 多了一个 `uint16_t _flags`。

```c++
#if FAST_CACHE_ALLOC_MASK
    bool hasFastInstanceSize(size_t extra) const
    {
        if (__builtin_constant_p(extra) && extra == 0) {
            return _flags & FAST_CACHE_ALLOC_MASK16;
        }
        return _flags & FAST_CACHE_ALLOC_MASK;
    }

    size_t fastInstanceSize(size_t extra) const
    {
        ASSERT(hasFastInstanceSize(extra));

        if (__builtin_constant_p(extra) && extra == 0) {
            return _flags & FAST_CACHE_ALLOC_MASK16;
        } else {
            size_t size = _flags & FAST_CACHE_ALLOC_MASK;
            // remove the FAST_CACHE_ALLOC_DELTA16 that was added
            // by setFastInstanceSize
            return align16(size + extra - FAST_CACHE_ALLOC_DELTA16);
        }
    }

    void setFastInstanceSize(size_t newSize)
    {
        // Set during realization or construction only. No locking needed.
        uint16_t newBits = _flags & ~FAST_CACHE_ALLOC_MASK;
        uint16_t sizeBits;

        // Adding FAST_CACHE_ALLOC_DELTA16 allows for FAST_CACHE_ALLOC_MASK16
        // to yield the proper 16byte aligned allocation size with a single mask
        sizeBits = word_align(newSize) + FAST_CACHE_ALLOC_DELTA16;
        sizeBits &= FAST_CACHE_ALLOC_MASK;
        if (newSize <= sizeBits) {
            newBits |= sizeBits;
        }
        _flags = newBits;
    }
#else
    // 不支持 FAST_CACHE_ALLOC_MASK 时
    bool hasFastInstanceSize(size_t extra) const {
        return false;
    }
    size_t fastInstanceSize(size_t extra) const {
        abort();
    }
    void setFastInstanceSize(size_t extra) {
        // nothing
    }
#endif
```
### `endMarker`
```c++

```


## 参考链接
**参考链接:🔗**
+ [黑幕背后的Autorelease](http://blog.sunnyxx.com/2014/10/15/behind-autorelease/)

