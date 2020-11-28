# iOS 多线程知识体系构建(八)：GCD 源码：自定义队列创建

> &emsp;上篇主要看了源码中基础的数据结构以及和队列相关的一些内容，那么本篇就从创建自定义队列作为主线，过程中遇到新的数据结构时展开作为支线来学习，那么开始吧！⛽️⛽️

&emsp;在 GCD 中使用最多的三种队列：主队列（`dispatch_get_main_queue()`）、全局并发队列（`dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0)`）、自定义队列（`dispatch_queue_create`），那么我们就先由创建自定义队列开始学习。
## dispatch_queue_create
&emsp;下面就沿着源码一路看队列的创建过程。
```c++
// 创建一个并发队列
dispatch_queue_t concurrentQueue = dispatch_queue_create("com.concurrent", DISPATCH_QUEUE_CONCURRENT);
// 创建一个串行队列
dispatch_queue_t serialQueue = dispatch_queue_create("com.serial", DISPATCH_QUEUE_SERIAL);
```
### DISPATCH_QUEUE_SERIAL 
&emsp;用于创建以 FIFO 顺序串行调用块的调度队列（串行队列）的属性，值是 `NULL`。
```c++
#define DISPATCH_QUEUE_SERIAL NULL
```
### DISPATCH_QUEUE_CONCURRENT
&emsp;可用于创建调度队列（并发队列）的属性，该调度队列可同时调用块并支持通过调度屏障 API （`dispatch_barrier_async`）提交的屏障块。(常规 block 和 barrier 的 block 任务块)
```c++
#define DISPATCH_GLOBAL_OBJECT(type, object) ((OS_OBJECT_BRIDGE type)&(object))

#define DISPATCH_QUEUE_CONCURRENT \
        DISPATCH_GLOBAL_OBJECT(dispatch_queue_attr_t, _dispatch_queue_attr_concurrent)
API_AVAILABLE(macos(10.7), ios(4.3))
DISPATCH_EXPORT
struct dispatch_queue_attr_s _dispatch_queue_attr_concurrent; // 这里有一个 dispatch_queue_attr_s 结构体类型的全局变量。
```
&emsp;`DISPATCH_QUEUE_CONCURRENT` 宏定义是把全局变量 `_dispatch_queue_attr_concurrent` 强制转化为了 `dispatch_queue_attr_t` 类型的变量。

