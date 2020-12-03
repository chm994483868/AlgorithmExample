# iOS 多线程知识体系构建(十一)：GCD 源码：dispatch_group函数

> &emsp;用法的话可以看前面的文章，本篇只看 dispatch_group 的数据结构和相关 API 的源码实现。

## dispatch_group
&emsp;dispatch_group 可以将 GCD 的任务合并到一个组里来管理，可以同时监听组里所有任务的执行情况，当组内所有任务异步执行完毕后我们可以得到一个回调通知。
### dispatch_group_s
&emsp;`dispatch_group_s` 定义和 `dispatch_semaphore_s` 定义都是放在 semaphore_internal.h 文件中，而且该文件中仅包含它俩的内容，其实文件这样布局也是有用意的，因为它俩的内部实现有一些相似性，dispatch_group 在内部也会维护一个值，当进组（`dg_bits` - `0x0000000000000004ULL`）和出组（`dg_state` + `0x0000000000000004ULL`）时对该值进行操作，当该值达到临界值 0 时会做一些后续操作（`_dispatch_group_wake` 唤醒 `dispatch_group_notify` 函数添加的回调），且在使用过程中一定要谨记进组（enter）和出组（leave）保持平衡，下面就一起看看 dispatch_group 的详细实现吧（`dg_bits` 和 `dg_state` 两个变量下面会细说，还有其实在之前的老版本中 dispatch_group 的内部是用 dispatch_semaphore 实现的，目前则是使用类似的思想各自独立实现，我们目前看的源码正是最新版本）。

&emsp;依然从基础的数据结构开始，`dispatch_group_t` 是指向 `dispatch_group_s` 结构体的指针，那么先看下 `dispatch_group_s` 结构体的定义。
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
        uint64_t volatile dg_state;  // leave 时加 DISPATCH_GROUP_VALUE_INTERVAL
        struct { 
            uint32_t dg_bits; // enter 时减 DISPATCH_GROUP_VALUE_INTERVAL
            uint32_t dg_gen;  // 主要用于 dispatch_group_wait 函数中 
        };
    } __attribute__((aligned(8)));
    
    // 下面两个成员变量比较特殊，它们分别是一个链表的头部和尾部
    // 调用 dispatch_group_notify 函数可添加当 dispatch_group 关联的 block 异步执行完成后的回调事件，
    // 多次调用 dispatch_group_notify 函数可添加多个回调事件（我们日常开发一般就用了一个回调事件，可能会忽略这个细节），
    // 而这些回调事件则会构成一个 dispatch_continuation_s 作为节点的链表，当 dispatch_group 中关联的 block 全部执行完成后，
    // 此链表中的 dispatch_continuation_s 都会得到异步执行（注意是异步，具体在哪个队列则根据 dispatch_group_notify 函数的入参决定）。
    
    struct dispatch_continuation_s *volatile dg_notify_head; // dispatch_continuation_s 链表的头部节点
    struct dispatch_continuation_s *volatile dg_notify_tail; // dispatch_continuation_s 链表的尾部节点
};
```
&emsp;`dg_bits` 和 `dg_state` 是联合体共享同一块内存空间的不同的成员变量，进组和出组时减加 `DISPATCH_GROUP_VALUE_INTERVAL` 操作的其实是同一个值，再详细一点的话是联合体共占用 64 bit 空间，其中 uint64_t 类型的 dg_state 可占完整 64 bit，然后 uint32_t 类型的 `dg_bits` 和  uint32_t 类型的 `dg_gen` 组成结构体共占用这 64 bit，其中 `dg_bits` 在 低 32 bit，`dg_gen` 在高 32 bit。

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
&emsp;`dispatch_group_create` 用于创建可与 block 关联的新组（`dispatch_group_s`），调度组（`dispatch_group_s`）可用于等待它引用的 block 的执行完成（所有的 blocks 异步执行完成）。（使用 `dispatch_release` 释放 group 对象内存）

&emsp;关联方式：
+ 调用 `dispatch_group_enter` 表示一个 block 已与组关联，同时 block 执行完后要调用 `dispatch_group_leave` 表示解除关联，否则 `dispatch_group_s` 会永远等下去。
+ 调用 `dispatch_group_async` 函数。

&emsp;`dispatch_group_create` 函数内部直接调用了 `_dispatch_group_create_with_count` 并且入参为 0，表明目前没有 block 关联组操作。`_dispatch_group_create_and_enter` 函数则调用 `_dispatch_group_create_with_count` 入参为 1，表明有一次 block 关联组操作。
```c++
dispatch_group_t
dispatch_group_create(void)
{
    // 入参为 0 调用 _dispatch_group_create_with_count 函数
    return _dispatch_group_create_with_count(0);
}

