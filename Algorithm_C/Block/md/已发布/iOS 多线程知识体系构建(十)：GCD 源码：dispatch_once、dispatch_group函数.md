# iOS 多线程知识体系构建(十)：GCD 源码：dispatch_once、dispatch_group函数

> &emsp;上一篇我们学习了 dispatch_async 和 dispatch_sync 函数，本篇我们开始学习 GCD  中 dispatch_group 相关的函数。

&emsp;GCD 函数阅读过程中会涉及多个由大量宏定义组成的结构体的定义，需要一步一步进行宏展开才能更好的理解代码。

## dispatch_once
&emsp;`dispatch_once` 能保证任务只会被执行一次，即使同时多线程调用也是线程安全的。常用于创建单例、`swizzeld method` 等功能。

&emsp;`dispatch_once` 函数类似 `dispatch_sync` 函数，内部也是直接调用后缀加 `_f` 的函数。

&emsp;`dispatch_once` 是同步函数，会阻塞当前线程，直到 block 执行完成返回，才会执行接下的语句。（我们日常写的函数本来就是同步顺序执行的，可能看 `dispatch_sync` 和 `dispatch_async` 函数看的有点魔怔了，看到这种函数参数里面有个 block 的函数形式时，总是首先想想它会不会立刻返回或者要等 block 执行完成才会返回。）