&emsp;`dispatch_queue_create` 函数的实现。`label` 参数是要附加到队列的自定义的字符串标签，`attr` 参数是预定义属性，`DISPATCH_QUEUE_SERIAL`、`DISPATCH_QUEUE_CONCURRENT` 或调用 `dispatch_queue_attr_make_with_*` 函数的自定义创建的 `dispatch_queue_attr_t` 结构体实例。
```c++
dispatch_queue_t
dispatch_queue_create(const char *label, dispatch_queue_attr_t attr) {
    return _dispatch_lane_create_with_target(label, attr, DISPATCH_TARGET_QUEUE_DEFAULT, true);
}
```
&emsp;`dispatch_queue_create` 函数内部调用了一个中间函数 `_dispatch_lane_create_with_target`，其中用了一个 `DISPATCH_TARGET_QUEUE_DEFAULT` 作为默认参数。
### DISPATCH_TARGET_QUEUE_DEFAULT
&emsp;`DISPATCH_TARGET_QUEUE_DEFAULT` 是传递给 `dispatch_queue_create_with_target`、`dispatch_set_target_queue` 和 `dispatch_source_create` 函数的常量，以指示应使用（相关对象类型的）默认目标队列，它的实际值是 `NULL`。
```c++
#define DISPATCH_TARGET_QUEUE_DEFAULT NULL
```
### dispatch_lane_t
&emsp;`dispatch_lane_t` 是指向 `dispatch_lane_s` 结构体的指针。
```c++
typedef struct dispatch_lane_s {
    DISPATCH_LANE_CLASS_HEADER(lane);
    /* 32bit hole on LP64 */
} DISPATCH_ATOMIC64_ALIGN *dispatch_lane_t;
```
#### DISPATCH_LANE_CLASS_HEADER
```c++
#define DISPATCH_LANE_CLASS_HEADER(x) \
    struct dispatch_queue_s _as_dq[0]; \
    DISPATCH_QUEUE_CLASS_HEADER(x, \
            struct dispatch_object_s *volatile dq_items_tail); \
    dispatch_unfair_lock_s dq_sidelock; \
    struct dispatch_object_s *volatile dq_items_head; \
    uint32_t dq_side_suspend_cnt
```
&emsp;把 `dispatch_lane_s` 定义中的宏完全展开的话：
```c++
typedef struct dispatch_lane_s {
    // 此处两行则是 dispatch_lane_s 继承的父类 dispatch_queue_s 的头部内容
    struct dispatch_queue_s _as_dq[0];
    struct dispatch_object_s _as_do[0];
    struct _os_object_s _as_os_obj[0];
    
    const struct dispatch_lane_vtable_s *do_vtable; /* must be pointer-sized */
    int volatile do_ref_cnt;
    int volatile do_xref_cnt;
    
    struct dispatch_lane_s *volatile do_next;
    struct dispatch_queue_s *do_targetq;
    void *do_ctxt;
    void *do_finalizer
    
    struct dispatch_object_s *volatile dq_items_tail;
    
    union { 
        uint64_t volatile dq_state;
        struct {
            dispatch_lock dq_state_lock;
            uint32_t dq_state_bits;
        };
    };
    
    /* LP64 global queue cacheline boundary */
    unsigned long dq_serialnum;
    const char *dq_label;
    
    union { 
        uint32_t volatile dq_atomic_flags;
        struct {
            const uint16_t dq_width; // 队列的宽度（串行队列为 1，并发队列大于 1）
            const uint16_t __dq_opaque2;
        };
    };
    
    dispatch_priority_t dq_priority;
    union {
        struct dispatch_queue_specific_head_s *dq_specific_head;
        struct dispatch_source_refs_s *ds_refs;
        struct dispatch_timer_source_refs_s *ds_timer_refs;
        struct dispatch_mach_recv_refs_s *dm_recv_refs;
        struct dispatch_channel_callbacks_s const *dch_callbacks;
    };
    int volatile dq_sref_cnt;
    
    dispatch_unfair_lock_s dq_sidelock; // 锁
    struct dispatch_object_s *volatile dq_items_head; // 头
    uint32_t dq_side_suspend_cnt // 挂起次数
    
} DISPATCH_ATOMIC64_ALIGN *dispatch_lane_t;
```
&emsp;可看到 `dispatch_lane_s` 是继承自 `dispatch_queue_s` 的“子类”，且 `_dispatch_lane_create_with_target` 函数返回的正是 `dispatch_lane_s` 而不是 `dispatch_queue_s` 类型。
### DISPATCH_QUEUE_WIDTH_MAX
```c++
#define DISPATCH_QUEUE_WIDTH_FULL            0x1000ull //（4096）为创建全局队列时候所使用的
#define DISPATCH_QUEUE_WIDTH_POOL (DISPATCH_QUEUE_WIDTH_FULL - 1) // 0xfffull（4095）
#define DISPATCH_QUEUE_WIDTH_MAX  (DISPATCH_QUEUE_WIDTH_FULL - 2) // 0xffeull // 队列宽度的最大值 （4094）
```
### _dispatch_priority_make
&emsp;优先级及相对量。
```c++
#define _dispatch_priority_make(qos, relpri) \
    (qos ? ((((qos) << DISPATCH_PRIORITY_QOS_SHIFT) & DISPATCH_PRIORITY_QOS_MASK) | \
    ((dispatch_priority_t)(relpri - 1) & DISPATCH_PRIORITY_RELPRI_MASK)) : 0)
```