dispatch_group_t
_dispatch_group_create_and_enter(void)
{
    // 入参为 1，调用 _dispatch_group_create_with_count 函数，表示有一次 enter 操作
    return _dispatch_group_create_with_count(1);
}
```
&emsp;有符号数字和无符号数字的转换想必大家都比较熟悉了，不过开始看 `_dispatch_group_create_with_count` 函数之前我们还是先重温一下数值转换的知识点。

&emsp;在 dispatch_group 中进组出组的操作每次是用加减 4 （`DISPATCH_GROUP_VALUE_INTERVAL`）来记录的并不是常见的加 1 减 1，然后起始值是从 uint32_t 的最小值 0 开始的，这里用了一个无符号数和有符号数的转换的小技巧，例如 dispatch_group 起始状态时 uint32_t 类型的 `dg_bits` 值为 0，然后第一个 enter 操作进来以后，把 uint32_t 类型的 `dg_bits` 从 0 减去 4，然后 -4 转换为 uint32_t 类型后值为 4294967292，然后 leave 操作时 `dg_bits` 加 4，即 4294967292 加 4，这样会使 uint32_t 类型值溢出然后 `dg_bits` 值就变回 0 了（uint32_t 类型的最小值），对应到 dispatch_group 中的逻辑原理即表示 `dg_bits` 达到临界值了，表示与组关联的 block 都执行完成了，可以执行后续的唤醒操作了。

&emsp;还有一点，`dg_bits` 使用 32 bit 空间对应使用 uint32_t 类型，然后 `DISPATCH_GROUP_VALUE_INTERVAL`（间隔）用 4 是因为 uint32_t 类型表示的数字个数刚好是 4 的整数倍吗，不过只要是 2 的幂都是整数倍，且 uint32_t 类型的数字即使以 4 为间隔表示的数字也完全足够使用了， 应该主要是为了掩码的使用，4 的二进制表示时后两位是 0，然后后两位是 1 时分别对应 `DISPATCH_GROUP_HAS_NOTIFS` 和 `DISPATCH_GROUP_HAS_WAITERS` 两个宏，这两个宏的使用我们后续再讲。
```c++
#define DISPATCH_GROUP_VALUE_MASK       0x00000000fffffffcULL
#define DISPATCH_GROUP_VALUE_INTERVAL   0x0000000000000004ULL

#define DISPATCH_GROUP_VALUE_1          DISPATCH_GROUP_VALUE_MASK // 可表示 dispatch_group 关联一个 block 
#define DISPATCH_GROUP_VALUE_MAX        DISPATCH_GROUP_VALUE_INTERVAL // 可表示 dispatch_group 关联的 block 达到了最大值，正常情况时应小于此值

#define DISPATCH_GROUP_HAS_NOTIFS       0x0000000000000002ULL 
#define DISPATCH_GROUP_HAS_WAITERS      0x0000000000000001ULL // 对应 dispatch_group_wait 函数的使用， dispatch_group 处于等待状态

uint32_t max = (uint32_t)-1;
NSLog(@"⏰⏰：%u", UINT32_MAX);
NSLog(@"⏰⏰：%u", max);
max = max + 1;
NSLog(@"⏰⏰：%u", max);

