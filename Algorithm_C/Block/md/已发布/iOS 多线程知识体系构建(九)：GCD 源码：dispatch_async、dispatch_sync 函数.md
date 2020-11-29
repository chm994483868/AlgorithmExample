# iOS 多线程知识体系构建(九)：GCD 源码：dispatch_async、dispatch_sync 函数

> &emsp;上一篇我们学习了队列的创建，本篇我们开始学习 GCD  中的相关函数，首先从我们用的最多的 dispatch_async 和 dispatch_sync 开始。

&emsp;GCD 函数阅读过程中会涉及多个由大量宏定义组成的结构体的定义，需要一步一步进行宏展开才能更好的里面代码。

## dispatch_async
&emsp;当我们向队列提交任务时，无论 block 还是 function 形式，最终都会被封装为 `dispatch_continuation_s`，所以可以把它理解为描述任务内容的结构体，`dispatch_async` 函数内部会首先创建 `dispatch_continuation_s` 结构体。

&emsp;首先我们要知道一点不管是用 `dispatch_async` 向队列中异步提交 block，还是用 `dispatch_async_f` 向队列中异步提交函数，都会把提交的任务包装成 `dispatch_continuation_s`，而在 `dispatch_continuation_s` 结构体中是使用一个函数指针（`dc_func`）来存储要执行的任务的，当提交的是 block 任务时 `dispatch_continuation_s` 内部存储的是 block 结构体定义的函数，而不是 block 本身。
```c++
void
dispatch_async(dispatch_queue_t dq, dispatch_block_t work)
{
    // 取得一个 dispatch_continuation_s (从缓存中获取或者新建)
    dispatch_continuation_t dc = _dispatch_continuation_alloc();
    
    // DC_FLAG_CONSUME 标记 dc 是设置在异步中的源程序（即表明 dispatch_continuation_s 是一个在异步中执行的任务）
    // #define DC_FLAG_CONSUME   0x004ul
    uintptr_t dc_flags = DC_FLAG_CONSUME;
    
    // dispatch_qos_t 是一个 uint32_t 类型别名
    dispatch_qos_t qos;
    
    qos = _dispatch_continuation_init(dc, dq, work, 0, dc_flags);
    
    _dispatch_continuation_async(dq, dc, qos, dc->dc_flags);
}
```
&emsp;`dispatch_continuation_s` 定义中内部使用的宏定义展开如下：
```c++
typedef struct dispatch_continuation_s {
    union {
        const void *do_vtable;
        uintptr_t dc_flags;
    };
    
    union {
        pthread_priority_t dc_priority;
        int dc_cache_cnt;
        uintptr_t dc_pad;
    };
    
    struct dispatch_continuation_s *volatile do_next; // 下一个任务
    struct voucher_s *dc_voucher;
    
    // typedef void (*dispatch_function_t)(void *_Nullable);
    
    dispatch_function_t dc_func; // 要执行的函数指针
    void *dc_ctxt; // 方法的上下文
    void *dc_data; // 相关数据
    void *dc_other; // 其它信息 
} *dispatch_continuation_t;
```
### _dispatch_continuation_alloc
&emsp;`_dispatch_continuation_alloc` 函数内部首先调用 `_dispatch_continuation_alloc_cacheonly` 函数从缓存中找 `dispatch_continuation_t`，如果找不到则调用 `_dispatch_continuation_alloc_from_heap` 函数在堆区新建一个 `dispatch_continuation_s`。
```c++
DISPATCH_ALWAYS_INLINE
static inline dispatch_continuation_t
_dispatch_continuation_alloc(void)
{
    dispatch_continuation_t dc =
            _dispatch_continuation_alloc_cacheonly();
    // 如果缓存中不存在则在堆区新建 dispatch_continuation_s
    if (unlikely(!dc)) {
        return _dispatch_continuation_alloc_from_heap();
    }
    return dc;
}
```
### _dispatch_continuation_alloc_cacheonly
&emsp;`_dispatch_continuation_alloc_cacheonly` 函数内部调用 `_dispatch_thread_getspecific` 函数从当前线程获取根据 `dispatch_cache_key` 作为 key 保存的 `dispatch_continuation_t` 赋值给 `dc`，然后把 `dc` 的 `do_next` 作为新的 value 调用 `_dispatch_thread_setspecific` 函数保存在当前线程的存储空间中。（即更新当前缓存中可用的 `dispatch_continuation_t`）
```c++
DISPATCH_ALWAYS_INLINE
static inline dispatch_continuation_t
_dispatch_continuation_alloc_cacheonly(void)
{
    dispatch_continuation_t dc = (dispatch_continuation_t)
            _dispatch_thread_getspecific(dispatch_cache_key);

    // 更新 dispatch_cache_key 作为 key 保存在线程存储空间中的值
    if (likely(dc)) {
        _dispatch_thread_setspecific(dispatch_cache_key, dc->do_next);
    }
    return dc;
}

#define _dispatch_thread_getspecific(key) \
    (_dispatch_get_tsd_base()->key)
#define _dispatch_thread_setspecific(key, value) \
    (void)(_dispatch_get_tsd_base()->key = (value))
```
### DC_FLAG_CONSUME
&emsp;continuation resources 在运行时释放，表示 dispatch_continuation_s 是设置在异步或 non event_handler 源处理程序。
```c++
#define DC_FLAG_CONSUME   0x004ul
```
### DC_FLAG_BLOCK
&emsp;continuation function 是一个 block。
```c++
#define DC_FLAG_BLOCK   0x010ul
```
### DC_FLAG_ALLOCATED
&emsp;bit 用于确保分配的 continuations 的  dc_flags 永远不会为 0。
```c++
#define DC_FLAG_ALLOCATED   0x100ul
```
### dispatch_qos_t
&emsp;`dispatch_qos_t` 是一个 `uint32_t` 类型别名。
```c++
typedef uint32_t dispatch_qos_t;
```
### _dispatch_continuation_init
&emsp;`_dispatch_continuation_init` 函数是根据入参对 `dc` 进行初始化。
```c++
DISPATCH_ALWAYS_INLINE
static inline dispatch_qos_t
_dispatch_continuation_init(dispatch_continuation_t dc,
        dispatch_queue_class_t dqu, dispatch_block_t work,
        dispatch_block_flags_t flags, uintptr_t dc_flags)
{
    // 把入参 block 复制到堆区（内部是调用了 Block_copy 函数）
    void *ctxt = _dispatch_Block_copy(work);
    
    // 入参 dc_flags 是 DC_FLAG_CONSUME（0x004ul）
    // #define DC_FLAG_BLOCK   0x010ul
    // #define DC_FLAG_ALLOCATED   0x100ul
    
    // 即 dc_flags 等于 0x114ul
    dc_flags |= DC_FLAG_BLOCK | DC_FLAG_ALLOCATED;
    
    // 判断 work block 是否有私有数据，
    if (unlikely(_dispatch_block_has_private_data(work))) {
        dc->dc_flags = dc_flags;
        dc->dc_ctxt = ctxt;
        // will initialize all fields but requires dc_flags & dc_ctxt to be set
        return _dispatch_continuation_init_slow(dc, dqu, flags);
    }

    dispatch_function_t func = _dispatch_Block_invoke(work);
    if (dc_flags & DC_FLAG_CONSUME) {
        func = _dispatch_call_block_and_release;
    }
    return _dispatch_continuation_init_f(dc, dqu, ctxt, func, flags, dc_flags);
}
```
### _dispatch_Block_copy
&emsp;`_dispatch_Block_copy` 内部调用 `Block_copy` 函数，把栈区 block 复制到堆区，或者堆区 block 引用加 1。
```c++
void *
(_dispatch_Block_copy)(void *db)
{
    dispatch_block_t rval;

    if (likely(db)) {
        while (unlikely(!(rval = Block_copy(db)))) {
            // 保证 block 复制成功
            _dispatch_temporary_resource_shortage();
        }
        return rval;
    }
    DISPATCH_CLIENT_CRASH(0, "NULL was passed where a block should have been");
}

DISPATCH_NOINLINE
void
_dispatch_temporary_resource_shortage(void)
{
    sleep(1);
    __asm__ __volatile__("");  // prevent tailcall
}
```
### _dispatch_Block_invoke
&emsp;如果熟悉 block 内部的构造的话可知 `invoke` 是一个指向 block 要执行的函数的函数指针。（在我们定义 block 时，用 { } 扩起的内容会构成一个完整的函数表达式，它就是 block 要执行的函数。）