&emsp;`_dispatch_lane_create_with_target` 函数实现：
```c++
DISPATCH_NOINLINE
static dispatch_queue_t
_dispatch_lane_create_with_target(const char *label, dispatch_queue_attr_t dqa,
        dispatch_queue_t tq, bool legacy)
{
    // _dispatch_queue_attr_to_info 函数上篇我们讲解过，
    // 1. 如果 dqa 是 DISPATCH_QUEUE_SERIAL（值是 NULL）作为入参传入的话，
    //    会直接返回一个空的 dispatch_queue_attr_info_t 结构体实例，（dispatch_queue_attr_info_t dqai = { };）。
    // 2. 如果 dqa 是 DISPATCH_QUEUE_CONCURRENT（值是全局变量 _dispatch_queue_attr_concurrent）作为入参传入的话，
    //    会返回一个 dqai_concurrent 值是 true 的 dispatch_queue_attr_info_t 结构体实例，（dqai_concurrent 为 true 表示是并发队列）。
    // 3. 第三种情况则是传入自定义的 dispatch_queue_attr_t 时，
    //    则会进行取模和取商运算为 dispatch_queue_attr_info_t 结构体实例的每个成员变量赋值后返回该 dispatch_queue_attr_info_t 结构体实例。
    
    dispatch_queue_attr_info_t dqai = _dispatch_queue_attr_to_info(dqa);

    //
    // Step 1: Normalize arguments (qos, overcommit, tq) 规范化参数
    //

    dispatch_qos_t qos = dqai.dqai_qos; //（dqai_qos 表示线程优先级）
    
    // 如果 HAVE_PTHREAD_WORKQUEUE_QOS 为假会进行一个 dqai_qos 的切换
#if !HAVE_PTHREAD_WORKQUEUE_QOS
    if (qos == DISPATCH_QOS_USER_INTERACTIVE) {
        // 如果是 "用户交互" 这个最高优先级，则切到 "用户启动" 这个第二优先级
        dqai.dqai_qos = qos = DISPATCH_QOS_USER_INITIATED;
    }
    if (qos == DISPATCH_QOS_MAINTENANCE) {
        // 如果是 "QOS_CLASS_MAINTENANCE" 这个最低优先级，则切到 "后台线程" 这个倒数第二优先级
        dqai.dqai_qos = qos = DISPATCH_QOS_BACKGROUND;
    }
#endif // !HAVE_PTHREAD_WORKQUEUE_QOS

    // 取出是否允许 "过量使用（超过物理上的核心数）"
    _dispatch_queue_attr_overcommit_t overcommit = dqai.dqai_overcommit;
    if (overcommit != _dispatch_queue_attr_overcommit_unspecified && tq) {
        // 如果 overcommit 不等于 "未指定 overcommit" 并且 tq 不为空
        //（已知上面 dispatch_queue_create 函数调用默认入参 DISPATCH_TARGET_QUEUE_DEFAULT 是 NULL）
        if (tq->do_targetq) {
            // crash
            DISPATCH_CLIENT_CRASH(tq, "Cannot specify both overcommit and "
                    "a non-global target queue");
        }
    }

    if (tq && dx_type(tq) == DISPATCH_QUEUE_GLOBAL_ROOT_TYPE) {
        // Handle discrepancies between attr and target queue, attributes win
        // 处理 attr 和目标队列之间的差异，以 attr 为主
        
        // 如果目标队列存在，且目标队列是全局根队列
        if (overcommit == _dispatch_queue_attr_overcommit_unspecified) {
            // 如果 overcommit 是未指定
            if (tq->dq_priority & DISPATCH_PRIORITY_FLAG_OVERCOMMIT) {
                // 如果目标队列的优先级是 DISPATCH_PRIORITY_FLAG_OVERCOMMIT，则把 overcommit 置为允许
                overcommit = _dispatch_queue_attr_overcommit_enabled;
            } else {
                // 否则是不允许
                overcommit = _dispatch_queue_attr_overcommit_disabled;
            }
        }
        
        // 如果优先级未指定，则新创建的队列的优先级继承目标队列的优先级
        if (qos == DISPATCH_QOS_UNSPECIFIED) {
            qos = _dispatch_priority_qos(tq->dq_priority);
        }
        
        // tq 置 NULL
        tq = NULL;
    } else if (tq && !tq->do_targetq) {
        // target is a pthread or runloop root queue, setting QoS or overcommit is disallowed
        // target queue 是一个 pthread 或 runloop root queue， 设置 QoS 或 overcommit 是不允许的
        
        if (overcommit != _dispatch_queue_attr_overcommit_unspecified) {
            // 如果 tq 存在且 overcommit 不是未指定的话，则 crash
            DISPATCH_CLIENT_CRASH(tq, "Cannot specify an overcommit attribute "
                    "and use this kind of target queue");
        }
    } else {
        // tq 为 NULL 的情况
        
        if (overcommit == _dispatch_queue_attr_overcommit_unspecified) {
            // Serial queues default to overcommit! (串行队列默认为 overcommit)
            // 根据上面的入参知道，串行队列的 dqai_concurrent 为 false，并发队列的 dqai_concurrent 为 true。
            
            // 当 dqai.dqai_concurrent 为 true，不允许 overcommit，否则允许 overcommit
            overcommit = dqai.dqai_concurrent ?
                    _dispatch_queue_attr_overcommit_disabled :
                    _dispatch_queue_attr_overcommit_enabled;
        }
    }
    
    // 当 tq 为 NULL，即入参目标队列为 DISPATCH_TARGET_QUEUE_DEFAULT（值是 NULL） 时，
    // 根据 qos 和 overcommit 从 _dispatch_root_queues 全局的根队列数组中获取一个根队列作为新队列的目标队列
    if (!tq) {
        tq = _dispatch_get_root_queue(
                qos == DISPATCH_QOS_UNSPECIFIED ? DISPATCH_QOS_DEFAULT : qos,
                overcommit == _dispatch_queue_attr_overcommit_enabled)->_as_dq;
                
        if (unlikely(!tq)) {
            // 如果未取得目标队列则 crash
            DISPATCH_CLIENT_CRASH(qos, "Invalid queue attribute");
        }
    }

    //
    // Step 2: Initialize the queue（初始化队列）
    //
    
    // dispatch_queue_create 函数的调用中，legacy 默认传的是 true
    if (legacy) {
        // if any of these attributes is specified, use non legacy classes
        // 如果指定了这些属性中的任何一个，请使用非旧类
        
        // 活动状态（dqai_inactive）和自动释放频率（dqai_autorelease_frequency）
        if (dqai.dqai_inactive || dqai.dqai_autorelease_frequency) {
            legacy = false;
        }
    }

    const void *vtable;
    dispatch_queue_flags_t dqf = legacy ? DQF_MUTABLE : 0;
    if (dqai.dqai_concurrent) {
        // 并发队列
        vtable = DISPATCH_VTABLE(queue_concurrent); // _dispatch_queue_concurrent_vtable 包裹队列可进行的函数调用
    } else {
        // 串行队列
        vtable = DISPATCH_VTABLE(queue_serial); // _dispatch_queue_serial_vtable 包裹队列可进行的函数调用
    }
    
    // 自动释放频率
    switch (dqai.dqai_autorelease_frequency) {
    case DISPATCH_AUTORELEASE_FREQUENCY_NEVER:
        dqf |= DQF_AUTORELEASE_NEVER;
        break;
    case DISPATCH_AUTORELEASE_FREQUENCY_WORK_ITEM:
        dqf |= DQF_AUTORELEASE_ALWAYS;
        break;
    }
    
    // 队列标签
    if (label) {
        // _dispatch_strdup_if_mutable 函数的功能：如果 label 入参是可变的字符串则申请空间并复制原始字符串进入，如果 label 入参是不可变字符串则直接返回原始值
        const char *tmp = _dispatch_strdup_if_mutable(label);
        
        if (tmp != label) {
            // 新申请了空间
            dqf |= DQF_LABEL_NEEDS_FREE;
            // "新值" 赋给 label
            label = tmp;
        }
    }

    // void *_dispatch_object_alloc(const void *vtable, size_t size) 函数未找到其定义，只在 object_internal.h 中看到其声明。
    // dispatch_lane_s 是 dispatch_queue_s 的子类。
    
    // dq 是一个指向 dispatch_lane_s 结构体的指针
    dispatch_lane_t dq = _dispatch_object_alloc(vtable,
            sizeof(struct dispatch_lane_s));
            
    // 当 dqai.dqai_concurrent 为真时入参为 DISPATCH_QUEUE_WIDTH_MAX（4094）否则是 1
    // 当 dqai.dqai_inactive 为真时表示非活动状态，否则为活动状态
    // #define DISPATCH_QUEUE_ROLE_INNER            0x0000000000000000ull
    // #define DISPATCH_QUEUE_INACTIVE              0x0180000000000000ull
    
    // 初始化 dq
    _dispatch_queue_init(dq, dqf, dqai.dqai_concurrent ?
            DISPATCH_QUEUE_WIDTH_MAX : 1, DISPATCH_QUEUE_ROLE_INNER |
            (dqai.dqai_inactive ? DISPATCH_QUEUE_INACTIVE : 0));

    // 队列签名
    dq->dq_label = label;
    // 优先级
    dq->dq_priority = _dispatch_priority_make((dispatch_qos_t)dqai.dqai_qos,
            dqai.dqai_relpri);
    // overcommit
    if (overcommit == _dispatch_queue_attr_overcommit_enabled) {
        dq->dq_priority |= DISPATCH_PRIORITY_FLAG_OVERCOMMIT;
    }
    
    // 如果是非活动状态
    if (!dqai.dqai_inactive) {
        // 新队列的优先级继承自目标队列优先级
        _dispatch_queue_priority_inherit_from_target(dq, tq);
        _dispatch_lane_inherit_wlh_from_target(dq, tq);
    }
    
    // 目标队列的内部引用计数加 1（原子操作）
    _dispatch_retain(tq);
    
    // 设置新队列的目标队列
    dq->do_targetq = tq;
    
    // DEBUG 时的打印函数
    _dispatch_object_debug(dq, "%s", __func__);
    return _dispatch_trace_queue_create(dq)._dq;
}
```
&emsp;`_dispatch_lane_create_with_target` 函数的执行流程如注释所示，下面我们摘录其中的较关键点再进行分析。

