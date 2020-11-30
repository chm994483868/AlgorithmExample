# iOS 多线程知识体系构建(十)：GCD 源码：dispatch_once、dispatch_group函数

> &emsp;上一篇我们学习了 dispatch_async 和 dispatch_sync 函数，本篇我们开始学习 GCD  中 dispatch_group 相关的函数。

&emsp;GCD 函数阅读过程中会涉及多个由大量宏定义组成的结构体的定义，需要一步一步进行宏展开才能更好的理解代码。

## dispatch_once
&emsp;`dispatch_once` 函数类似 `dispatch_sync` 内部也是直接调用后缀加 `_f` 的函数。
```c++
#ifdef __BLOCKS__
void
dispatch_once(dispatch_once_t *val, dispatch_block_t block)
{
    dispatch_once_f(val, block, _dispatch_Block_invoke(block));
}
#endif
```
### dispatch_once_t
&emsp;与 `dispatch_once` 函数一起使用的谓词，必须将其初始化为零。（静态和全局变量默认为零。）
```c++
DISPATCH_SWIFT3_UNAVAILABLE("Use lazily initialized globals instead")
typedef intptr_t dispatch_once_t;
```
### dispatch_once_gate_t
&emsp;`dispatch_gate_t` 是指向 `dispatch_gate_s` 结构体的指针，`dispatch_gate_s` 结构体仅有一个 `uint32_t` 类型的成员变量 `dgl_lock`。

&emsp;`dispatch_once_gate_t` 是指向 `dispatch_once_gate_s` 结构体的指针，`dispatch_once_gate_s` 结构体仅包含一个联合体。
```c++
typedef struct dispatch_gate_s {
    // typedef uint32_t dispatch_lock;
    dispatch_lock dgl_lock;
} dispatch_gate_s, *dispatch_gate_t;

typedef struct dispatch_once_gate_s {
    // 
    union {
        dispatch_gate_s dgo_gate;
        uintptr_t dgo_once;
    };
} dispatch_once_gate_s, *dispatch_once_gate_t;
```
### dispatch_once_f
&emsp;
```c++

#define DLOCK_ONCE_DONE   (~(uintptr_t)0)

DISPATCH_NOINLINE
void
dispatch_once_f(dispatch_once_t *val, void *ctxt, dispatch_function_t func)
{
    // 把 val 转换为 dispatch_once_gate_t 类型，
    dispatch_once_gate_t l = (dispatch_once_gate_t)val;

#if !DISPATCH_ONCE_INLINE_FASTPATH || DISPATCH_ONCE_USE_QUIESCENT_COUNTER
    uintptr_t v = os_atomic_load(&l->dgo_once, acquire);
    if (likely(v == DLOCK_ONCE_DONE)) {
        return;
    }
#if DISPATCH_ONCE_USE_QUIESCENT_COUNTER
    if (likely(DISPATCH_ONCE_IS_GEN(v))) {
        return _dispatch_once_mark_done_if_quiesced(l, v);
    }
#endif
#endif

    if (_dispatch_once_gate_tryenter(l)) {
        // 
        return _dispatch_once_callout(l, ctxt, func);
    }
    return _dispatch_once_wait(l);
}
```
#### _dispatch_once_gate_tryenter
&emsp;
```c++
#define DLOCK_ONCE_UNLOCKED   ((uintptr_t)0)

DISPATCH_ALWAYS_INLINE
static inline bool
_dispatch_once_gate_tryenter(dispatch_once_gate_t l)
{
    return os_atomic_cmpxchg(&l->dgo_once, DLOCK_ONCE_UNLOCKED,
            (uintptr_t)_dispatch_lock_value_for_self(), relaxed);
}
```
#### _dispatch_once_callout
&emsp;
```c++
DISPATCH_NOINLINE
static void
_dispatch_once_callout(dispatch_once_gate_t l, void *ctxt,
        dispatch_function_t func)
{
    // 调用函数
    _dispatch_client_callout(ctxt, func);
    // 
    _dispatch_once_gate_broadcast(l);
}
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
+ [OC底层探索(二十一)GCD异步、GCD同步、单例、信号量、调度组、栅栏函数等底层分析](https://blog.csdn.net/weixin_40918107/article/details/109520980)

