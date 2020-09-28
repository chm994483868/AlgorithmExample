# iOS 从源码解析Runtime (七)：聚焦 isa、objc_object(dealloc、autorelease返回值优化篇)

> 上一篇我们非常非常详尽的分析了自动释放池的相关的源码，这篇我们继续学习 `objc_object` 剩余的函数，目前只剩下`rootDealloc` 和 `rootAutorelease` 的实现流程，本篇前面部分首先把 `rootDealloc` 的调用流程讲解一下，然后后面的重点都放在 `Autorelease` 对函数返回值的优化上。

## `rootReleaseShouldDealloc`
```c++
ALWAYS_INLINE bool 
objc_object::rootReleaseShouldDealloc()
{
    // rootRelease 函数在 《iOS 从源码解析Runtime (五)：聚焦 isa、objc_object(retain、release、retaincount相关内容篇)》 中已经详细讲了
    return rootRelease(false, false);
}
```
## `rootDealloc`
&emsp;对象 `Dealloc` 的内部实现。如下条件全部为真的话，可以直接调用 `free` 进行快速释放内存。
1. 对象的 `isa` 是优化的 `isa`。
2. 对象不存在弱引用。
3. 对象没有关联对象。
4. 对象没有 `C++` 的析构的函数。
5. 对象的引用计数没有保存在 `SideTable` 中。

