#  iOS底层Class结构之Cache_t
在之前的文章中探索过类的结构的相关信息，其中主要说了 superclss、isa 等，但是还有一个 cache_t，这个也是 Class 中极其重要的一部分。

在 objc4-779 之后的版本，cache_t 结构发生了变化，但是原理是一样的。
> （看到很多博主的的都是对旧版的解析，只有这个博主是用的最新的，所以一直在看博主的文章，本文中用的代码都是 objc4-781 中的，是目前苹果最新的代码。）

cache_t  是用来缓存 class 调用过的实例方法。但是其本身的工作机制和我们想象的是不一样的。
> 方法缓存原理：
> 1. 缓存第一个实例方法的时候，初始化一个 mask 容量为 3 的表，往里面存方法信息并记录缓存数量为 1.
> 2. 当有方法需要缓存的时候，如果需要缓存的方法数量 （当前缓存数量 + 1）<= 当前容量 + 1 的 3/4, 直接缓存并记录缓存数量 + 1
> 3. 当有方法需要缓存的时候，如果需要缓存的方法数量（当前缓存数量 + 1）> 当前容量的 3/4, 按照之前的容量 x ( (x + 1) * 2 - 1) 重新分配存储容量，并清除之前缓存的方法信息，然后再存储，并记录缓存数量为 1.（扩容以后并不会把以前的数据复制到新内存中，而是直接舍弃）

> mask 的机制
> 1.  初始化为 4 - 1
> 2. 当需要存储数量 > 当前容量 + 1 的 3/4，重新分配容量 （（当前容量 + 1））* 2 - 1

**看 cache_t 源码结构之前延展学习 C++ 原子操作 std::atomic<T> :**
## std::atomic<T>
std::atomic<T> 模版类可以使对象操作为原子操作，避免多线程竞争问题。
std::atomic<T> 是模版类，一个模版类型为 T 的原子对象中封装了一个类型为 T 的值。
`template <class T> struct atomic;`
原子类型对象的主要特点就是从不同线程访问不会导致数据竞争（data race）。因此从不同线程访问某个原子对象是良性（well-defined）行为，而通常对于非原子类型而言，并发访问某个对象（如果不做任何同步操作）会导致未定义（undefined）行为发生。


## objc4-781 版本
**cache_t 结构**
```
struct cache_t {
#if CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_OUTLINED
    /// 方法缓存数组（以散列表的形式进行存储）
    explicit_atomic<struct bucket_t *> _buckets; // 存储方法信息 8 
    /// 容量的临界值 （散列表长度 - 1）
    explicit_atomic<mask_t> _mask; // 留了多少个缓存位置 4
#elif CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_HIGH_16
    explicit_atomic<uintptr_t> _maskAndBuckets;
    mask_t _mask_unused;
    
    // How much the mask is shifted by.
    static constexpr uintptr_t maskShift = 48;
    
    // Additional bits after the mask which must be zero. msgSend
    // takes advantage of these additional bits to construct the value
    // `mask << 4` from `_maskAndBuckets` in a single instruction.
    static constexpr uintptr_t maskZeroBits = 4;
    
    // The largest mask value we can store.
    static constexpr uintptr_t maxMask = ((uintptr_t)1 << (64 - maskShift)) - 1;
    
    // The mask applied to `_maskAndBuckets` to retrieve the buckets pointer.
    static constexpr uintptr_t bucketsMask = ((uintptr_t)1 << (maskShift - maskZeroBits)) - 1;
    
    // Ensure we have enough bits for the buckets pointer.
    static_assert(bucketsMask >= MACH_VM_MAX_ADDRESS, "Bucket field doesn't have enough bits for arbitrary pointers.");
#elif CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_LOW_4
    // _maskAndBuckets stores the mask shift in the low 4 bits, and
    // the buckets pointer in the remainder of the value. The mask
    // shift is the value where (0xffff >> shift) produces the correct
    // mask. This is equal to 16 - log2(cache_size).
    explicit_atomic<uintptr_t> _maskAndBuckets;
    mask_t _mask_unused;

    static constexpr uintptr_t maskBits = 4;
    static constexpr uintptr_t maskMask = (1 << maskBits) - 1;
    static constexpr uintptr_t bucketsMask = ~maskMask;
#else
#error Unknown cache mask storage type.
#endif
    
#if __LP64__
    uint16_t _flags; // 2
#endif
    /// 表示已经缓存的方法的数量
    uint16_t _occupied; // 已缓存的数量 2

public:
    static bucket_t *emptyBuckets();
    
    struct bucket_t *buckets();
    mask_t mask();
    mask_t occupied();
    void incrementOccupied();
    void setBucketsAndMask(struct bucket_t *newBuckets, mask_t newMask);
    void initializeToEmpty();

    unsigned capacity();
    bool isConstantEmptyCache();
    bool canBeFreed();

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

    static size_t bytesForCapacity(uint32_t cap);
    static struct bucket_t * endMarker(struct bucket_t *b, uint32_t cap);

    void reallocate(mask_t oldCapacity, mask_t newCapacity, bool freeOld);
    void insert(Class cls, SEL sel, IMP imp, id receiver);

    static void bad_cache(id receiver, SEL sel, Class isa) __attribute__((noreturn, cold));
};
```
```
struct bucket_t {
private:
    // IMP-first is better for arm64e ptrauth and no worse for arm64.
    // SEL-first is better for armv7* and i386 and x86_64.
#if __arm64__
    explicit_atomic<uintptr_t> _imp; // 获取方法实现
    explicit_atomic<SEL> _sel; // 以方法名为 key，选择子
#else
    explicit_atomic<SEL> _sel;
    explicit_atomic<uintptr_t> _imp;
#endif
....
};
```
定义 LGPerson 类，并实现几个方法并调用：
```
// .h
@interface LGPerson : NSObject

- (void)instanceMethod1;
- (void)instanceMethod2;
- (void)instanceMethod3;
- (void)instanceMethod4;
- (void)instanceMethod5;
- (void)instanceMethod6;
- (void)instanceMethod7;

@end

// main.m 中
LGPerson *person = [LGPerson alloc];
LGPerson *p = [person init]; // 在此处打断点

[p instanceMethod1];
[p instanceMethod2];
[p instanceMethod3];
[p instanceMethod4];
[p instanceMethod5];
[p instanceMethod6];
[p instanceMethod7];
```
通过控制台打印:
```
// 打印类信息
(lldb) p [person class]
(Class) $0 = LGPerson

// 拿到 cache 信息
(lldb) x/4gx $0
0x1000021d8: 0x00000001000021b0 （isa） 0x00000001003ee140 (superclass)
0x1000021e8: 0x0000000100677860 0x0002801000000003 (cache_t)
(lldb) p (cache_t *)0x1000021e8
(cache_t *) $1 = 0x00000001000021e8

// 查看 cache 具体有什么东西
(lldb) p *$1
(cache_t) $2 = {
  _buckets = {
    std::__1::atomic<bucket_t *> = 0x0000000100677860 {
      _sel = {
        std::__1::atomic<objc_selector *> = 0x00007fff70893e54
      }
      _imp = {
        std::__1::atomic<unsigned long> = 4041432
      }
    }
  }
  _mask = {
    std::__1::atomic<unsigned int> = 3
  }
  _flags = 32784
  _occupied = 2
}
```
这里就奇怪了，明明没有调用任何实例方法，为什么已经缓存了两个了呢？
继续打印 _buckets 信息：
```
(lldb) p (bucket_t *)$1.buckets()
(bucket_t *) $3 = 0x0000000100677860
  Fix-it applied, fixed expression was: 
    (bucket_t *)$1->buckets()
(lldb) p (bucket_t *)$1->buckets()
(bucket_t *) $4 = 0x0000000100677860
(lldb) p $4[0]
(bucket_t) $5 = {
  _sel = {
    std::__1::atomic<objc_selector *> = 0x00007fff70893e54
  }
  _imp = {
    std::__1::atomic<unsigned long> = 4041432
  }
}
(lldb) p $4[1]
(bucket_t) $6 = {
  _sel = {
    std::__1::atomic<objc_selector *> = 0x0000000000000000
  }
  _imp = {
    std::__1::atomic<unsigned long> = 0
  }
}
(lldb) p $4[2]
(bucket_t) $7 = {
  _sel = {
    std::__1::atomic<objc_selector *> = 0x0000000000000000
  }
  _imp = {
    std::__1::atomic<unsigned long> = 0
  }
}
(lldb) 
```
> 只看到 `$4[0]` 发现缓存的方法，但是无法看出究竟是缓存的什么方法。

