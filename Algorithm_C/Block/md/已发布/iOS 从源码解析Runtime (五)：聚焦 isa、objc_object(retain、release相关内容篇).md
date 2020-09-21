# iOS 从源码解析Runtime (五)：聚焦 isa、objc_object(retain、release相关内容篇)

> 经过上面两篇 `DenseMap` 和 `DenseMapBase` 的分析，相信对存储 `objc_object` 的引用计数所用的数据结构已经极其清楚了，那么接下来继续分析 `objc_object` 剩下的函数吧。

## `Optimized calls to retain/release methods`

### `id retain()`
```c++
// Equivalent to calling [this retain], with shortcuts if there is no override
// 等效于调用 [this retain]，如果没有重载此函数，则能快捷执行（快速执行）
inline id 
objc_object::retain()
{
    // Tagged Pointer 不参与引用计数管理，它的内存在栈区，由系统自行管理
    ASSERT(!isTaggedPointer());

    // 如果没有重载 retain/release 函数，则调用根类的 rootRetain() 函数
    //（hasCustomRR() 函数定义在 objc_class 中，等下面分析 objc_class 时再对其进行详细分析）
    if (fastpath(!ISA()->hasCustomRR())) {
        return rootRetain();
    }

    // 如果重载了 retain 函数，则以 objc_msgSend 调用重载的 retain 函数
    return ((id(*)(objc_object *, SEL))objc_msgSend)(this, @selector(retain));
}
```
### `void release()`
```c++
// Equivalent to calling [this release], with shortcuts if there is no override
// 等效于调用 [this release]，如果没有重载此函数，则能快捷执行（快速执行）
inline void
objc_object::release()
{
    // Tagged Pointer 不参与引用计数管理，它的内存在栈区，由系统自行管理
    ASSERT(!isTaggedPointer());
    
    // 如果没有重载 retain/release 函数，则调用根类的 rootRelease() 函数
    //（hasCustomRR() 函数定义在 objc_class 中，等下面分析 objc_class 时再对其进行详细分析）
    if (fastpath(!ISA()->hasCustomRR())) {
        rootRelease();
        return;
    }

    // 如果重载了 release 函数，则 objc_msgSend 调用 release 函数
    ((void(*)(objc_object *, SEL))objc_msgSend)(this, @selector(release));
}
```
### `id autorelease()`
```c++
// Equivalent to [this autorelease], with shortcuts if there is no override
// 等效于调用[this autorelease]，如果没有重载此函数，则能快捷执行（快速执行）
inline id 
objc_object::autorelease()
{
    // Tagged Pointer 不参与引用计数管理，它的内存在栈区，由系统自行管理
    ASSERT(!isTaggedPointer());
    
    // 如果没有重载 retain/release 函数，则调用根类的 rootAutorelease() 函数
    //（hasCustomRR() 函数定义在 objc_class 中，等下面分析 objc_class 时再对其进行详细分析）
    if (fastpath(!ISA()->hasCustomRR())) {
        return rootAutorelease();
    }
    
    // 如果重载了 autorelease 函数，则 objc_msgSend 调用 autorelease 函数
    return ((id(*)(objc_object *, SEL))objc_msgSend)(this, @selector(autorelease));
}
```

## `Implementations of retain/release methods`