## _dispatch_get_root_queue
&emsp;当 `tq` 不存在时，会调用 `_dispatch_get_root_queue` 返回一个 `dispatch_queue_global_t` 赋值给 `tq`。在 `dispatch_queue_create` 函数调用中 `tq` 传了一个默认值：`DISPATCH_TARGET_QUEUE_DEFAULT`（它的实际值是 `NULL`），所以当我们创建串行队列或者并发队列的时候都会调用 `_dispatch_get_root_queue` 函数来获取一个目标队列。
```c++
// 当 tq 为 NULL，即入参目标队列为 DISPATCH_TARGET_QUEUE_DEFAULT（值是 NULL） 时，
// 根据 qos 和 overcommit 从 _dispatch_root_queues 全局的根队列数组中获取一个根队列作为新队列的目标队列
if (!tq) {
    tq = _dispatch_get_root_queue(
            qos == DISPATCH_QOS_UNSPECIFIED ? DISPATCH_QOS_DEFAULT : qos,
            overcommit == _dispatch_queue_attr_overcommit_enabled)->_as_dq;
            
    if (unlikely(!tq)) {
        // 如果未取得目标队列则 crash
        DISPATCH_CLIENT_CRASH(qos, "Invalid queue attribute");
    }
}
```
&emsp;`_dispatch_get_root_queue` 函数有两个参数分别是 `qos` 和 `overcommit`，下面我们分析一下，当 `dqa` 为 `DISPATCH_QUEUE_SERIAL` 或 `DISPATCH_QUEUE_CONCURRENT` 时 `_dispatch_get_root_queue` 函数所使用的两个参数值各是什么。