// Implicit conversion from 'unsigned long long' to 'uint32_t' (aka 'unsigned int') changes value from 17179869180 to 4294967292
// 把 -1 转换为 uint32_t 后再转换为 ULL（无符号 long long） 然后乘以 0x0000000000000004ULL 后再强转为 uint32_t（也可以理解为 -4 转换为 uint32_t）
uint32_t dg_bits = (uint32_t)-1 * DISPATCH_GROUP_VALUE_INTERVAL;

NSLog(@"⏰⏰：%u", dg_bits);
NSLog(@"⏰⏰：%u", (uint32_t)-4);

// Implicit conversion from 'unsigned long long' to 'uint32_t' (aka 'unsigned int') changes value from 17179869176 to 4294967288
// 把 -2 转换为 uint32_t 后再转换为 ULL（无符号 long long） 然后乘以 0x0000000000000004ULL 后再强转为 uint32_t（也可以理解为 -8 转换为 uint32_t）
dg_bits = (uint32_t)-2 * DISPATCH_GROUP_VALUE_INTERVAL;

NSLog(@"⏰⏰：%u", dg_bits);

dg_bits = dg_bits - DISPATCH_GROUP_VALUE_INTERVAL;
NSLog(@"⏰⏰：%u", dg_bits);

// 打印结果:
⏰⏰：4294967295 // ⬅️ UINT32_MAX 无符号 32 位 int 的最大值（十六进制：0xFFFFFFFF）
⏰⏰：4294967295 // ⬅️ -1 强制转换为 uint32_t 的值即为 UINT32_MAX（十六进制：0xFFFFFFFF）
⏰⏰：0 // ⬅️ UINT32_MAX + 1 后溢出即切回到 uint32_t 类型的最小值 0（十六进制：0x00000000） 
⏰⏰：4294967292 // ⬅️ (uint32_t)-1 * DISPATCH_GROUP_VALUE_INTERVAL 计算的值（十六进制：0xFFFFFFFC）
⏰⏰：4294967292 // ⬅️ -4 强转为 uint32_t 类型的值，看到和上面 ⬆️ 是相等的（十六进制：0xFFFFFFFC）
⏰⏰：4294967288 // ⬅️ (uint32_t)-2 * DISPATCH_GROUP_VALUE_INTERVAL 的值也可以理解为 dg_bits 从 0 开始减了两次 4（十六进制：0xFFFFFFF8）
⏰⏰：4294967284 // ⬅️ 值为 dg_bits 再次减去 DISPATCH_GROUP_VALUE_INTERVAL（）（十六进制：0xFFFFFFF4）
```
#### _dispatch_group_create_with_count
&emsp;`_dispatch_group_create_with_count` 创建 `dispatch_group_s` 结构体，如果入参 `n` 大于 0，则指定为 `dg_bits` 的值为 `(uint32_t)-n * DISPATCH_GROUP_VALUE_INTERVAL`，并且 `dispatch_group_s` 的引用计数加 1。（`do_ref_cnt`）
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
    // // the bottom nibble must not be zero, the rest of the bits should be random we sign extend 
    // // the 64-bit version so that a better instruction encoding is generated on Intel
    // #define DISPATCH_OBJECT_LISTLESS ((void *)0xffffffff89abcdef)
    // #else
    // #define DISPATCH_OBJECT_LISTLESS ((void *)0x89abcdef)
    // #endif
    
    // 表示链表的下一个节点，（目前赋一个初值 DISPATCH_OBJECT_LISTLESS）
    dg->do_next = DISPATCH_OBJECT_LISTLESS;
    
    // 目标队列（从全局的队列数组 _dispatch_root_queues 中取默认 QOS 的队列）
    dg->do_targetq = _dispatch_get_default_queue(false);
    
    // ⬆️ 以上内容完全同 dispatch_semaphore_s 可参考上篇，这里不再展开 
    
    if (n) {
        // ⬇️ 以原子方式把 (uint32_t)-n * DISPATCH_GROUP_VALUE_INTERVAL 的值存储到 dg_bits 中
        // n 表示 dg 关联的 block 数量
        os_atomic_store2o(dg, dg_bits, (uint32_t)-n * DISPATCH_GROUP_VALUE_INTERVAL, relaxed);
        
        // ⬇️ 以原子方式把 1 保存到 do_ref_cnt 中（表示 dispatch_group 内部引用计数为 1，即目前有与组关联的 block 或者有任务进组了）
        os_atomic_store2o(dg, do_ref_cnt, 1, relaxed); // <rdar://22318411>
    }
    
    return dg;
}
```
&emsp;`os_atomic_store2o` 宏定义中 `f` 是 `p` 的成员变量，内部的 `atomic_store_explicit` 函数保证以原子方式把 `v` 的值存储到 `f` 中。
```c++
#define os_atomic_store2o(p, f, v, m) \
        os_atomic_store(&(p)->f, (v), m)

#define _os_atomic_c11_atomic(p) \
        ((__typeof__(*(p)) _Atomic *)(p))

#define os_atomic_store(p, v, m) \
        atomic_store_explicit(_os_atomic_c11_atomic(p), v, memory_order_##m)
```
### dispatch_group_enter
&emsp;`dispatch_group_enter` 表示 dispatch_group 已手动输入块。

