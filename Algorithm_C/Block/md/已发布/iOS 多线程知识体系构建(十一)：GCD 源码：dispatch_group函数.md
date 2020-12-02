# iOS 多线程知识体系构建(十一)：GCD 源码：dispatch_group函数

> &emsp;本篇看 dispatch_group 的内容。

## dispatch_group
&emsp;用法的话可以看前面的文章，本篇只看数据结构和相关的 API 的源码实现。
### dispatch_group_s
&emsp;`dispatch_group_s` 定义和 `dispatch_semaphore_s` 定义都是放在 semaphore_internal.h 文件中，而且该文件中仅包含它俩的内容，其实文件这样布局也是有用意的，因为它俩的内部实现有一些相似性，dispatch_group 在内部也会维护一个值，当进组和出组时对该值进行操作，当达到临界值 0 时会做一些后续操作，下面就一起看看 dispatch_group 的详细实现吧。

&emsp;`dispatch_group_t` 是指向 `dispatch_group_s` 结构体的指针。首先看一下基础的数据结构。
```c++
DISPATCH_CLASS_DECL(group, OBJECT);
struct dispatch_group_s {
    DISPATCH_OBJECT_HEADER(group);
    DISPATCH_UNION_LE(uint64_t volatile dg_state,
            uint32_t dg_bits,
            uint32_t dg_gen
    ) DISPATCH_ATOMIC64_ALIGN;
    struct dispatch_continuation_s *volatile dg_notify_head;
    struct dispatch_continuation_s *volatile dg_notify_tail;
};
```
&emsp;宏定义展开如下:
```c++
struct dispatch_group_extra_vtable_s {
    unsigned long const do_type;
    void (*const do_dispose)(struct dispatch_group_s *, bool *allow_free);
    size_t (*const do_debug)(struct dispatch_group_s *, char *, size_t);
    void (*const do_invoke)(struct dispatch_group_s *, dispatch_invoke_context_t, dispatch_invoke_flags_t);
};

struct dispatch_group_vtable_s {
    // _OS_OBJECT_CLASS_HEADER();
    void (*_os_obj_xref_dispose)(_os_object_t);
    void (*_os_obj_dispose)(_os_object_t);
    
    struct dispatch_group_extra_vtable_s _os_obj_vtable;
};

// OS_OBJECT_CLASS_SYMBOL(dispatch_group)

extern const struct dispatch_group_vtable_s _OS_dispatch_group_vtable;
extern const struct dispatch_group_vtable_s OS_dispatch_group_class __asm__("__" OS_STRINGIFY(dispatch_group) "_vtable");

struct dispatch_group_s {
    struct dispatch_object_s _as_do[0];
    struct _os_object_s _as_os_obj[0];
    
    const struct dispatch_group_vtable_s *do_vtable; /* must be pointer-sized */
    
    int volatile do_ref_cnt;
    int volatile do_xref_cnt;
    
    struct dispatch_group_s *volatile do_next;
    struct dispatch_queue_s *do_targetq;
    void *do_ctxt;
    void *do_finalizer;
    
    // 可看到上半部分和其它 GCD 对象都是相同的，毕竟大家都是继承自 dispatch_object_s，重点是下面的新内容
    
    union { 
        uint64_t volatile dg_state; 
        struct { 
            uint32_t dg_bits; // 保存进组的任务数量 
            uint32_t dg_gen; 
        };
    } __attribute__((aligned(8)));
    
    struct dispatch_continuation_s *volatile dg_notify_head;
    struct dispatch_continuation_s *volatile dg_notify_tail;
};
```
&emsp;`DISPATCH_VTABLE_INSTANCE` 宏定义包裹的内容是 `dispatch_group_vtable_s` 结构体中的内容的初始化，即 dispatch_group 的一些操作函数。（在 init.c 文件中 Dispatch object cluster 部分包含很多 GCD 对象的操作函数的的初始化）
```c++
// dispatch_group_extra_vtable_s 结构体中对应的成员变量的赋值
DISPATCH_VTABLE_INSTANCE(group,
    .do_type        = DISPATCH_GROUP_TYPE,
    .do_dispose     = _dispatch_group_dispose,
    .do_debug       = _dispatch_group_debug,
    .do_invoke      = _dispatch_object_no_invoke,
);
⬇️（宏展开）
DISPATCH_VTABLE_SUBCLASS_INSTANCE(group, group, __VA_ARGS__)
⬇️（宏展开）
OS_OBJECT_VTABLE_SUBCLASS_INSTANCE(dispatch_group, dispatch_group, _dispatch_xref_dispose, _dispatch_dispose, __VA_ARGS__)
⬇️（宏展开）
const struct dispatch_group_vtable_s OS_OBJECT_CLASS_SYMBOL(dispatch_group) = { \
    ._os_obj_xref_dispose = _dispatch_xref_dispose, \
    ._os_obj_dispose = _dispatch_dispose, \
    ._os_obj_vtable = { __VA_ARGS__ }, \
}
⬇️（宏展开）
const struct dispatch_group_vtable_s OS_dispatch_group_class = {
    ._os_obj_xref_dispose = _dispatch_xref_dispose,
    ._os_obj_dispose = _dispatch_dispose,
    ._os_obj_vtable = { 
        .do_type        = DISPATCH_GROUP_TYPE,
        .do_dispose     = _dispatch_group_dispose,
        .do_debug       = _dispatch_group_debug,
        .do_invoke      = _dispatch_object_no_invoke,
    }, 
}
```
### dispatch_group_create
&emsp;`dispatch_group_create` 用于创建可与 block 关联的新组（`dispatch_group_s`），调度组（dispatch group）可用于等待它引用的 block 的执行完成（所有的 blocks 异步执行完成）。（使用 `dispatch_release` 释放 group 对象内存）