+ `qos`：分析 `_dispatch_queue_attr_to_info` 时我们已知当 `dqa` 为 `DISPATCH_QUEUE_SERIAL` 或者 `DISPATCH_QUEUE_CONCURRENT` 时，都不会对返回的 `dispatch_queue_attr_info_t` 结构体实例的 `dqai_qos` 成员变量赋值所以 `dqai_qos` 的值是 0，即为 `DISPATCH_QOS_UNSPECIFIED`（`#define DISPATCH_QOS_UNSPECIFIED ((dispatch_qos_t)0)`），那么 `qos == DISPATCH_QOS_UNSPECIFIED ? DISPATCH_QOS_DEFAULT : qos` 的值即为 `DISPATCH_QOS_DEFAULT`（`#define DISPATCH_QOS_DEFAULT ((dispatch_qos_t)4)`），即不管是创建串行队列还是并发队列，当调用 `_dispatch_get_root_queue` 函数时 `qos` 用的都是 4。
+ `overcommit`：分析 `_dispatch_queue_attr_to_info` 时已知，串行队列时 `dqai.dqai_concurrent` 值为 `false`，并发队列时是 `true`，即当 `dqa` 是 `DISPATCH_QUEUE_SERIAL` 时，`overcommit` 的值是 `_dispatch_queue_attr_overcommit_enabled`，当 `dqa` 是 `DISPATCH_QUEUE_CONCURRENT` 时，`overcommit` 的值是 `_dispatch_queue_attr_overcommit_disabled`。