&emsp;调用此函数表示一个 block 已通过 `dispatch_group_async` 以外的方式加入了该 dispatch_group，对该函数的调用必须与 `dispatch_group_leave` 保持平衡。
```c++
void
dispatch_group_enter(dispatch_group_t dg)
{
    // The value is decremented on a 32bits wide atomic so that the  carry
    // for the 0 -> -1 transition is not propagated to the upper 32bits.
    // dg_bits 是 32 位 int，-1 和 0 的转换在 32 位 int 范围内，不会过渡到高位，影响 dg_gen 和 dg_state 的值
    
    // dg_bits 以原子方式减少 DISPATCH_GROUP_VALUE_INTERVAL，并返回 dg_bits 的旧值
    // 表示 dispatch_group 增加了一个关联 block
    uint32_t old_bits = os_atomic_sub_orig2o(dg, dg_bits, DISPATCH_GROUP_VALUE_INTERVAL, acquire);
    
    // #define DISPATCH_GROUP_VALUE_MASK   0x00000000fffffffcULL 二进制表示 ➡️ 0b0000...11111100ULL
    // 拿 dg_bits 的旧值和 DISPATCH_GROUP_VALUE_MASK 进行与操作，主要是用来判断 dg_bits 的旧值是否是 0，
    // 即用来判断这次 enter 之前 dispatch_group 内部是否没有关联任何 block。
    uint32_t old_value = old_bits & DISPATCH_GROUP_VALUE_MASK;
    
    if (unlikely(old_value == 0)) {
        // 表示此时调度组由未关联任何 block 的状态变换到了关联了一个 block 的状态，
        // dg 的内部引用计数 +1 
        
        //（表示 dispatch_group 内部有 block 待执行即调度组正在被使用，
        // 此时 dispatch_group 不能进行释放，想到前面的信号量，
        // 如果 dsema_value 小于 dsema_orig 表示信号量正在使用，此时释放信号量对象的话也会导致 crash，
        // 整体思想和我们的 NSObject 的引用计数是相同的，不同之处是内存泄漏不一定会 crash，而这里则是立即 crash，
        // 当然作为一名合格的开发绝对不能容许任何内存泄漏和崩溃！！！）
        
        _dispatch_retain(dg); // <rdar://problem/22318411>
    }
    
    // #define DISPATCH_GROUP_VALUE_INTERVAL   0x0000000000000004ULL
    // #define DISPATCH_GROUP_VALUE_MAX   DISPATCH_GROUP_VALUE_INTERVAL
    
    // 如果旧值等于 DISPATCH_GROUP_VALUE_MAX 表示 dispatch_group_enter 函数过度调用，则 crash
    // 0 + DISPATCH_GROUP_VALUE_INTERVAL
    
    if (unlikely(old_value == DISPATCH_GROUP_VALUE_MAX)) {
        DISPATCH_CLIENT_CRASH(old_bits,
                "Too many nested calls to dispatch_group_enter()");
    }
}
```
#### _dispatch_retain
&emsp;`_dispatch_retain` GCD 对象的引用计数加 1（`os_obj_ref_cnt` 的值）。
```c++
DISPATCH_ALWAYS_INLINE_NDEBUG
static inline void
_dispatch_retain(dispatch_object_t dou)
{
    (void)_os_object_retain_internal_n_inline(dou._os_obj, 1);
}
```
&emsp;`_os_object_retain_internal_n_inline` 的第一个参数是 `_os_object_t` 类型。`dou._os_obj` 的入参，正是我们很早就看过的 `dispatch_object_t` 透明联合体中的 `struct _os_object_s *_os_obj` 成员变量。
```c++
DISPATCH_ALWAYS_INLINE
static inline _os_object_t
_os_object_retain_internal_n_inline(_os_object_t obj, int n)
{
    int ref_cnt = _os_object_refcnt_add_orig(obj, n);
    
    // ref_cnt 是引用计数的原始值，如果原始值小于 0，则 crash 
    if (unlikely(ref_cnt < 0)) {
        _OS_OBJECT_CLIENT_CRASH("Resurrection of an object");
    }
    
    return obj;
}
```
&emsp;`_os_object_refcnt_add_orig` 是一个宏定义，以原子方式增加引用计数。
```c++
#define _os_object_refcnt_add_orig(o, n) \
        _os_atomic_refcnt_add_orig2o(o, os_obj_ref_cnt, n)
        
#define _os_atomic_refcnt_add_orig2o(o, m, n) \
        _os_atomic_refcnt_perform2o(o, m, add_orig, n, relaxed)

// #define _OS_OBJECT_GLOBAL_REFCNT INT_MAX

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
&emsp;`dispatch_group_leave` 手动指示 dispatch_group 中的一个关联 block 已完成。

&emsp;调用此函数表示一个关联 block 已完成，并且已通过 `dispatch_group_async` 以外的方式离开了 dispatch_group。
```c++
void
dispatch_group_leave(dispatch_group_t dg)
{
    // The value is incremented on a 64bits wide atomic so that the carry
    // for the -1 -> 0 transition increments the generation atomically.
    // dg_state 是无符号 64 位 int，但是此处是加 -1 到 0 的转换不受影响
    
    // 看到这里是以原子方式增加 dg_state 的值，这里 dg_bits 的内存空间是 dg_state 的低 32 bit，
    // 所以 dg_state + DISPATCH_GROUP_VALUE_INTERVAL 即 dg_bits + DISPATCH_GROUP_VALUE_INTERVAL。

    //（这里注意是把 dg_state 的旧值同时赋值给了 new_state 和 old_state）
    uint64_t new_state, old_state = os_atomic_add_orig2o(dg, dg_state,
            DISPATCH_GROUP_VALUE_INTERVAL, release);
    
    // #define DISPATCH_GROUP_VALUE_MASK   0x00000000fffffffcULL ➡️ 0b0000...11111100ULL
    // #define DISPATCH_GROUP_VALUE_1   DISPATCH_GROUP_VALUE_MASK
    // dg_state 的旧值和 DISPATCH_GROUP_VALUE_MASK 进行与操作，如果此时仅关联了一个 block 的话那么 dg_state 的旧值就是（十六进制：0xFFFFFFFC），
    // （那么上面的 os_atomic_add_orig2o 执行后，dg_state 的值是 0x0000000100000000ULL，因为它是 uint64_t 类型，不同于 dg_bits 的 uint32_t 类型）
    
    // 如果 dg_state 旧值 old_state 等于 0xFFFFFFFC 则与操作结果还是 0xFFFFFFFC
    uint32_t old_value = (uint32_t)(old_state & DISPATCH_GROUP_VALUE_MASK);

    // 如果 old_value 等于 DISPATCH_GROUP_VALUE_1，然后再经过上面的 + DISPATCH_GROUP_VALUE_INTERVAL 后，刚好等于 0 了，
    // 即 enter 和 leave 平衡了，即调度组关联的 block 执行完了。
    
    if (unlikely(old_value == DISPATCH_GROUP_VALUE_1)) {
        // old_value 的值是 0xFFFFFFFC
        
        // old_state 是 0x00000000fffffffcULL，DISPATCH_GROUP_VALUE_INTERVAL 的值是 0x0000000000000004ULL
        // 所以这里 old_state 是 uint64_t 类型，不会发生溢出，old_state = 0x0000000100000000ULL
        old_state += DISPATCH_GROUP_VALUE_INTERVAL;
        
        // #define DISPATCH_GROUP_HAS_WAITERS   0x0000000000000001ULL
        // #define DISPATCH_GROUP_HAS_NOTIFS    0x0000000000000002ULL
        
        do {
            // new_state = 0x0000000100000000ULL
            new_state = old_state;
            
            if ((old_state & DISPATCH_GROUP_VALUE_MASK) == 0) {
                // 如果目前是仅关联了一个 block 而且是正常的 enter 和 leave 配对执行，则会执行这里
                
                new_state &= ~DISPATCH_GROUP_HAS_WAITERS;
                new_state &= ~DISPATCH_GROUP_HAS_NOTIFS;
            } else {
                // If the group was entered again since the atomic_add above,
                // we can't clear the waiters bit anymore as we don't know for
                // which generation the waiters are for
                
                new_state &= ~DISPATCH_GROUP_HAS_NOTIFS;
            }
            
            // 如果目前是仅关联了一个 block 而且是正常的 enter 和 leave 配对执行，则会执行这里的 break，
            // 结束 do while 循环，执行下面的 _dispatch_group_wake 函数，唤醒执行 dispatch_group_notify 添加到指定队列中的回调函数。
            if (old_state == new_state) break;
            
        // 比较 dg_state 和 old_state 的值，如果相等则把 dg_state 的值存入 new_state 中，如果不相等则把 dg_state 的值存入 old_state 中
        } while (unlikely(!os_atomic_cmpxchgv2o(dg, dg_state, old_state, new_state, &old_state, relaxed)));
                
        // 唤醒执行 dispatch_group_notify 添加到指定队列中的回调函数
        return _dispatch_group_wake(dg, old_state, true);
    }

    // 如果 old_value 为 0，而上面又进行了一个 dg_state + DISPATCH_GROUP_VALUE_INTERVAL 操作，此时就过度 leave 了，则 crash。
    // 例如创建好一个 dispatch_group 后直接调用 dispatch_group_leave 函数即会触发这个 crash。
    if (unlikely(old_value == 0)) {
        DISPATCH_CLIENT_CRASH((uintptr_t)old_value, "Unbalanced call to dispatch_group_leave()");
    }
}
```
#### os_atomic_add_orig2o
&emsp;`os_atomic_add_orig2o` 宏定义中 `f` 是 `p` 的成员变量，内部的 `atomic_fetch_add_explicit` 函数保证以原子方式把 `f` 的值增加 `v`。
```c++
#define os_atomic_add_orig2o(p, f, v, m) \
        os_atomic_add_orig(&(p)->f, (v), m)