### `id rootRetain()`
```c++
// Base retain implementation, ignoring overrides.
// Base retain 函数实现，忽略重载。

// This does not check isa.fast_rr; if there is an RR override then it was already called and it chose to call [super retain].
// 这不检查 isa.fast_rr; 如果存在 RR 重载，则它已经被调用，并选择调用 [super retain]。

// tryRetain=true is the -_tryRetain path.
// handleOverflow=false is the frameless fast path.
// handleOverflow=true is the framed slow path including overflow to side table

// The code is structured this way to prevent duplication.
// 以这种方式构造代码以防止重复。

ALWAYS_INLINE id 
objc_object::rootRetain()
{
    // tryRetain 和 handleOverflow 都传入的 false，不执行 -_tryRetain path. 
    // handleOverflow=false 处理 extra_rc++ 溢出的情况
    return rootRetain(false, false);
}
```
### `id rootRetain(bool tryRetain, bool handleOverflow)`
```c++
ALWAYS_INLINE id 
objc_object::rootRetain(bool tryRetain, bool handleOverflow)
{
    // 如果是 Tagged Pointer 则直接返回 this (Tagged Pointer 不参与引用计数管理，它的内存在栈区，由系统处理)
    if (isTaggedPointer()) return (id)this;
    
    // 临时变量，标记 SideTable 是否加锁
    bool sideTableLocked = false;
    // 临时变量，标记是否需要把引用计数迁移到 SideTable 中
    bool transcribeToSideTable = false;

    // 记录 objc_object 之前的 isa
    isa_t oldisa;
    // 记录 objc_object 修改后的 isa
    isa_t newisa;
    
    // 循环结束的条件是 slowpath(!StoreExclusive(&isa.bits, oldisa.bits, newisa.bits))
    // StoreExclusive 函数，如果 &isa.bits 与 oldisa.bits 的内存内容相同，则返回 true，否则返回 false。
    // 即 do-while 的循环条件是指，&isa.bits 与 oldisa.bits 内容不同，如果它们内容不同，则一直进行循环。
    
    // 如果 &isa.bits 与 oldisa.bits 的内容不相同，则把 newisa.bits 的内容复制给 &isa.bits。
    // return __c11_atomic_compare_exchange_weak((_Atomic(uintptr_t) *)dst, &oldvalue, value, __ATOMIC_RELAXED, __ATOMIC_RELAXED)
    
    // _Bool atomic_compare_exchange_weak( volatile A *obj, C* expected, C desired );
    // 定义于头文件 <stdatomic.h>
    // 原子地比较 obj 所指向对象的内存的内容与 expected 所指向的内存的内容。若它们相等，则以 desired 替换前者（进行读修改写操作）。
    // 否则，将 obj 所指向的实际内存内容加载到 *expected （进行加载操作）。
    
    do {
        // 默认不需要转移引用计数到 SideTable
        transcribeToSideTable = false;
        
        // x86_64 平台下
        // C atomic_load( const volatile A* obj );
        // 定义于头文件 <stdatomic.h>
        // 以原子方式加载并返回 obj 指向的原子变量的当前值。该操作是原子读取操作。
        //return  __c11_atomic_load((_Atomic(uintptr_t) *)src, __ATOMIC_RELAXED);
        
        // 以原子方式读取 &isa.bits。（&为取地址） 
        oldisa = LoadExclusive(&isa.bits);
        
        // 赋值给 newisa（第一次进来时 &isa.bits, oldisa.bits, newisa.bits 三者是完全相同的）
        newisa = oldisa;
        
        // 如果 newisa 不是优化的 isa (元类的 isa 是原始的 isa (Class cls))
        if (slowpath(!newisa.nonpointer)) {
            
            // 在 mac、arm64e 下不执行任何操作，只在 arm64 下执行 __builtin_arm_clrex();
            // 在 arm64 平台下，清除对 &isa.bits 的独占访问标记。
            ClearExclusive(&isa.bits);
            
            // 如果是元类则直接返回 this，元类对象都是全局唯一的，不参与引用计数管理
            if (rawISA()->isMetaClass()) return (id)this;
            
            // 如果不需要 tryRetain 并且当前 SideTable 处于加锁状态，则进行解锁
            if (!tryRetain && sideTableLocked) sidetable_unlock();
            
            if (tryRetain) {
            
              // 如果需要 tryRetain 则调用 sidetable_tryRetain 函数，并根据结果返回 this 或者 nil
              // 执行此行之前是不需要在当前函数对 SideTable 加锁的
              
              // sidetable_tryRetain 返回 false 表示对象已被标记为正在释放，
              // 所以此时再执行 retain 操作是没有意义的，所以返回 nil
              return sidetable_tryRetain() ? (id)this : nil;
              
            } else { 
              // 如果不需要 tryRetain 则调用 sidetable_retain()
              return sidetable_retain();
            }
        }
        
        // don't check newisa.fast_rr; we already called any RR overrides
        // 不要检查 newisa.fast_rr; 我们已经调用所有 RR 的重载。
        
        // 如果 tryRetain 为真并且 objc_object 被标记为正在释放 (newisa.deallocating)
        if (slowpath(tryRetain && newisa.deallocating)) {
            
            // 在 mac、arm64e 下不执行任何操作，只在 arm64 下执行 __builtin_arm_clrex();
            // 在 arm64 平台下，清除对 &isa.bits 的独占访问标记。
            ClearExclusive(&isa.bits);
            
            // 如果不需要 tryRetain 并且当前 SideTable 处于加锁状态，则进行解锁
            if (!tryRetain && sideTableLocked) sidetable_unlock();
            
            // 返回 nil
            return nil;
        }
        
        // 下面就是 isa 为 nonpointer，并且没有被标记为正在释放的对象
        uintptr_t carry;
        // bits extra_rc 自增
        
        // x86_64 平台下:
        // # define RC_ONE (1ULL<<56)
        // uintptr_t extra_rc   : 8
        // extra_rc 位于 56~64 位
        
        newisa.bits = addc(newisa.bits, RC_ONE, 0, &carry);  // extra_rc++
        
        // 如果 carry 为 true，表示要处理引用计数溢出的情况
        if (slowpath(carry)) {
            // newisa.extra_rc++ overflowed
            
            // 如果 handleOverflow 为 false，则调用 rootRetain_overflow(tryRetain)
            if (!handleOverflow) {
                
                // 在 mac、arm64e 下不执行任何操作，只在 arm64 下执行 __builtin_arm_clrex();
                // 在 arm64 平台下，清除对 &isa.bits 的独占访问标记。
                ClearExclusive(&isa.bits);
                
                return rootRetain_overflow(tryRetain);
            }
            
            // Leave half of the retain counts inline and prepare to
            // copy the other half to the side table.
            // 将 retain count 的一半留在 inline，并准备将另一半复制到 SideTable.
            
            // SideTable 加锁，接下来需要操作 refcnts 
            // 如果 tryRetain 为 false 并且 sideTableLocked 为 false，则 SideTable 加锁
            if (!tryRetain && !sideTableLocked) sidetable_lock();
            
            // 标记 SideTable 已经加锁
            // 整个函数只有这里把 sideTableLocked 置为 true
            sideTableLocked = true;
            // 标记需要把引用计数转移到 SideTable 中
            transcribeToSideTable = true;
            
            // x86_64 平台下：
            // uintptr_t extra_rc : 8
            // # define RC_HALF  (1ULL<<7) 二进制表示为: 0b 1000,0000
            // 把 extra_rc 总共 8 位，现在把它置为 RC_HALF，表示 extra_rc 溢出
            newisa.extra_rc = RC_HALF;
            
            // 把 has_sidetable_rc 标记为 true，表示 extra_rc 已经存不下该对象的引用计数，
            // 需要扩张到 SideTable 中
            newisa.has_sidetable_rc = true;
        }
    } while (slowpath(!StoreExclusive(&isa.bits, oldisa.bits, newisa.bits)));

    if (slowpath(transcribeToSideTable)) {
        // Copy the other half of the retain counts to the side table.
        // 复制 retain count 的另一半到 SideTable 中。
        sidetable_addExtraRC_nolock(RC_HALF);
    }

    // 如果 tryRetain 为 false 并且 sideTableLocked 为 true，则 SideTable 解锁
    if (slowpath(!tryRetain && sideTableLocked)) sidetable_unlock();
    
    // 返回 this 
    return (id)this;
}
```
#### `LoadExclusive、ClearExclusive、StoreExclusive、StoreReleaseExclusive`
&emsp;这四个函数主要用来进行原子读写(修改)操作。在 `Project Headers/objc-os.h` 的定义可看到在不同平台下它们的实现是不同的。首先是 `__arm64__ && !__arm64e__`，它针对的平台是从 `iPhone 5s` 开始到 `A12` 之前，已知 `A12` 开始是属于 `__arm64e__` 架构。