```c++
inline void
objc_object::rootDealloc()
{
    // 如果是 Tagged Pointer，则直接 return
    
    // 针对下面的 fixme:
    // 这里的大概 Tagged Pointer 的是不会执行到这里的，因为 dealloc 函数的调用是在 rootRelease 函数的最后通过
    // ((void(*)(objc_object *, SEL))objc_msgSend)(this, @selector(dealloc)) 来执行的，
    // 且正常情况下面我们不能主动调用 dealloc 函数，而 Tagged Pointer 调用 rootRelease 函数时会直接返回 false，
    // 所以很大概率下 Tagged Pointer 是走不到这里来的...
    
    if (isTaggedPointer()) return;  // fixme necessary? 是必要的吗？

    // 如下条件全部为真的话，可以直接调用 free 进行快速释放内存
    // 1. 对象的 isa 是优化的 isa。
    // 2. 对象不存在弱引用。
    // 3. 对象没有关联对象。
    // 4. 对象没有 C++ 的析构的函数。
    // 5. 对象的引用计数没有保存在 SideTable 中。
    if (fastpath(isa.nonpointer  &&  
                 !isa.weakly_referenced  &&  
                 !isa.has_assoc  &&  
                 !isa.has_cxx_dtor  &&  
                 !isa.has_sidetable_rc))
    {
        // 断言：1. 对象的引用计数没有保存在 SideTable 中 
        // 2. this 在 weak_table 中不存在弱引用(内部详细就是判断 weak_table 的 weak_entries 中是否有 this 存在) 
        
        // 刚刚点进 sidetable_present 函数时发现，此函数只是 DEBUG 模式下的函数，
        // 然后再把模式切换到 Release 模式下时编译运行，再 command 点击 assert， 看到宏定义是 #define assert(e) ((void)0)
        // 至此 看了这么久源码才发现，原来这个随处可见的断言只是针对 DEBUG 模式下使用的，我好菜呀 😭 
        
        assert(!sidetable_present());
        
        // 释放 this 的内存空间
        free(this);
    } 
    else {
        // 进入慢速释放的路径
        object_dispose((id)this);
    }
}
```
### `object_dispose`
```c++
id 
object_dispose(id obj)
{
    // 如果 obj 不存在，则直接返回 nil
    if (!obj) return nil;

    // 释放对象内存前的一些清理工作
    objc_destructInstance(obj); 
    
    // 释放对象内存
    free(obj);

    return nil;
}
```
### `objc_destructInstance`
```c++
/*
* objc_destructInstance
* Destroys an instance without freeing memory.
* 在对象释放内存之前清理对象相关的内容。

* Calls C++ destructors.
* 如果有 C++ 析构函数，则调用 C++ 析构函数。

* Calls ARC ivar cleanup.
* ARC ivar 的清理工作。（这个是指哪一部分清理工作？） 

* Removes associative references.
* 如果对象有关联对象的话，移除对象的关联对象。

* Returns `obj`. Does nothing if `obj` is nil.
* 返回 obj。 如果 obj 是 nil 的话则不执行任何操作。

*/
void *objc_destructInstance(id obj) 
{
    if (obj) {
        // Read all of the flags at once for performance.
        // 一次读取所有标志位以提高性能。
        
        // 是否有 C++ 析构函数 
        bool cxx = obj->hasCxxDtor();
        // 是否有关联对象
        bool assoc = obj->hasAssociatedObjects();

        // This order is important.
        // 下面的执行顺序很重要。
        
        // 如果有，则执行 C++ 析构函数
        if (cxx) object_cxxDestruct(obj);
        
        // 如果有，则移除关联对象。（具体实现可参考前面关联对象那篇文章）
        if (assoc) _object_remove_assocations(obj);
        
        // 清除对象的 Deallocating 状态，主要是对对象所处的 SideTable 进行清理工作 
        obj->clearDeallocating();
    }

    return obj;
}
```
### `clearDeallocating`
```c++
inline void 
objc_object::clearDeallocating()
{
    if (slowpath(!isa.nonpointer)) {
        // 对象的 isa 是非优化的 isa 
        
        // Slow path for raw pointer isa.
        // 针对 isa 是原始指针的对象的慢速执行路径
        
        // 1. 如果对象有弱引用，则调用 weak_clear_no_lock 函数执行清理工作，
        //    把对象的所有弱引用置为 nil，并对象的 weak_entry_t 移除（必要时还会缩小 weak_table_t 容量）
        // 2. 处理对象在 refcnts 中的 Bucket， ValueT 执行析构 KeyT 赋值为 TombstoneKey.
        sidetable_clearDeallocating();
    }
    else if (slowpath(isa.weakly_referenced  ||  isa.has_sidetable_rc)) {
        // Slow path for non-pointer isa with weak refs and/or side table data.
        // 同上 isa 是优化 isa 的对象的慢速执行路径
        
        // 包含的操作也基本完全相同
        clearDeallocating_slow();
    }

    // 可验证上面的 SideTable 的操作是否都完成了
    assert(!sidetable_present());
}
```
### `sidetable_clearDeallocating`
```c++
void 
objc_object::sidetable_clearDeallocating()
{
    // 从全局的 SideTables 中取出 SideTable
    SideTable& table = SideTables()[this];

    // clear any weak table items
    // 清除所有弱引用项（把弱引用置为 nil）
    
    // clear extra retain count and deallocating bit
    // 清除 SideTable 中的引用计数以及 deallocating 位
    
    // (fixme warn or abort if extra retain count == 0 ?)
    // (fixme 如果额外保留计数== 0，则发出警告或中止 ?)
    
    // 加锁
    table.lock();
    
    // 从 refcnts 中取出 this 对应的 BucketT（由 BucketT 构建的迭代器）
    RefcountMap::iterator it = table.refcnts.find(this);
    
    // 如果找到了
    if (it != table.refcnts.end()) {
    
        // ->second 取出 ValueT，最后一位是有无弱引用的标志位
        if (it->second & SIDE_TABLE_WEAKLY_REFERENCED) {
            // 具体实现可参考 weak 那几篇文章
            weak_clear_no_lock(&table.weak_table, (id)this);
        }
        
        // 把 this 对应的 BucketT "移除"（标记为移除）
        table.refcnts.erase(it);
    }
    table.unlock();
}
```
### `clearDeallocating_slow`
```c++
// Slow path of clearDeallocating() 
// for objects with nonpointer isa
// that were ever weakly referenced 
// or whose retain count ever overflowed to the side table.

NEVER_INLINE void
objc_object::clearDeallocating_slow()
{
    ASSERT(isa.nonpointer  &&  (isa.weakly_referenced || isa.has_sidetable_rc));

    SideTable& table = SideTables()[this];
    table.lock();
    
    // 同上，清理弱引用
    if (isa.weakly_referenced) {
        weak_clear_no_lock(&table.weak_table, (id)this);
    }
    
    // 同上，清理 refcnts 中的引用数据
    if (isa.has_sidetable_rc) {
        table.refcnts.erase(this);
    }
    table.unlock();
}
```
&emsp;至此 `rootDealloc` 函数涉及的全流程就分析完毕了，主要是在对象 `free` 之前做一些清理和收尾工作。