&emsp;`dqa` 是 `DISPATCH_QUEUE_SERIAL` 时:
```c++
tq = _dispatch_get_root_queue(4, true)->_as_dq;
```
&emsp;`dqa` 是 `DISPATCH_QUEUE_CONCURRENT` 时：
```c++
tq = _dispatch_get_root_queue(4, false)->_as_dq;
```
&emsp;`_dispatch_get_root_queue` 函数的实现很简单，仅是根据下标从 `_dispatch_root_queues` 根队列数组中取指定的队列而已。
```c++
DISPATCH_ALWAYS_INLINE DISPATCH_CONST
static inline dispatch_queue_global_t
_dispatch_get_root_queue(dispatch_qos_t qos, bool overcommit)
{
    // DISPATCH_QOS_MAX = 6
    // DISPATCH_QOS_MIN = 1
    
    if (unlikely(qos < DISPATCH_QOS_MIN || qos > DISPATCH_QOS_MAX)) {
        DISPATCH_CLIENT_CRASH(qos, "Corrupted priority");
    }
    
    return &_dispatch_root_queues[2 * (qos - 1) + overcommit];
}
```
+ `DISPATCH_QUEUE_SERIAL` 时目标队列是: `&_dispatch_root_queues[7]`（com.apple.root.default-qos.overcommit）。
+ `DISPATCH_QUEUE_CONCURRENT` 时目标队列是: `&_dispatch_root_queues[6]`（com.apple.root.default-qos）