&emsp;`_dispatch_Block_invoke` 是一个宏定义，即取得 block 的函数指针。
```c++
typedef void(*BlockInvokeFunction)(void *, ...);
struct Block_layout {
    ...
    // 函数指针，指向 block 要执行的函数（即 block 定义中花括号中的表达式）
    BlockInvokeFunction invoke;
    ...
};

#define _dispatch_Block_invoke(bb) \
        ((dispatch_function_t)((struct Block_layout *)bb)->invoke)
```
### _dispatch_block_has_private_data
&emsp;
```c++

```




## 参考链接
**参考链接:🔗**
+ [libdispatch苹果源码](https://opensource.apple.com/tarballs/libdispatch/)
+ [GCD源码分析1 —— 开篇](http://lingyuncxb.com/2018/01/31/GCD源码分析1%20——%20开篇/)
+ [扒了扒libdispatch源码](http://joeleee.github.io/2017/02/21/005.扒了扒libdispatch源码/)
+ [GCD源码分析](https://developer.aliyun.com/article/61328)
+ [关于GCD开发的一些事儿](https://www.jianshu.com/p/f9e01c69a46f)
+ [GCD 深入理解：第一部分](https://github.com/nixzhu/dev-blog/blob/master/2014-04-19-grand-central-dispatch-in-depth-part-1.md)
+ [dispatch_once 详解](https://www.jianshu.com/p/4fd27f1db63d)
+ [透明联合类型](http://nanjingabcdefg.is-programmer.com/posts/23951.html)
+ [变态的libDispatch结构分析-dispatch_object_s](https://blog.csdn.net/passerbysrs/article/details/18228333?utm_source=blogxgwz2)
+ [深入浅出 GCD 之基础篇](https://xiaozhuanlan.com/topic/9168375240)
+ [从源码分析Swift多线程—DispatchGroup](http://leevcan.com/2020/05/30/从源码分析Swift多线程—DispatchGroup/)
+ [GCD源码分析（一）](https://www.jianshu.com/p/bd629d25dc2e)
+ [GCD-源码分析](https://www.jianshu.com/p/866b6e903a2d)
+ [GCD底层源码分析](https://www.jianshu.com/p/4ef55563cd14)
+ [GCD源码吐血分析(1)——GCD Queue](https://blog.csdn.net/u013378438/article/details/81031938)
+ [c/c++:计算可变参数宏 __VA_ARGS__ 的参数个数](https://blog.csdn.net/10km/article/details/80760533)