+ `ldrex` 可从内存加载数据，如果物理地址有共享 `TLB` 属性，则 `ldrex` 会将该物理地址标记为由当前处理器独占访问，并且会清除该处理器对其他任何物理地址的任何独占访问标记。否则，会标记：执行处理器已经标记了一个物理地址，但访问尚未完毕。清除标记时使用 `clrex` 指令。

+ `strex` 可在一定条件下向内存存储数据。条件具体如下：
  1. 如果物理地址没有共享 `TLB `属性，且执行处理器有一个已标记但尚未访问完毕的物理地址，那么将会进行存储，清除该标记，并在 `Rd` 中返回值 0。
  2. 如果物理地址没有共享 `TLB` 属性，且执行处理器也没有已标记但尚未访问完毕的物理地址，那么将不会进行存储，而会在 `Rd` 中返回值 1。
  3. 如果物理地址有共享 `TLB` 属性，且已被标记为由执行处理器独占访问，那么将进行存储，清除该标记，并在 `Rd` 中返回值 0。
  4. 如果物理地址有共享 `TLB` 属性，但没有标记为由执行处理器独占访问，那么不会进行存储，且会在 `Rd` 中返回值 1。

+ `stlex` ...

+ `clrex` 该指令的作用就是在独占访问结束时，清除 `cpu` 中本地处理器针对某块内存区域的独占访问标志（核中的某个状态寄存器），以防在未清除时的其他操作，对系统产生影响。