#define os_atomic_add_orig(p, v, m) \
        _os_atomic_c11_op_orig((p), (v), m, add, +)
        
#define _os_atomic_c11_op_orig(p, v, m, o, op) \
        atomic_fetch_##o##_explicit(_os_atomic_c11_atomic(p), v, \
        memory_order_##m)
```
#### os_atomic_cmpxchgv2o
&emsp;`os_atomic_cmpxchgv2o` 宏定义中 `f` 是 `&(p)` 的成员变量，比较 `&(p)->f` 和 `e` 的值，如果相等则用 `&(p)->f` 的值替换 `v` 的值，如果不相等，则把 `&(p)->f` 的值存入 `e` 中。
```c++
#define os_atomic_cmpxchgv2o(p, f, e, v, g, m) \
        os_atomic_cmpxchgv(&(p)->f, (e), (v), (g), m)

#define _os_atomic_basetypeof(p) \
        __typeof__(atomic_load_explicit(_os_atomic_c11_atomic(p), memory_order_relaxed))

#define _os_atomic_c11_atomic(p) \
        ((__typeof__(*(p)) _Atomic *)(p))
        
#define os_atomic_cmpxchgv(p, e, v, g, m) \
        ({ _os_atomic_basetypeof(p) _r = (e); _Bool _b = \
        atomic_compare_exchange_strong_explicit(_os_atomic_c11_atomic(p), \
        &_r, v, memory_order_##m, memory_order_relaxed); *(g) = _r; _b; })
```
### dispatch_group_async
&emsp;`dispatch_group_async` 
```c++
#ifdef __BLOCKS__
void
dispatch_group_async(dispatch_group_t dg, dispatch_queue_t dq,
        dispatch_block_t db)
{
    dispatch_continuation_t dc = _dispatch_continuation_alloc();
    uintptr_t dc_flags = DC_FLAG_CONSUME | DC_FLAG_GROUP_ASYNC;
    dispatch_qos_t qos;

    qos = _dispatch_continuation_init(dc, dq, db, 0, dc_flags);
    _dispatch_continuation_group_async(dg, dq, dc, qos);
}
#endif
```
#### _dispatch_continuation_group_async
&emsp;
```c++
DISPATCH_ALWAYS_INLINE
static inline void
_dispatch_continuation_group_async(dispatch_group_t dg, dispatch_queue_t dq,
        dispatch_continuation_t dc, dispatch_qos_t qos)
{
    // 进组
    dispatch_group_enter(dg);
    
    dc->dc_data = dg;
    
    _dispatch_continuation_async(dq, dc, qos, dc->dc_flags);
}
```
##### _dispatch_continuation_async
&emsp;
```c++
DISPATCH_ALWAYS_INLINE
static inline void
_dispatch_continuation_async(dispatch_queue_class_t dqu,
        dispatch_continuation_t dc, dispatch_qos_t qos, uintptr_t dc_flags)
{
#if DISPATCH_INTROSPECTION
    if (!(dc_flags & DC_FLAG_NO_INTROSPECTION)) {
        _dispatch_trace_item_push(dqu, dc);
    }
#else
    (void)dc_flags;
#endif
    return dx_push(dqu._dq, dc, qos);
}
```
##### 
```c++
DISPATCH_ALWAYS_INLINE
static inline void
_dispatch_continuation_with_group_invoke(dispatch_continuation_t dc)
{
    struct dispatch_object_s *dou = dc->dc_data;
    unsigned long type = dx_type(dou);
    if (type == DISPATCH_GROUP_TYPE) {
        _dispatch_client_callout(dc->dc_ctxt, dc->dc_func);
        _dispatch_trace_item_complete(dc);
        
        // 出组
        dispatch_group_leave((dispatch_group_t)dou);
    } else {
        DISPATCH_INTERNAL_CRASH(dx_type(dou), "Unexpected object type");
    }
}
```
### dispatch_group_notify
&emsp;`dispatch_group_notify` 当与 dispatch_group 相关联的所有 block 都已完成时，计划将 `db` 提交到队列 `dq`（即当与 dispatch_group 相关联的所有 block 都已完成时，提交到 `dq` 的 `db` 将执行）。如果没有 block 与 dispatch_group 相关联（即该 dispatch_group 为空），则通知块 `db` 将立即提交。

&emsp;通知块 `db` 提交到目标队列 `dq` 时，该 dispatch_group 将为空。该 dispatch_group 可以通过 `dispatch_release` 释放，也可以重新用于其他操作。

&emsp;`dispatch_group_notify` 函数不会阻塞当前线程，此函数会立即返回，如果我们想阻塞当前线程，想要等 dispatch_group 中关联的 block 全部执行完成后才执行接下来的操作时，可以使用 `dispatch_group_wait` 函数并指定具体的等待时间（`DISPATCH_TIME_FOREVER`）。
```c++
#ifdef __BLOCKS__
void
dispatch_group_notify(dispatch_group_t dg, dispatch_queue_t dq,
        dispatch_block_t db)
{
    // 从缓存中取一个 dispatch_continuation_t 或者新建一个 dispatch_continuation_t 返回
    dispatch_continuation_t dsn = _dispatch_continuation_alloc();
    // 配置 dsn，（db 转换为函数）
    _dispatch_continuation_init(dsn, dq, db, 0, DC_FLAG_CONSUME);
    
    // 
    _dispatch_group_notify(dg, dq, dsn);
}
#endif
```
#### _dispatch_group_notify
&emsp;
```c++
DISPATCH_ALWAYS_INLINE
static inline void
_dispatch_group_notify(dispatch_group_t dg, dispatch_queue_t dq,
        dispatch_continuation_t dsn)
{
    // 临时变量，
    uint64_t old_state, new_state;
    dispatch_continuation_t prev;

    // 任务的 dc_data 成员变量赋值为任务执行时所在的队列 
    dsn->dc_data = dq;
    
    // dq 队列引用计数 +1 
    _dispatch_retain(dq);

    prev = os_mpsc_push_update_tail(os_mpsc(dg, dg_notify), dsn, do_next);
    
    if (os_mpsc_push_was_empty(prev)) _dispatch_retain(dg);
    
    os_mpsc_push_update_prev(os_mpsc(dg, dg_notify), prev, dsn, do_next);
    
    if (os_mpsc_push_was_empty(prev)) {
    
        // 一直循环等待直到 old_state == 0
        os_atomic_rmw_loop2o(dg, dg_state, old_state, new_state, release, {
            new_state = old_state | DISPATCH_GROUP_HAS_NOTIFS;
            
            if ((uint32_t)old_state == 0) {
            
                os_atomic_rmw_loop_give_up({
                    // 调用 _dispatch_group_wake
                    return _dispatch_group_wake(dg, new_state, false);
                });
            }
        });
    }
}
```
&emsp;
### _dispatch_group_wake
&emsp;
```c++
DISPATCH_NOINLINE
static void
_dispatch_group_wake(dispatch_group_t dg, uint64_t dg_state, bool needs_release)
{
    // 
    uint16_t refs = needs_release ? 1 : 0; // <rdar://problem/22318411>
    
    // #define DISPATCH_GROUP_HAS_NOTIFS   0x0000000000000002ULL
    
    if (dg_state & DISPATCH_GROUP_HAS_NOTIFS) {
        dispatch_continuation_t dc, next_dc, tail;

        // Snapshot before anything is notified/woken <rdar://problem/8554546>
        dc = os_mpsc_capture_snapshot(os_mpsc(dg, dg_notify), &tail);
        
        do {
            dispatch_queue_t dsn_queue = (dispatch_queue_t)dc->dc_data;
            next_dc = os_mpsc_pop_snapshot_head(dc, tail, do_next);
            
            // 异步执行函数
            _dispatch_continuation_async(dsn_queue, dc,
                    _dispatch_qos_from_pp(dc->dc_priority), dc->dc_flags);
                    
            // 释放 dsn_queue
            _dispatch_release(dsn_queue);
        } while ((dc = next_dc));

        refs++;
    }
    
    // 
    if (dg_state & DISPATCH_GROUP_HAS_WAITERS) {
        _dispatch_wake_by_address(&dg->dg_gen);
    }

    // 
    if (refs) _dispatch_release_n(dg, refs);
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
+ [iOS源码解析: GCD的信号量semaphore](https://juejin.cn/post/6844904122370490375)

