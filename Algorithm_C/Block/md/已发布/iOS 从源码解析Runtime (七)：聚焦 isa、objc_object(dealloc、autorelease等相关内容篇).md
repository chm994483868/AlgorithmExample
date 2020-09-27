# iOS 从源码解析Runtime (七)：聚焦 isa、objc_object(dealloc、autorelease等相关内容篇)

> 上一篇我们非常非常详尽的分析了自动释放池的相关的源码，这篇我们继续学习 `objc_object` 剩余的函数。


## `rootAutorelease`
&emsp;看到 `if (prepareOptimizedReturn(ReturnAtPlus1)) return (id)this;` 原来并不是所有对象都会被放进自动释放池的（Tagged Pointer  除外）。
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
&emsp;如果硬翻的话，可以理解为 “返回布置/安排”，而枚举的两个值分别是 “加 0”/“加 1”。
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

#   define SUPPORT_DIRECT_THREAD_KEYS 1 

// 这三个 key 还没有见到在哪里使用
#   define TLS_DIRECT_KEY        ((tls_key_t)__PTK_FRAMEWORK_OBJC_KEY0) // #define __PTK_FRAMEWORK_OBJC_KEY0    40
#   define SYNC_DATA_DIRECT_KEY  ((tls_key_t)__PTK_FRAMEWORK_OBJC_KEY1) // #define __PTK_FRAMEWORK_OBJC_KEY1    41
#   define SYNC_COUNT_DIRECT_KEY ((tls_key_t)__PTK_FRAMEWORK_OBJC_KEY2) // #define __PTK_FRAMEWORK_OBJC_KEY2    42

// 从 tls 中获取 hotPage 使用 
#   define AUTORELEASE_POOL_KEY  ((tls_key_t)__PTK_FRAMEWORK_OBJC_KEY3) // #define __PTK_FRAMEWORK_OBJC_KEY3    43

# if SUPPORT_RETURN_AUTORELEASE // 只要是非 TARGET_OS_WIN32 平台下都是支持对 autoreleased 返回值进行优化的

// 从 tls 中获取 disposition 
#   define RETURN_DISPOSITION_KEY ((tls_key_t)__PTK_FRAMEWORK_OBJC_KEY4) // #define __PTK_FRAMEWORK_OBJC_KEY4    44

# endif

#else

#   define SUPPORT_DIRECT_THREAD_KEYS 0

#endif

// Define SUPPORT_RETURN_AUTORELEASE to optimize autoreleased return values
// 定义 SUPPORT_RETURN_AUTORELEASE 以优化 autoreleased 的返回值

#if TARGET_OS_WIN32
#   define SUPPORT_RETURN_AUTORELEASE 0
#else
// 只要是非 TARGET_OS_WIN32 平台下都是支持对 autoreleased 返回值进行优化的
#   define SUPPORT_RETURN_AUTORELEASE 1
#endif
```
### `getReturnDisposition/setReturnDisposition`
&emsp;这里又见到了 `tls_get_direct` 函数，已知它是运用 `Thread Local stroge` 机制在线程的存储空间里面根据 `key` 来保存对应的值。在 `getReturnDisposition` 函数中是取得 `RETURN_DISPOSITION_KEY` `key` 在 `tls` 中保存的值。
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
    
    if (callerAcceptsOptimizedReturn(__builtin_return_address(0))) {
        
        // 如果 disposition 是 true (ReturnAtPlus1) 则保存在线程的存储空间内
        if (disposition) setReturnDisposition(disposition);
        
        // 返回 true
        return true;
    }

    return false;
}
```
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
&emsp;对象 `Dealloc` 的内部实现。
```c++
inline void
objc_object::rootDealloc()
{
    // 如果是 Tagged Pointer，则直接 return
    if (isTaggedPointer()) return;  // fixme necessary? 是必要的吗？

    // 遵循如下条件是，可以进行快速释放内存
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
        // 断言：1. 对象的引用计数没有保存在 SideTable 中 2. this 在 weak_table 中不存在弱引用 
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
    if (!obj) return nil;

    objc_destructInstance(obj);    
    free(obj);

    return nil;
}
```
### `objc_destructInstance`
```c++
/*
* objc_destructInstance
* Destroys an instance without freeing memory.
* 在不释放内存的情况下销毁实例。

* Calls C++ destructors.
* Calls ARC ivar cleanup.
* Removes associative references.
* Returns `obj`. Does nothing if `obj` is nil.
*/
void *objc_destructInstance(id obj) 
{
    if (obj) {
        // Read all of the flags at once for performance.
        bool cxx = obj->hasCxxDtor();
        bool assoc = obj->hasAssociatedObjects();

        // This order is important.
        if (cxx) object_cxxDestruct(obj);
        if (assoc) _object_remove_assocations(obj);
        obj->clearDeallocating();
    }

    return obj;
}
```

## 参考链接
**参考链接:🔗**
+ [__builtin_return_address(LEVEL)](https://blog.csdn.net/dayancn/article/details/18899157)
+ [操作系统内存管理(思维导图详解)](https://blog.csdn.net/hguisu/article/details/5713164)