&emsp;`dispatch_group_create` 函数内部直接调用了 `_dispatch_group_create_with_count` 并且入参为 0，表明目前没有进组操作。`_dispatch_group_create_and_enter` 函数则调用 `_dispatch_group_create_with_count` 入参为 1，表明进行了一次进组操作。
```c++
dispatch_group_t
dispatch_group_create(void)
{
    return _dispatch_group_create_with_count(0);
}

dispatch_group_t
_dispatch_group_create_and_enter(void)
{
    return _dispatch_group_create_with_count(1);
}
```
#### _dispatch_group_create_with_count
&emsp;创建 `dispatch_group_s` 时指定 `dg_bits` 的值。 
```c++
DISPATCH_ALWAYS_INLINE
static inline dispatch_group_t
_dispatch_group_create_with_count(uint32_t n)
{
    // DISPATCH_VTABLE(group) ➡️ &OS_dispatch_group_class
    // _dispatch_object_alloc 是为 dispatch_group_s 申请空间，然后用 &OS_dispatch_group_class 初始化，
    // &OS_dispatch_group_class 设置了 dispatch_semaphore_t 的相关回调函数，如销毁函数 _dispatch_group_dispose 等
    dispatch_group_t dg = _dispatch_object_alloc(DISPATCH_VTABLE(group),
            sizeof(struct dispatch_group_s));
    
    // #if DISPATCH_SIZEOF_PTR == 8
    // // the bottom nibble must not be zero, the rest of the bits should be random we sign extend the 64-bit version so that a better instruction encoding is generated on Intel
    // #define DISPATCH_OBJECT_LISTLESS ((void *)0xffffffff89abcdef)
    // #else
    // #define DISPATCH_OBJECT_LISTLESS ((void *)0x89abcdef)
    // #endif
    
    // 表示链表的下一个节点
    dg->do_next = DISPATCH_OBJECT_LISTLESS;
    
    // 目标队列（从全局的队列数组 _dispatch_root_queues 中取默认队列）
    dg->do_targetq = _dispatch_get_default_queue(false);
    
    // ⬆️ 以上内容完全同 dispatch_semaphore_s 可参考上篇，这里不再展开 
    
    // #define DISPATCH_GROUP_VALUE_INTERVAL   0x0000000000000004ULL
    
    if (n) {
        // 以原子方式把 (uint32_t)-n * DISPATCH_GROUP_VALUE_INTERVAL 的值保存到 dg_bits 中
        // 以原子方式把 1 保存到 do_ref_cnt 中
        os_atomic_store2o(dg, dg_bits,
                (uint32_t)-n * DISPATCH_GROUP_VALUE_INTERVAL, relaxed);
        os_atomic_store2o(dg, do_ref_cnt, 1, relaxed); // <rdar://22318411>
    }
    return dg;
}
```
##### os_atomic_store2o
&emsp;`os_atomic_store2o` 宏定义中 `f` 是 `p` 的成员变量，内部的 `atomic_store_explicit` 函数保证以原子方式把 `v` 的值存储到 `f` 中。
```c++
#define os_atomic_store2o(p, f, v, m) \
        os_atomic_store(&(p)->f, (v), m)

#define os_atomic_store(p, v, m) \
        atomic_store_explicit(_os_atomic_c11_atomic(p), v, memory_order_##m)
```
### dispatch_group_enter
&emsp;`dispatch_group_enter` 表示 dispatch_group 已手动输入块。