由于对 `ARM` 相关内容属于完全未知，具体内容可参考: 
[Linux内核同步机制之（一）：原子操作](http://www.wowotech.net/linux_kenrel/atomic.html) 
[arm架构的独占读写指令ldrex和strex的使用详解（原子操作和自旋锁实现的基本原理）](https://blog.csdn.net/duanlove/article/details/8212123) 
[【解答】arm架构的linux内核中，clrex指令的作用是什么，内核中什么时候才会用到？](https://blog.csdn.net/qianlong4526888/article/details/8536922)

```c++
#if __arm64__ && !__arm64e__

static ALWAYS_INLINE
uintptr_t
LoadExclusive(uintptr_t *src)
{
    return __builtin_arm_ldrex(src);
}

static ALWAYS_INLINE
bool
StoreExclusive(uintptr_t *dst, uintptr_t oldvalue __unused, uintptr_t value)
{
    return !__builtin_arm_strex(value, dst);
}

static ALWAYS_INLINE
bool
StoreReleaseExclusive(uintptr_t *dst, uintptr_t oldvalue __unused, uintptr_t value)
{
    return !__builtin_arm_stlex(value, dst);
}

static ALWAYS_INLINE
void
ClearExclusive(uintptr_t *dst __unused)
{
    __builtin_arm_clrex();
}

#else

static ALWAYS_INLINE
uintptr_t
LoadExclusive(uintptr_t *src)
{
    return __c11_atomic_load((_Atomic(uintptr_t) *)src, __ATOMIC_RELAXED);
}

static ALWAYS_INLINE
bool
StoreExclusive(uintptr_t *dst, uintptr_t oldvalue, uintptr_t value)
{
    return __c11_atomic_compare_exchange_weak((_Atomic(uintptr_t) *)dst, &oldvalue, value, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
}

static ALWAYS_INLINE
bool
StoreReleaseExclusive(uintptr_t *dst, uintptr_t oldvalue, uintptr_t value)
{
    return __c11_atomic_compare_exchange_weak((_Atomic(uintptr_t) *)dst, &oldvalue, value, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
}

static ALWAYS_INLINE
void
ClearExclusive(uintptr_t *dst __unused)
{
}

#endif
```
&emsp;在非 `arm64` 平台下，例如 `mac` 的 `x86_64` 架构下则都是基于 `C++11` 后推出的 `atomic` 操作来实现的。
+ `template< class T > T atomic_load( const std::atomic<T>* obj ) noexcept` 原子地获得 `obj` 所指向的值。

+ `template< class T > bool atomic_compare_exchange_weak( std::atomic<T>* obj, typename std::atomic<T>::value_type* expected, typename std::atomic<T>::value_type desired ) noexcept;` 原子地比较 `obj` 所指向对象与 `expected` 所指向对象的对象表示 (`C++20` 前)值表示 (`C++20` 起)，若它们逐位相等，则以 `desired` 替换前者（进行读修改写操作）。否则，将 `obj` 所指向对象的实际值加载到 `*expected` （进行加载操作）。复制如同以 `std::memcpy` 进行。

具体内容可参考:
[atomic_load](https://zh.cppreference.com/w/cpp/atomic/atomic_load)
[atomic_compare_exchange](https://zh.cppreference.com/w/cpp/atomic/atomic_compare_exchange)

### `sidetable_tryRetain`
```c++
bool
objc_object::sidetable_tryRetain()
{

// 如果当前平台支付 isa 优化
#if SUPPORT_NONPOINTER_ISA
    // 如果 isa 是优化的 isa 则直接执行断言，
    // 表明 sidetable_tryRetain 函数只针对原始 isa 调用（Class cls）
    ASSERT(!isa.nonpointer);
#endif

    // 从全局的 SideTalbes 中找到 this 所处的 SideTable
    SideTable& table = SideTables()[this];

    // NO SPINLOCK HERE
    // 这里没有 SPINLOCK
    
    // _objc_rootTryRetain() is called exclusively by _objc_loadWeak(), 
    // which already acquired the lock on our behalf.
    // _objc_rootTryRetain() 仅由 _objc_loadWeak() 独占调用，已经代表我们获得了锁。

    // fixme can't do this efficiently with os_lock_handoff_s
    // fixme os_lock_handoff_s 无法有效地做到这一点
    // if (table.slock == 0) {
    //     _objc_fatal("Do not call -_tryRetain.");
    // }

    bool result = true;
    
    // std::pair<DenseMapIterator<BucketT>, bool>
    // it 类型是: std::pair<DenseMapIterator<std::pair<Disguised<objc_object>, size_t>>, bool>
    auto it = table.refcnts.try_emplace(this, SIDE_TABLE_RC_ONE);
    // refcnt 是引用计数值，it.first 是迭代器，迭代器其实是 BucketT 指针，然后再 ->second 就正是 size_t 了。
    auto &refcnt = it.first->second;
    
    // it.second 为 true，表示 objc_object 的引用计数第一次放进 SideTable
    if (it.second) {
        // there was no entry
    } else if (refcnt & SIDE_TABLE_DEALLOCATING) { // 表示对象正在释放，result 置为 false
        result = false;
    } else if (! (refcnt & SIDE_TABLE_RC_PINNED)) { // refcnt 加 SIDE_TABLE_RC_ONE
        refcnt += SIDE_TABLE_RC_ONE;
    }
    
    // 返回 result
    return result;
}
```
### `sidetable_retain`
```c++
id
objc_object::sidetable_retain()
{
#if SUPPORT_NONPOINTER_ISA
    ASSERT(!isa.nonpointer);
#endif
    SideTable& table = SideTables()[this];
    
    table.lock();
    size_t& refcntStorage = table.refcnts[this];
    if (! (refcntStorage & SIDE_TABLE_RC_PINNED)) {
        refcntStorage += SIDE_TABLE_RC_ONE;
    }
    table.unlock();

    return (id)this;
}
```

## 参考链接
**参考链接:🔗**
+ [atomic_compare_exchange_weak](https://en.cppreference.com/w/c/atomic/atomic_compare_exchange)
+ [atomic_load, atomic_load_explicit](https://en.cppreference.com/w/c/atomic/atomic_load)
+ [从源码角度看苹果是如何实现 retainCount、retain 和 release 的](https://juejin.im/post/6844903847131889677)
+ [Perfect forwarding and universal references in C++](https://eli.thegreenplace.net/2014/perfect-forwarding-and-universal-references-in-c/)
+ [操作系统内存管理(思维导图详解)](https://blog.csdn.net/hguisu/article/details/5713164)
+ [内存管理](https://www.jianshu.com/p/8d742a44f0da)
