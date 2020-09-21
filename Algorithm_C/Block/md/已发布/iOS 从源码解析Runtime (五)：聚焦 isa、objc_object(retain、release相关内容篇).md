# iOS 从源码解析Runtime (五)：聚焦 isa、objc_object(retain、release相关内容篇)

> 经过上面两篇 `DenseMap` 和 `DenseMapBase` 的分析，相信对存储 `objc_object` 的引用计数所用的数据结构已经极其清楚了，那么接下来继续分析 `objc_object` 剩下的函数吧。

## `Optimized calls to retain/release methods`

### `id retain()`
```c++
// Equivalent to calling [this retain], with shortcuts if there is no override
// 等效于调用[this keep]，如果没有重载此函数，则能快捷执行（快速执行）。
inline id 
objc_object::retain()
{
    // Tagged Pointer 不执行 retain 函数。
    ASSERT(!isTaggedPointer());

    // 如果没有重载 retain/release 函数，（hasCustomRR 函数属于 objc_class，等下面分析 objc_class 时再详细分析）
    if (fastpath(!ISA()->hasCustomRR())) {
        return rootRetain();
    }

    // 如果重载了 retain 函数，则 objc_msgSend 调用 retain 函数
    return ((id(*)(objc_object *, SEL))objc_msgSend)(this, @selector(retain));
}
```
### `void release()`
```c++
// Equivalent to calling [this release], with shortcuts if there is no override
// 等效于调用[this release]，如果没有重载此函数，则能快捷执行（快速执行）。
inline void
objc_object::release()
{
    // Tagged Pointer 不执行 release 函数。
    ASSERT(!isTaggedPointer());
    
    // 如果没有重载 retain/release 函数，（hasCustomRR 函数属于 objc_class，等下面分析 objc_class 时再详细分析）
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
// 等效于调用[this autorelease]，如果没有重载此函数，则能快捷执行（快速执行）。
inline id 
objc_object::autorelease()
{
    // Tagged Pointer 不执行 autorelease 函数。
    ASSERT(!isTaggedPointer());
    
    // 如果没有重载 retain/release 函数，（hasCustomRR 函数属于 objc_class，等下面分析 objc_class 时再详细分析）
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
    // tryRetain 和 handleOverflow 都传入的 false，不执行  -_tryRetain path. 
    // handleOverflow=false 处理 extra_rc++ 溢出的情况
    return rootRetain(false, false);
}
```
### `id rootRetain(bool tryRetain, bool handleOverflow)`
```c++
ALWAYS_INLINE id 
objc_object::rootRetain(bool tryRetain, bool handleOverflow)
{
    // 如果是 Tagged Pointer 则直接返回 this
    if (isTaggedPointer()) return (id)this;

    bool sideTableLocked = false;
    bool transcribeToSideTable = false;

    isa_t oldisa;
    isa_t newisa;

    do {
        transcribeToSideTable = false;
        
        // 原子读取 isa.bits 
        oldisa = LoadExclusive(&isa.bits);
        // 赋值给 newisa
        newisa = oldisa;
        
        // 如果 newisa 不是优化的 isa (元类的 isa 是原始的 isa (Class cls))
        if (slowpath(!newisa.nonpointer)) {
            
            // 在 mac、arm64e 下不执行任何操作，只在 arm64 下执行 __builtin_arm_clrex();
            ClearExclusive(&isa.bits);
            
            // 如果是元类则直接返回 this
            if (rawISA()->isMetaClass()) return (id)this;
            
            // SideTable 解锁
            if (!tryRetain && sideTableLocked) sidetable_unlock();
            
            // 如果 tryRetain 为真，
            if (tryRetain) return sidetable_tryRetain() ? (id)this : nil;
            
            else return sidetable_retain();
        }
        
        // don't check newisa.fast_rr; we already called any RR overrides
        // 不要检查 newisa.fast_rr; 我们已经调用所有 RR 的重载。
        
        if (slowpath(tryRetain && newisa.deallocating)) {
            ClearExclusive(&isa.bits);
            
            // 解锁
            if (!tryRetain && sideTableLocked) sidetable_unlock();
            
            return nil;
        }
        
        // 
        uintptr_t carry;
        newisa.bits = addc(newisa.bits, RC_ONE, 0, &carry);  // extra_rc++

        if (slowpath(carry)) {
            // newisa.extra_rc++ overflowed
            if (!handleOverflow) {
                ClearExclusive(&isa.bits);
                return rootRetain_overflow(tryRetain);
            }
            
            // Leave half of the retain counts inline and prepare to copy the other half to the side table.
            // 将 retain count 的一半留在 inline，并准备将另一半复制到 SideTable.
            
            // SideTable 加锁，接下来需要操作 refcnts 
            if (!tryRetain && !sideTableLocked) sidetable_lock();
            
            // 标记 SideTable 已经加锁
            sideTableLocked = true;
            // 标记需要把引用计数转移到 SideTable 中
            transcribeToSideTable = true;
            
            // uintptr_t extra_rc        : 8
            // #   define RC_HALF  (1ULL<<7)
            // 把 extra_rc 置为一半 
            newisa.extra_rc = RC_HALF;
            // 把 has_sidetable_rc 标记为 true，表示 extra_rc 已经存不下该对象的引用计数，需要扩张到 SideTable 中
            newisa.has_sidetable_rc = true;
        }
    // __c11_atomic_compare_exchange_weak((_Atomic(uintptr_t) *)dst, &oldvalue, value, __ATOMIC_RELAXED, __ATOMIC_RELAXED)
    } while (slowpath(!StoreExclusive(&isa.bits, oldisa.bits, newisa.bits)));

    if (slowpath(transcribeToSideTable)) {
        // Copy the other half of the retain counts to the side table.
        // 复制 retain count 的一半到 SideTable 中。
        sidetable_addExtraRC_nolock(RC_HALF);
    }

    // 解锁
    if (slowpath(!tryRetain && sideTableLocked)) sidetable_unlock();
    
    // 返回 this 
    return (id)this;
}
```
### `sidetable_tryRetain`
```c++
bool
objc_object::sidetable_tryRetain()
{

// 如果当前平台支付 isa 优化
#if SUPPORT_NONPOINTER_ISA
    // 如果 isa 是优化的 isa 则直接执行断言.
    
    // 此函数只针对原始 isa 调用（Class cls）
    ASSERT(!isa.nonpointer);
#endif
    // 从全局的 SideTalbes 中找到 this 所处的 SideTable
    SideTable& table = SideTables()[this];

    // NO SPINLOCK HERE
    // 这里没有 SPINLOCK
    
    // _objc_rootTryRetain() is called exclusively by _objc_loadWeak(), 
    // which already acquired the lock on our behalf.
    // _objc_rootTryRetain() 仅由 _objc_loadWeak() 调用，已经代表我们获得了锁。

    // fixme can't do this efficiently with os_lock_handoff_s
    // fixme os_lock_handoff_s 无法有效地做到这一点
    // if (table.slock == 0) {
    //     _objc_fatal("Do not call -_tryRetain.");
    // }

    bool result = true;
    
    // std::pari<DenseMapIterator<BucketT>, bool>
    // std::pair<DenseMapIterator<std::pair<Disguised<objc_object>, size_t>>, bool>
    
    auto it = table.refcnts.try_emplace(this, SIDE_TABLE_RC_ONE);
    auto &refcnt = it.first->second;
    if (it.second) {
        // there was no entry
    } else if (refcnt & SIDE_TABLE_DEALLOCATING) {
        result = false;
    } else if (! (refcnt & SIDE_TABLE_RC_PINNED)) {
        refcnt += SIDE_TABLE_RC_ONE;
    }
    
    return result;
}
```


## 参考链接
**参考链接:🔗**
+ [Objective-C 1.0 中类与对象的定义](https://kangzubin.com/objc1.0-class-object/)
+ [苹果架构分类](https://www.jianshu.com/p/63420dfb217c)
+ [Object-C 中的Selector 概念](https://www.cnblogs.com/geek6/p/4106199.html)
+ [C语言中文开发手册:atomic_load/atomic_compare_exchange_weak](https://www.php.cn/manual/view/34155.html)
+ [操作系统内存管理(思维导图详解)](https://blog.csdn.net/hguisu/article/details/5713164)
+ [内存管理](https://www.jianshu.com/p/8d742a44f0da)