## _dispatch_root_queues
```c++
// 6618342 Contact the team that owns the Instrument DTrace probe before
//         renaming this symbol
struct dispatch_queue_global_s _dispatch_root_queues[] = {
#define _DISPATCH_ROOT_QUEUE_IDX(n, flags) \
        ((flags & DISPATCH_PRIORITY_FLAG_OVERCOMMIT) ? \
        DISPATCH_ROOT_QUEUE_IDX_##n##_QOS_OVERCOMMIT : \
        DISPATCH_ROOT_QUEUE_IDX_##n##_QOS)
#define _DISPATCH_ROOT_QUEUE_ENTRY(n, flags, ...) \
    [_DISPATCH_ROOT_QUEUE_IDX(n, flags)] = { \
        DISPATCH_GLOBAL_OBJECT_HEADER(queue_global), \
        .dq_state = DISPATCH_ROOT_QUEUE_STATE_INIT_VALUE, \
        .do_ctxt = _dispatch_root_queue_ctxt(_DISPATCH_ROOT_QUEUE_IDX(n, flags)), \
        .dq_atomic_flags = DQF_WIDTH(DISPATCH_QUEUE_WIDTH_POOL), \
        .dq_priority = flags | ((flags & DISPATCH_PRIORITY_FLAG_FALLBACK) ? \
                _dispatch_priority_make_fallback(DISPATCH_QOS_##n) : \
                _dispatch_priority_make(DISPATCH_QOS_##n, 0)), \
        __VA_ARGS__ \
    }
    _DISPATCH_ROOT_QUEUE_ENTRY(MAINTENANCE, 0,
        .dq_label = "com.apple.root.maintenance-qos",
        .dq_serialnum = 4,
    ),
    _DISPATCH_ROOT_QUEUE_ENTRY(MAINTENANCE, DISPATCH_PRIORITY_FLAG_OVERCOMMIT,
        .dq_label = "com.apple.root.maintenance-qos.overcommit",
        .dq_serialnum = 5,
    ),
    _DISPATCH_ROOT_QUEUE_ENTRY(BACKGROUND, 0,
        .dq_label = "com.apple.root.background-qos",
        .dq_serialnum = 6,
    ),
    _DISPATCH_ROOT_QUEUE_ENTRY(BACKGROUND, DISPATCH_PRIORITY_FLAG_OVERCOMMIT,
        .dq_label = "com.apple.root.background-qos.overcommit",
        .dq_serialnum = 7,
    ),
    _DISPATCH_ROOT_QUEUE_ENTRY(UTILITY, 0,
        .dq_label = "com.apple.root.utility-qos",
        .dq_serialnum = 8,
    ),
    _DISPATCH_ROOT_QUEUE_ENTRY(UTILITY, DISPATCH_PRIORITY_FLAG_OVERCOMMIT,
        .dq_label = "com.apple.root.utility-qos.overcommit",
        .dq_serialnum = 9,
    ),
    _DISPATCH_ROOT_QUEUE_ENTRY(DEFAULT, DISPATCH_PRIORITY_FLAG_FALLBACK,
        .dq_label = "com.apple.root.default-qos",
        .dq_serialnum = 10,
    ),
    _DISPATCH_ROOT_QUEUE_ENTRY(DEFAULT,
            DISPATCH_PRIORITY_FLAG_FALLBACK | DISPATCH_PRIORITY_FLAG_OVERCOMMIT,
        .dq_label = "com.apple.root.default-qos.overcommit",
        .dq_serialnum = 11,
    ),
    _DISPATCH_ROOT_QUEUE_ENTRY(USER_INITIATED, 0,
        .dq_label = "com.apple.root.user-initiated-qos",
        .dq_serialnum = 12,
    ),
    _DISPATCH_ROOT_QUEUE_ENTRY(USER_INITIATED, DISPATCH_PRIORITY_FLAG_OVERCOMMIT,
        .dq_label = "com.apple.root.user-initiated-qos.overcommit",
        .dq_serialnum = 13,
    ),
    _DISPATCH_ROOT_QUEUE_ENTRY(USER_INTERACTIVE, 0,
        .dq_label = "com.apple.root.user-interactive-qos",
        .dq_serialnum = 14,
    ),
    _DISPATCH_ROOT_QUEUE_ENTRY(USER_INTERACTIVE, DISPATCH_PRIORITY_FLAG_OVERCOMMIT,
        .dq_label = "com.apple.root.user-interactive-qos.overcommit",
        .dq_serialnum = 15,
    ),
};
```
## _dispatch_queue_init
&emsp;`dispatch_lane_s` 结构体实例创建完成后，调用了 `_dispatch_queue_init` 函数进行初始化操作。
```c++
...
// dq 是一个指向 dispatch_lane_s 结构体的指针
dispatch_lane_t dq = _dispatch_object_alloc(vtable,
        sizeof(struct dispatch_lane_s));
        
// 当 dqai.dqai_concurrent 为真时入参为 DISPATCH_QUEUE_WIDTH_MAX（4094）否则是 1，即串行队列时是 1，并发队列时是 4094
// 当 dqai.dqai_inactive 为真时表示非活动状态，否则为活动状态
// #define DISPATCH_QUEUE_ROLE_INNER            0x0000000000000000ull
// #define DISPATCH_QUEUE_INACTIVE              0x0180000000000000ull

// 初始化 dq
_dispatch_queue_init(dq, dqf, dqai.dqai_concurrent ?
        DISPATCH_QUEUE_WIDTH_MAX : 1, DISPATCH_QUEUE_ROLE_INNER |
        (dqai.dqai_inactive ? DISPATCH_QUEUE_INACTIVE : 0));
...
```
### dispatch_queue_class_t
&emsp;`dispatch_queue_class_t` 是一个透明联合类型，且每个成员变量都是指向 `dispatch_queue_s` 结构体的子类的指针。
```c++
// Dispatch queue cluster class: type for any dispatch_queue_t
// 调度队列群集类：包含任何 dispatch_queue_t
typedef union {
    struct dispatch_queue_s *_dq;
    struct dispatch_workloop_s *_dwl;
    struct dispatch_lane_s *_dl;
    struct dispatch_queue_static_s *_dsq;
    struct dispatch_queue_global_s *_dgq;
    struct dispatch_queue_pthread_root_s *_dpq;
    struct dispatch_source_s *_ds;
    struct dispatch_channel_s *_dch;
    struct dispatch_mach_s *_dm;
    dispatch_lane_class_t _dlu;
#ifdef __OBJC__
    id<OS_dispatch_queue> _objc_dq;
#endif
} dispatch_queue_class_t DISPATCH_TRANSPARENT_UNION;
```