&emsp;`dispatch_once` 不同于我们的日常的函数，它的 block 参数全局只能调用一次，即使在多线程的环境中也是全局只能执行一次，那么当多个线程同时调用 `dispatch_once` 时，系统时怎么加锁或者阻塞线程的呢？下面我们一起探究一下...
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
    union {
        dispatch_gate_s dgo_gate;
        uintptr_t dgo_once;
    };
} dispatch_once_gate_s, *dispatch_once_gate_t;
```
### DLOCK_ONCE_DONE/DLOCK_ONCE_UNLOCKED
&emsp;`DLOCK_ONCE_UNLOCKED` 用于标记 `dispatch_once` 还没有执行过，`DLOCK_ONCE_DONE` 用于标记 `dispatch_once` 已经执行过了。
```c++
#define DLOCK_ONCE_UNLOCKED   ((uintptr_t)0)
#define DLOCK_ONCE_DONE   (~(uintptr_t)0)
```
### dispatch_once_f
&emsp;根据 `val`（`dgo_once` 成员变量） 的值非零与否来判断是否执行 `dispatch_once_f` 提交的函数。
```c++
DISPATCH_NOINLINE
void
dispatch_once_f(dispatch_once_t *val, void *ctxt, dispatch_function_t func)
{
    // 把 val 转换为 dispatch_once_gate_t 类型
    dispatch_once_gate_t l = (dispatch_once_gate_t)val;

#if !DISPATCH_ONCE_INLINE_FASTPATH || DISPATCH_ONCE_USE_QUIESCENT_COUNTER
    // 原子性获取 l->dgo_once 的值
    uintptr_t v = os_atomic_load(&l->dgo_once, acquire);
    // 判断 v 的值是否是 DLOCK_ONCE_DONE（大概率是，表示 val 已经被赋值 DLOCK_ONCE_DONE 和 func 已经执行过了），是则直接返回
    if (likely(v == DLOCK_ONCE_DONE)) {
        return;
    }
#if DISPATCH_ONCE_USE_QUIESCENT_COUNTER
    // 不同的判定形式
    if (likely(DISPATCH_ONCE_IS_GEN(v))) {
        return _dispatch_once_mark_done_if_quiesced(l, v);
    }
#endif
#endif

    // 原子判断 l（l->dgo_once）是否非零，
    // 非零表示正在执行（或已经执行过了，如果执行过了 l->dgo_once 值是 DLOCK_ONCE_DONE，在上面的 if 中应该已经 return 了，或者下面的 _dispatch_once_wait 函数内部结束函数执行），
    // 零的话表示还没有执行过则进入 if 开始执行提交的函数
    
    // _dispatch_once_gate_tryenter 函数当 l（l->dgo_once）值是 NULL（0）时返回 YES，否则返回 NO，
    // 这样也保证了多线程调用时，只有最早的一条线程进入 if，开始执行提交的函数，然后另外的线程则是执行下面的 _dispatch_once_wait 函数阻塞，等待 _dispatch_once_callout 里面的唤醒操作

    if (_dispatch_once_gate_tryenter(l)) {
        // 执行 dispatch_once_f 提交的函数
        return _dispatch_once_callout(l, ctxt, func);
    }
    
    // 线程阻塞 等待 dispatch_function_t 提交的 func 执行完成或者内部判断 func 已经执行完成了，则直接 return 
    // 如果是阻塞的话，当 func 执行完后 _dispatch_once_callout 内部会发出广播唤醒阻塞线程
    return _dispatch_once_wait(l);
}
```
&emsp;下面对 `dispatch_once_f` 函数中嵌套调用的函数进行分析。
#### _dispatch_once_gate_tryenter
&emsp;`_dispatch_once_gate_tryenter` 函数原子性的判断 `l`（`l->dgo_once`） 是否非零，非零表示 `dispatch_once_f` 提交的函数已经执行过了，零的话还没有执行过，同时如果 `l`（`l->dgo_once`） 是零的话，`_dispatch_once_gate_tryenter` 函数内部也会把 `l`（`l->dgo_once`） 赋值为当前线程的 ID（这里是一个临时赋值），在最后 `dispatch_once_f` 中提交的函数执行完成后 `_dispatch_once_gate_broadcast` 函数内部会把 `l`（`l->dgo_once`）赋值为 `DLOCK_ONCE_DONE`。（`_dispatch_lock_value_for_self` 函数是取出当前线程的 ID）

&emsp;
```c++
DISPATCH_ALWAYS_INLINE
static inline bool
_dispatch_once_gate_tryenter(dispatch_once_gate_t l)
{
    // os_atomic_cmpxchg 原子性的判断 l->dgo_once 是否等于 DLOCK_ONCE_UNLOCKED（表示值为 0），若是 0 则赋值为当前线程 id
    // 如果 &l->dgo_once 的值为 NULL（0）则返回 YES，否则返回 NO
    return os_atomic_cmpxchg(&l->dgo_once, DLOCK_ONCE_UNLOCKED, (uintptr_t)_dispatch_lock_value_for_self(), relaxed);
}
```
##### os_atomic_cmpxchg
&emsp;`p` 变量相当于 `atomic_t` 类型的 `ptr` 指针用于获取当前内存访问制约规则 `m` 的值，用于对比旧值 `e`，若相等就赋新值 `v`。
```c++
#define os_atomic_cmpxchg(p, e, v, m) \
        ({ _os_atomic_basetypeof(p) _r = (e); \
        atomic_compare_exchange_strong_explicit(_os_atomic_c11_atomic(p), \
        &_r, v, memory_order_##m, memory_order_relaxed); })
```
##### _dispatch_lock_value_for_self
&emsp;`_dispatch_lock_value_for_self` 取出当前线程的 ID，用于赋值给 `val`（`dgo_once` 成员变量）。（ `val` 在 `dispatch_once_f` 提交的函数执行完成之前会赋值为线程 ID，当提交的函数执行完成后会赋值为 `DLOCK_ONCE_DONE`，例如我们为准备 `dispatch_once` 准备的 `static dispatch_once_t onceToken;`，在 `dispatch_once` 执行前打印 `onceToken` 值为 0，`onceToken` 初始值必须为 0，否则 `dispatch_once` 里的 block 不会执行，当 `dispatch_once` 执行完成后，打印 `onceToken`，它的值是 -1）。
```c++
DISPATCH_ALWAYS_INLINE
static inline dispatch_lock
_dispatch_lock_value_for_self(void)
{
    // _dispatch_tid_self() 为取出当前线程的ID
    return _dispatch_lock_value_from_tid(_dispatch_tid_self());
}
```
##### _dispatch_lock_value_from_tid
&emsp;`_dispatch_lock_value_from_tid` 函数内部仅是一个与操作。
```c++
DISPATCH_ALWAYS_INLINE
static inline dispatch_lock
_dispatch_lock_value_from_tid(dispatch_tid tid)
{
    // #define DLOCK_OWNER_MASK   ((dispatch_lock)0xfffffffc)
    return tid & DLOCK_OWNER_MASK;
}
```
&emsp;到这里与 `_dispatch_once_gate_tryenter` 相关的函数就看完了，根据 `_dispatch_once_gate_tryenter` 函数返回值，下面会有两个分支，一个是执行提交的函数，一个执行 `_dispatch_once_wait(l)` 阻塞等待提交的函数（多线程环境下的同时调用，恰巧处于提交的函数正在执行，另一个线程的调用也进来了，那么该线程会阻塞等待，在提交的函数执行完成后该阻塞的线程会被唤醒），下面我们先看一下首次执行 `dispatch_once` 函数的过程。
#### _dispatch_once_callout
&emsp;`_dispatch_once_callout` 函数做了两件事，一是调用提交的函数，二是发出广播唤醒阻塞等待的线程。
```c++
// return _dispatch_once_callout(l, ctxt, func);

DISPATCH_NOINLINE
static void
_dispatch_once_callout(dispatch_once_gate_t l, void *ctxt,
        dispatch_function_t func)
{
    // _dispatch_client_callout 函数上篇已经看过了，内部实现很简单，就是执行函数 f(ctxt)，（func(ctxt)）
    _dispatch_client_callout(ctxt, func);
    
    // 广播唤醒阻塞的线程
    _dispatch_once_gate_broadcast(l);
}
```
##### _dispatch_once_gate_broadcast
&emsp;
```c++
DISPATCH_ALWAYS_INLINE
static inline void
_dispatch_once_gate_broadcast(dispatch_once_gate_t l)
{
    // 取出当前线程的 ID
    dispatch_lock value_self = _dispatch_lock_value_for_self();
    
    uintptr_t v;
    
#if DISPATCH_ONCE_USE_QUIESCENT_COUNTER
    v = _dispatch_once_mark_quiescing(l);
#else
    // 原子性的设置 l（dgo_once 成员变量）的值为 DLOCK_ONCE_DONE，并返回 l（dgo_once 成员变量）的原始值
    v = _dispatch_once_mark_done(l);
#endif
    // 
    if (likely((dispatch_lock)v == value_self)) return;
    
    _dispatch_gate_broadcast_slow(&l->dgo_gate, (dispatch_lock)v);
}
```
##### _dispatch_once_mark_done
&emsp;原子性的设置 `&dgo->dgo_once` 的值为 `DLOCK_ONCE_DONE`，同时返回 `&dgo->dgo_once` 的旧值。
```c++
DISPATCH_ALWAYS_INLINE
static inline uintptr_t
_dispatch_once_mark_done(dispatch_once_gate_t dgo)
{
    return os_atomic_xchg(&dgo->dgo_once, DLOCK_ONCE_DONE, release);
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