## `sidetable_lock`
```c++
void 
objc_object::sidetable_lock()
{
    // SideTable 加锁
    SideTable& table = SideTables()[this];
    table.lock();
}
```
## `sidetable_unlock`
```c++
void 
objc_object::sidetable_unlock()
{
    // SideTable 解锁
    SideTable& table = SideTables()[this];
    table.unlock();
}
```

## `rootAutorelease`
&emsp;看到 `if (prepareOptimizedReturn(ReturnAtPlus1)) return (id)this;` 原来并不是所有对象都会被放进自动释放池的（Tagged Pointer  除外），。
```c++
// Base autorelease implementation, ignoring overrides.
inline id 
objc_object::rootAutorelease()
{
    // 如果是 Tagged Pointer 则直接返回 (id)this
    if (isTaggedPointer()) return (id)this;
    
    // 如果 prepareOptimizedReturn(ReturnAtPlus1) 返回 true，则直接返回 (id)this
    if (prepareOptimizedReturn(ReturnAtPlus1)) return (id)this;
    
    // 否则正常调用 AutoreleasePoolPage::autorelease((id)this) 把 this 放进自动释放池
    // autorelease 可参考上篇
    return rootAutorelease2();
}
```
### `ReturnDisposition`
&emsp;`ReturnDisposition` 代表优化设置，`ReturnAtPlus0` 即为优化时引用计数加 `0`，`ReturnAtPlus1` 即为优化时引用计数加 `1`。
```c++
enum ReturnDisposition : bool {
    ReturnAtPlus0 = false, 
    ReturnAtPlus1 = true
};
```
### `RETURN_DISPOSITION_KEY`
```c++
// Thread keys reserved by libc for our use.
// libc 保留供我们使用的线程 key。

#if defined(__PTK_FRAMEWORK_OBJC_KEY0) // #define __PTK_FRAMEWORK_OBJC_KEY0    40

#   define SUPPORT_DIRECT_THREAD_KEYS 1 // 这个宏涉及到内容挺多的，暂时还无法展开

// 这三个 key 暂时还没有见到在哪里使用
#   define TLS_DIRECT_KEY        ((tls_key_t)__PTK_FRAMEWORK_OBJC_KEY0) // #define __PTK_FRAMEWORK_OBJC_KEY0    40
#   define SYNC_DATA_DIRECT_KEY  ((tls_key_t)__PTK_FRAMEWORK_OBJC_KEY1) // #define __PTK_FRAMEWORK_OBJC_KEY1    41
#   define SYNC_COUNT_DIRECT_KEY ((tls_key_t)__PTK_FRAMEWORK_OBJC_KEY2) // #define __PTK_FRAMEWORK_OBJC_KEY2    42

// 从 tls 中获取 hotPage 使用 
#   define AUTORELEASE_POOL_KEY  ((tls_key_t)__PTK_FRAMEWORK_OBJC_KEY3) // #define __PTK_FRAMEWORK_OBJC_KEY3    43

// 只要是非 TARGET_OS_WIN32 平台下都支持优化 autoreleased 返回值（优化方案是把返回值放在 tls 中，避免加入到 autoreleasePool 中）
# if SUPPORT_RETURN_AUTORELEASE

// 从 tls 中获取 disposition 
#   define RETURN_DISPOSITION_KEY ((tls_key_t)__PTK_FRAMEWORK_OBJC_KEY4) // #define __PTK_FRAMEWORK_OBJC_KEY4    44

# endif

#else

#   define SUPPORT_DIRECT_THREAD_KEYS 0 // 这个宏涉及到内容挺多的，暂时还无法展开

#endif

// Define SUPPORT_RETURN_AUTORELEASE to optimize autoreleased return values
// 定义 SUPPORT_RETURN_AUTORELEASE 以优化 autoreleased 返回值

#if TARGET_OS_WIN32

// TARGET_OS_WIN32 下不支持优化
#   define SUPPORT_RETURN_AUTORELEASE 0

#else

// 只要是非 TARGET_OS_WIN32 平台下都支持优化 autoreleased 返回值（优化方案是把返回值放在 tls 中，避免加入到 autoreleasePool 中）
#   define SUPPORT_RETURN_AUTORELEASE 1

#endif
```
### `getReturnDisposition/setReturnDisposition`
&emsp;这里又见到了 `tls_get_direct` 函数，已知它是运用 `Thread Local stroge (tls)` 机制在线程的存储空间里面根据 `key` 来获取对应的值，`static inline void tls_set_direct(tls_key_t k, void *value) ` 是根据 `key`，把 `value` 保存在 `tls` 中。（`tls` 涉及的内容太深了，这里先知悉其用法）