&emsp;`_dispatch_queue_init` 函数实现:
```c++
// Note to later developers: ensure that any initialization changes are made for statically allocated queues (i.e. _dispatch_main_q).

static inline dispatch_queue_class_t
_dispatch_queue_init(dispatch_queue_class_t dqu, dispatch_queue_flags_t dqf,
        uint16_t width, uint64_t initial_state_bits)
{
    uint64_t dq_state = DISPATCH_QUEUE_STATE_INIT_VALUE(width);
    dispatch_queue_t dq = dqu._dq;

    dispatch_assert((initial_state_bits & ~(DISPATCH_QUEUE_ROLE_MASK |
            DISPATCH_QUEUE_INACTIVE)) == 0);

    if (initial_state_bits & DISPATCH_QUEUE_INACTIVE) {
        dq->do_ref_cnt += 2; // rdar://8181908 see _dispatch_lane_resume
        if (dx_metatype(dq) == _DISPATCH_SOURCE_TYPE) {
            dq->do_ref_cnt++; // released when DSF_DELETED is set
        }
    }

    dq_state |= initial_state_bits;
    dq->do_next = DISPATCH_OBJECT_LISTLESS;
    dqf |= DQF_WIDTH(width);
    os_atomic_store2o(dq, dq_atomic_flags, dqf, relaxed);
    dq->dq_state = dq_state;
    dq->dq_serialnum =
            os_atomic_inc_orig(&_dispatch_queue_serial_numbers, relaxed);
    return dqu;
}
```
&emsp;以上即为创建自定义队列的全部内容，整体下来还是比较清晰的。


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