在初始化 LGPerson 之前，通过 `Class cls = NSClassFromString(@"LGPerson");` 方式得到 cls，打印 cls 里面的缓存信息，发现 `mask = 0, _occupied = 0`，然后 `LGPerson *person = [LGPerson alloc];` 初始化之后，再打印，发现 `mask = 3, _occupied = 2`，也就是说在 alloc 过程中，进行了缓存操作。

断点往下走，调用了一个实例方法 `[person init]`，调用 `init` 之后，再打印 `cache` 信息，看到 `init` 被缓存到了 `LGPerson` 中，这也证明了 `即使是父类的方法，也会缓存在本类中`。

```
// 任何函数未执行时，只调用 NSClassFromString(@"LGPerson")
Class cls = NSClassFromString(@"LGPerson");
// 打印:
(cache_t) $3 = {
  _buckets = {
    std::__1::atomic<bucket_t *> = 0x00000001003e8490 {
      _sel = {
        std::__1::atomic<objc_selector *> = 0x0000000000000000
      }
      _imp = {
        std::__1::atomic<unsigned long> = 0
      }
    }
  }
  _mask = {
    std::__1::atomic<unsigned int> = 0 // 临界容量 0
  }
  _flags = 16
  _occupied = 0 // 已占用 0 
}

// 执行到 [persont init] 处
// 命令列表
p [person class]
x/4gx $0
p (cache_t *)0x1000021f0
p *$1

...
_mask = {
  std::__1::atomic<unsigned int> = 3 // 临界容量是 3
}
_flags = 32784
_occupied = 2 // 已占用是 2
...
// 执行到完 init 后
...
_mask = {
  std::__1::atomic<unsigned int> = 3 // 临界容量是 3
}
_flags = 32784
_occupied = 3 // 已占用是 3
...

// 使用上面 cls 
// 打印 _mask = 3 _occupied = 3
```

| 调用过实例方法数量 |0|1|2|3|4|5|6|7|8|
|---|---|---|---|---|---|---|---|---|---|
|_mask|3|3|7|---|---|---|---|---|---|
|_occupied|2|3|1|---|---|---|---|---|---|

**参考链接:🔗**
[C++11 并发指南六( <atomic> 类型详解二 std::atomic )](https://www.cnblogs.com/haippy/p/3301408.html)