+ `getReturnDisposition` 函数是取得 `RETURN_DISPOSITION_KEY` 在 `tls` 中保存的值。
+ `setReturnDisposition` 函数是以 `RETURN_DISPOSITION_KEY` 为 `key`，把 `disposition` 保存在 `tls` 中。 
```c++
static ALWAYS_INLINE ReturnDisposition 
getReturnDisposition()
{
    return (ReturnDisposition)(uintptr_t)tls_get_direct(RETURN_DISPOSITION_KEY);
}
```
```c++
static ALWAYS_INLINE void 
setReturnDisposition(ReturnDisposition disposition)
{
    // 根据 RETURN_DISPOSITION_KEY 把传入的 disposition 保存在线程的存储空间内 
    tls_set_direct(RETURN_DISPOSITION_KEY, (void*)(uintptr_t)disposition);
}
```
### `__builtin_return_address`
1. `gcc` 默认不支持 `__builtin_return_address(LEVEL)` 的参数为非`0`。好像只支持参数为`0`。
2. `__builtin_return_address(0)` 的含义是，得到当前函数返回地址，即此函数被别的函数调用，然后此函数执行完毕后，返回，所谓返回地址就是那时候的地址。
3. `__builtin_return_address(1)` 的含义是，得到当前函数的调用者的返回地址。注意是调用者的返回地址，而不是函数起始地址。