&emsp;调用此函数表示一个 block 已通过 `dispatch_group_async` 以外的方式加入了该 dispatch_group，对该函数的调用必须与 `dispatch_group_leave` 平衡。
```c++
void
dispatch_group_enter(dispatch_group_t dg)
{
    // The value is decremented on a 32bits wide atomic so that the carry
    // for the 0 -> -1 transition is not propagated to the upper 32bits.
    // 该值在 32 位宽的原子上递减，以使 0->-1 过渡的进位不会传播到高 32 位。
    
    // dg_bits 原子方式减少 DISPATCH_GROUP_VALUE_INTERVAL
    uint32_t old_bits = os_atomic_sub_orig2o(dg, dg_bits,
            DISPATCH_GROUP_VALUE_INTERVAL, acquire);
    
    // #define DISPATCH_GROUP_VALUE_MASK   0x00000000fffffffcULL
    // 拿 dg_bits 的旧值和 DISPATCH_GROUP_VALUE_MASK 进行与操作
    uint32_t old_value = old_bits & DISPATCH_GROUP_VALUE_MASK;
    
    if (unlikely(old_value == 0)) {
        // 表示此时有新 block 进组了，dg 的内部引用计数 +1 
        _dispatch_retain(dg); // <rdar://problem/22318411>
    }
    
    // #define DISPATCH_GROUP_VALUE_INTERVAL   0x0000000000000004ULL
    // #define DISPATCH_GROUP_VALUE_MAX   DISPATCH_GROUP_VALUE_INTERVAL
    
    // 如果旧值等于 DISPATCH_GROUP_VALUE_MAX 表示 dispatch_group_enter 函数过度调用，会 crash
    if (unlikely(old_value == DISPATCH_GROUP_VALUE_MAX)) {
        DISPATCH_CLIENT_CRASH(old_bits,
                "Too many nested calls to dispatch_group_enter()");
    }
}
```
#### _dispatch_retain
```c++
DISPATCH_ALWAYS_INLINE_NDEBUG
static inline void
_dispatch_retain(dispatch_object_t dou)
{
    (void)_os_object_retain_internal_n_inline(dou._os_obj, 1);
}
```
##### _os_object_retain_internal_n_inline
```c++
DISPATCH_ALWAYS_INLINE
static inline _os_object_t
_os_object_retain_internal_n_inline(_os_object_t obj, int n)
{
    int ref_cnt = _os_object_refcnt_add_orig(obj, n);
    
    if (unlikely(ref_cnt < 0)) {
        _OS_OBJECT_CLIENT_CRASH("Resurrection of an object");
    }
    
    return obj;
}
```
##### _os_object_refcnt_add_orig
```c++
#define _os_object_refcnt_add_orig(o, n) \
        _os_atomic_refcnt_add_orig2o(o, os_obj_ref_cnt, n)
        
#define _os_atomic_refcnt_add_orig2o(o, m, n) \
        _os_atomic_refcnt_perform2o(o, m, add_orig, n, relaxed)

#define _os_atomic_refcnt_perform2o(o, f, op, n, m)   ({ \
        __typeof__(o) _o = (o); \
        int _ref_cnt = _o->f; \
        if (likely(_ref_cnt != _OS_OBJECT_GLOBAL_REFCNT)) { \
            _ref_cnt = os_atomic_##op##2o(_o, f, n, m); \
        } \
        _ref_cnt; \
    })
```
### dispatch_group_leave
&emsp;`dispatch_group_leave` 手动指示 dispatch_group 中的 block 已完成。

&emsp;调用此函数表示 block 已完成，并且已通过 `dispatch_group_async` 以外的方式离开了 dispatch_group。
```c++
void
dispatch_group_leave(dispatch_group_t dg)
{
    // The value is incremented on a 64bits wide atomic so that the carry
    // for the -1 -> 0 transition increments the generation atomically.
    // 该值在 64 位宽的原子上递增，以便 -1 -> 0 的携带以原子方式递增生成。
    
    uint64_t new_state, old_state = os_atomic_add_orig2o(dg, dg_state,
            DISPATCH_GROUP_VALUE_INTERVAL, release);
            
    uint32_t old_value = (uint32_t)(old_state & DISPATCH_GROUP_VALUE_MASK);

    // #define DISPATCH_GROUP_VALUE_MASK   0x00000000fffffffcULL
    // #define DISPATCH_GROUP_VALUE_1   DISPATCH_GROUP_VALUE_MASK
    
    if (unlikely(old_value == DISPATCH_GROUP_VALUE_1)) {
        old_state += DISPATCH_GROUP_VALUE_INTERVAL;
        
        do {
            new_state = old_state;
            if ((old_state & DISPATCH_GROUP_VALUE_MASK) == 0) {
                new_state &= ~DISPATCH_GROUP_HAS_WAITERS;
                new_state &= ~DISPATCH_GROUP_HAS_NOTIFS;
            } else {
                // If the group was entered again since the atomic_add above,
                // we can't clear the waiters bit anymore as we don't know for
                // which generation the waiters are for
                new_state &= ~DISPATCH_GROUP_HAS_NOTIFS;
            }
            if (old_state == new_state) break;
        } while (unlikely(!os_atomic_cmpxchgv2o(dg, dg_state,
                old_state, new_state, &old_state, relaxed)));
                
        // 
        return _dispatch_group_wake(dg, old_state, true);
    }

    if (unlikely(old_value == 0)) {
        DISPATCH_CLIENT_CRASH((uintptr_t)old_value,
                "Unbalanced call to dispatch_group_leave()");
    }
}
```
#### os_atomic_add_orig2o
```c++
#define os_atomic_add_orig2o(p, f, v, m) \
        os_atomic_add_orig(&(p)->f, (v), m)

#define os_atomic_add_orig(p, v, m) \
        _os_atomic_c11_op_orig((p), (v), m, add, +)
        
#define _os_atomic_c11_op_orig(p, v, m, o, op) \
        atomic_fetch_##o##_explicit(_os_atomic_c11_atomic(p), v, \
        memory_order_##m)
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
+ [iOS源码解析: GCD的信号量semaphore](https://juejin.cn/post/6844904122370490375)