### `callerAcceptsOptimizedReturn`
&emsp;这个函数针对不同的平台（`__x86__64__`/`__arm__`/`__arm64__`/`__i386`/`unknown`）有完全不同的实现。
```c++
/*
  Fast handling of return through Cocoa's +0 autoreleasing convention.
  The caller and callee cooperate to keep the returned object out of the autorelease pool and eliminate redundant retain/release pairs.
  调用方和被调用方合作将返回的对象保留在自动释放池之外，并消除多余的 retain/release 对。

  An optimized callee looks at the caller's instructions following the return. 
  一个优化的被调用方在返回后会查看调用方的指示。
  
  If the caller's instructions are also optimized then the callee skips all retain count operations: no autorelease, no retain/autorelease.
  如果调用方的指令也得到了优化，则被调用方将跳过所有保留计数操作：（autorelease retain/release）
  
  Instead it saves the result's current retain count (+0 or +1) in thread-local storage. 
  而是将结果的当前保留计数（+0 或 +1）保存在线程的存储空间中。（tls）
  
  If the caller does not look optimized then the callee performs autorelease or retain/autorelease as usual.
  如果调用方的指令看起来不能达到优化，则被调用方将照常执行 autorelease 或 retain/autorelease。
  
  An optimized caller looks at the thread-local storage. 
  一个优化的调用者会查看线程的本地存储空间。
  
  If the result is set then it performs any retain or release needed to change the result from the retain count left by the callee to the retain count desired by the caller.
  如果设置了结果，则它将执行将结果从被调用者留下的保留计数更改为调用者所需的保留计数所需的任何保留或释放操作。
  
  Otherwise the caller assumes the result is currently at +0 from an unoptimized callee and performs any retain needed for that case.
  否则，调用者会假设来自未优化的被调用者的结果当前为 +0，并执行该情况所需的任何 retain 操作。
  
  There are two optimized callees:
  这是两个优化的被调用者：
  
    objc_autoreleaseReturnValue
      result is currently +1. The unoptimized path autoreleases it.
      // + 1，未优化的执行路径是对它们执行 autorelease.
      
    objc_retainAutoreleaseReturnValue
      result is currently +0. The unoptimized path retains and autoreleases it.
      // + 0，未优化的执行路径是对它执行 retains 和 autorelease.

  There are two optimized callers:
  这是两个优化的调用者：
  
    objc_retainAutoreleasedReturnValue
      caller wants the value at +1. The unoptimized path retains it.
      // 调用者希望引用计数 +1。未优化路径对它执行 retain 操作。
      
    objc_unsafeClaimAutoreleasedReturnValue
      caller wants the value at +0 unsafely. The unoptimized path does nothing.
      // 调用者希望引用计数 +1 （不安全的）。未优化路径什么都不做。

  Example:

    Callee:
    被调用者：
      // compute ret at +1
      return objc_autoreleaseReturnValue(ret);
    
    Caller:
    调用者
      ret = callee();
      
      ret = objc_retainAutoreleasedReturnValue(ret);
      // use ret at +1 here

    Callee sees the optimized caller, sets TLS, and leaves the result at +1.
    
    Caller sees the TLS, clears it, and accepts the result at +1 as-is.

  The callee's recognition of the optimized caller is architecture-dependent.
  被调用方对优化的调用方的识别取决于体系结构。
  
  x86_64: Callee looks for `mov rax, rdi` followed by a call or 
    jump instruction to objc_retainAutoreleasedReturnValue or 
    objc_unsafeClaimAutoreleasedReturnValue. 
    
  i386:  Callee looks for a magic nop `movl %ebp, %ebp` (frame pointer register)
  
  armv7: Callee looks for a magic nop `mov r7, r7` (frame pointer register). 
  
  arm64: Callee looks for a magic nop `mov x29, x29` (frame pointer register). 

  Tagged pointer objects do participate in the optimized return scheme, 
 // 标记的指针对象确实参与了优化的返回方案
  
  because it saves message sends. They are not entered in the autorelease pool in the unoptimized case.
  // 因为它节省了消息发送。在未优化的情况下，它们不会放入到自动释放池中。
  
*/
# if __x86_64__

static ALWAYS_INLINE bool 
callerAcceptsOptimizedReturn(const void * const ra0)
{
    const uint8_t *ra1 = (const uint8_t *)ra0;
    const unaligned_uint16_t *ra2;
    const unaligned_uint32_t *ra4 = (const unaligned_uint32_t *)ra1;
    const void **sym;

#define PREFER_GOTPCREL 0
#if PREFER_GOTPCREL
    // 48 89 c7    movq  %rax,%rdi
    // ff 15       callq *symbol@GOTPCREL(%rip)
    if (*ra4 != 0xffc78948) {
        return false;
    }
    if (ra1[4] != 0x15) {
        return false;
    }
    ra1 += 3;
#else
    // 48 89 c7    movq  %rax,%rdi
    // e8          callq symbol
    if (*ra4 != 0xe8c78948) {
        return false;
    }
    ra1 += (long)*(const unaligned_int32_t *)(ra1 + 4) + 8l;
    ra2 = (const unaligned_uint16_t *)ra1;
    // ff 25       jmpq *symbol@DYLDMAGIC(%rip)
    if (*ra2 != 0x25ff) {
        return false;
    }
#endif
    ra1 += 6l + (long)*(const unaligned_int32_t *)(ra1 + 2);
    sym = (const void **)ra1;
    if (*sym != objc_retainAutoreleasedReturnValue  &&  
        *sym != objc_unsafeClaimAutoreleasedReturnValue) 
    {
        return false;
    }

    return true;
}

// __x86_64__
# elif __arm__

static ALWAYS_INLINE bool 
callerAcceptsOptimizedReturn(const void *ra)
{
    // if the low bit is set, we're returning to thumb mode
    if ((uintptr_t)ra & 1) {
        // 3f 46          mov r7, r7
        // we mask off the low bit via subtraction
        // 16-bit instructions are well-aligned
        if (*(uint16_t *)((uint8_t *)ra - 1) == 0x463f) {
            return true;
        }
    } else {
        // 07 70 a0 e1    mov r7, r7
        // 32-bit instructions may be only 16-bit aligned
        if (*(unaligned_uint32_t *)ra == 0xe1a07007) {
            return true;
        }
    }
    return false;
}

// __arm__
# elif __arm64__

static ALWAYS_INLINE bool 
callerAcceptsOptimizedReturn(const void *ra)
{
    // fd 03 1d aa    mov fp, fp
    // arm64 instructions are well-aligned
    if (*(uint32_t *)ra == 0xaa1d03fd) {
        return true;
    }
    return false;
}

// __arm64__
# elif __i386__

static ALWAYS_INLINE bool 
callerAcceptsOptimizedReturn(const void *ra)
{
    // 89 ed    movl %ebp, %ebp
    if (*(unaligned_uint16_t *)ra == 0xed89) {
        return true;
    }
    return false;
}

// __i386__
# else

#warning unknown architecture

static ALWAYS_INLINE bool 
callerAcceptsOptimizedReturn(const void *ra)
{
    return false;
}

// unknown architecture
# endif
```
### `prepareOptimizedReturn`
```c++
// Try to prepare for optimized return with the given disposition (+0 or +1).
// 尝试为优化做准备，返回给定的 disposition。（+0 或 +1）

// Returns true if the optimized path is successful.
// 如果这条优化的路径是成功的则返回 true。

// Otherwise the return value must be retained and/or autoreleased as usual.
// 否则，必须照常保留 和/或 自动释放返回值。

// 
static ALWAYS_INLINE bool 
prepareOptimizedReturn(ReturnDisposition disposition)
{
    // 这里从 tls 中取得 RETURN_DISPOSITION_KEY 的值必须是 false(ReturnAtPlus0)，
    // 要不然会执行断言
    ASSERT(getReturnDisposition() == ReturnAtPlus0);

    // __builtin_return_address(0) 得到当前函数的返回地址。
    
    // callerAcceptsOptimizedReturn 里面很多硬编码
    // __builtin_return_address(0) 返回值是 this 的地址吗 ？
    
    if (callerAcceptsOptimizedReturn(__builtin_return_address(0))) {
        
        // 如果 disposition 是 true (ReturnAtPlus1) 则保存在线程的存储空间内
        if (disposition) setReturnDisposition(disposition);
        
        // 返回 true
        return true;
    }

    return false;
}
```

&emsp;至此 `objc_object` 的代码就全部看完了。已经记不清花费了多少时间，但是整体对 `objc_object` 已经有了一个全面的认知，花多少时间都是超值的。⛽️⛽️

## 参考链接
**参考链接:🔗**
+ [__builtin_return_address(LEVEL)](https://blog.csdn.net/dayancn/article/details/18899157)
+ [返回值的 Autorelease 和 编译器优化](https://www.jianshu.com/p/aae7c3bd2191)
+ [操作系统内存管理(思维导图详解)](https://blog.csdn.net/hguisu/article/details/5713164)
