# iOS 从源码解析Run Loop (三)：结合源码学习 CFRunLoop.h 文件

> &emsp;本篇通过 Apple 开源的 CF-1151.16 来学习 CFRunLoop.h 文件。⛽️⛽️

&emsp;CFRunLoop.h 文件是 run loop 在 Core Foundation 下的最重要的接口文件，与我们前面学习的 Cocoa 下的 NSRunLoop.h 文件相对应，但是 CFRunLoop.h 文件包含更多的 run loop 的操作，下面我们就一起来学习一下吧！

&emsp;看到 CFRunLoop.h 文件的内容被包裹在 `CF_IMPLICIT_BRIDGING_ENABLED` 和 `CF_IMPLICIT_BRIDGING_DISABLED` 它们是一对表示隐式桥接转换的宏。
```c++
#ifndef CF_IMPLICIT_BRIDGING_ENABLED
#if __has_feature(arc_cf_code_audited)
#define CF_IMPLICIT_BRIDGING_ENABLED _Pragma("clang arc_cf_code_audited begin")
#else
#define CF_IMPLICIT_BRIDGING_ENABLED
#endif
#endif
```
```c++
#ifndef CF_IMPLICIT_BRIDGING_DISABLED
#if __has_feature(arc_cf_code_audited)
#define CF_IMPLICIT_BRIDGING_DISABLED _Pragma("clang arc_cf_code_audited end")
#else
#define CF_IMPLICIT_BRIDGING_DISABLED
#endif
#endif
```
&emsp;表示 Cocoa 和 Core Foundation 下对应的免费桥接转换类型。如 id 和 struct __CFRunLoop，id 和 struct __CFRunLoopSource，NSTimer 和 struct __CFRunLoopTimer。 
```c++
#if __has_attribute(objc_bridge)
    #define CF_BRIDGED_MUTABLE_TYPE(T)    __attribute__((objc_bridge_mutable(T)))
#else
    #define CF_BRIDGED_MUTABLE_TYPE(T)
#endif
```
## Run Loop 数据结构
&emsp;Core Founction 中 run loop 相关的数据结构有：CFRunLoopRef、CFRunLoopSourceRef、CFRunLoopObserverRef、CFRunLoopTimerRef 等等。
### CFRunLoopRef（struct __CFRunLoop *）
&emsp;在 Core Foundation 中 __CFRunLoop 结构体是 Run Loop 对应的数据结构，对应 Cocoa 中的 NSRunLoop 类。CFRunLoopRef 则是指向 __CFRunLoop 结构体的指针。
```c++
typedef struct __CFRunLoop * CFRunLoopRef;

struct __CFRunLoop {
    CFRuntimeBase _base; // 所有 CF "instances" 都是从这个结构开始的
    pthread_mutex_t _lock; /* locked for accessing mode list */ 锁定以访问模式列表
    
    // typedef mach_port_t __CFPort;
    __CFPort _wakeUpPort; // used for CFRunLoopWakeUp 手动唤醒 run loop 的端口。初始化 run loop 时设置，仅用于 CFRunLoopWakeUp，CFRunLoopWakeUp 函数会向 _wakeUpPort 发送一条消息
    
    Boolean _unused; // 标记是否使用过
    
    volatile _per_run_data *_perRunData; // reset for runs of the run loop // run loop 运行会重置的一个数据结构
    pthread_t _pthread; // run loop 所对应的线程
    uint32_t _winthread;
    
    CFMutableSetRef _commonModes; // 存储字符串（而非 runloopMode 对象）的容器，对应着所有标记为 common 的 mode。
    CFMutableSetRef _commonModeItems; // 存储 modeItem 对象的容器，对应于所有标记为 common 的 mode 下的 Item（即 source、timer、observer）
    CFRunLoopModeRef _currentMode; // 当前运行的 Run Loop Mode 实例的指针（typedef struct __CFRunLoopMode *CFRunLoopModeRef）
    CFMutableSetRef _modes; // 集合，存储的是 CFRunLoopModeRef
    
    // 链表头指针，该链表保存了所有需要被 run loop 执行的 block。
    // 外部通过调用 CFRunLoopPerformBlock 函数来向链表中添加一个 block 节点。
    // run loop 会在 CFRunLoopDoBlock 时遍历该链表，逐一执行 block。
    struct _block_item *_blocks_head;
    
    // 链表尾指针，之所以有尾指针，是为了降低增加 block 节点时的时间复杂度。
    //（例如有新节点插入时，只有头节点的话，要从头节点遍历才能找到尾节点，现在已经有尾节点不需要遍历了，则时间复杂度从 O(n) 降到了 O(1）。)
    struct _block_item *_blocks_tail;
    
    // 绝对时间是自参考日期到参考日期（纪元）为 2001 年 1 月 1 日 00:00:00 的时间间隔。
    // typedef double CFTimeInterval;
    // typedef CFTimeInterval CFAbsoluteTime;
    
    CFAbsoluteTime _runTime; // 运行时间点
    CFAbsoluteTime _sleepTime; // 休眠时间点
    
    // 所有 "CF objects" 的基础 "type" 及其上的多态函数（polymorphic functions）
    // typedef const void * CFTypeRef;
    CFTypeRef _counterpart;
};
```
#### CFRuntimeBase
&emsp;所有 CF "instances" 都是从这个结构开始的。不要直接引用这些字段--它们是供 CF 使用的，可以在没有警告的情况下添加或删除或更改格式。不能保证从一个版本到另一个版本使用此结构的二进制兼容性。
```c++
typedef struct __CFRuntimeBase {
    uintptr_t _cfisa;
    uint8_t _cfinfo[4];
#if __LP64__
    uint32_t _rc;
#endif
} CFRuntimeBase;
```
#### _per_run_data
&emsp;重置 run loop 时用的数据结构，每次 run loop 运行后都会重置 _perRunData。
```c++
typedef struct _per_run_data {
    uint32_t a;
    uint32_t b;
    uint32_t stopped; // run loop 是否停止
    uint32_t ignoreWakeUps; // run loop 是否已唤醒
} _per_run_data;
```
#### _block_item
&emsp;需要被 run loop 执行的 block 链表中的节点数据结构。
```c++
struct _block_item {
    struct _block_item *_next; // 指向下一个节点
    
    // typedef const void * CFTypeRef; 
    // CFString 或 CFSet 类型，也就是说一个 block 可能对应单个或多个 mode 
    CFTypeRef _mode;    // CFString or CFSet
    
    void (^_block)(void); // 真正要执行的 block 本体
};
```
### CFRunLoopModeRef（struct __CFRunLoopMode *）
&emsp;每次 run loop开始 run 的时候，都必须指定一个 mode，称为 run loop mode。mode 指定了在这次的 run 中，run loop 可以处理的任务。对于不属于当前 mode 的任务，则需要切换 run loop 至对应 mode 下，再重新调用 run 方法，才能够被处理
```c++
typedef struct __CFRunLoopMode *CFRunLoopModeRef;

struct __CFRunLoopMode {
    CFRuntimeBase _base; // 所有 CF "instances" 都是从这个结构开始的
    pthread_mutex_t _lock; /* must have the run loop locked before locking this */ 必须在锁定之前将 run loop 锁定，即加锁前需要 run loop 先加锁
    CFStringRef _name; // mode 都会指定一个字符串名称
    Boolean _stopped; // 标记了 run loop 的运行状态，实际上并非如此简单，还有前面的 _per_run_data。
    char _padding[3]; // 
    
    // _sources0、_sources1、_observers、_timers 都是集合类型，里面都是 mode item，即一个 mode 包含多个 mode item
    CFMutableSetRef _sources0; // sources0 事件集合（之所以用集合是为了保证每个元素唯一）
    CFMutableSetRef _sources1; // sources1 事件集合
    CFMutableArrayRef _observers; // run loop observer 观察者数组
    CFMutableArrayRef _timers; // 定时器数组
    
    CFMutableDictionaryRef _portToV1SourceMap; // 存储了 Source1 的 port 与 source 的对应关系，key 是 mach_port_t，value 是 CFRunLoopSourceRef
    __CFPortSet _portSet; // 保存所有需要监听的 port，比如 _wakeUpPort，_timerPort 都保存在这个集合中
    CFIndex _observerMask; // 添加 obsever 时设置 _observerMask 为 observer 的 _activities（CFRunLoopActivity 状态）
    
    // DEPLOYMENT_TARGET_MACOSX 表示是否部署在 maxOS 下
    // #if DEPLOYMENT_TARGET_MACOSX
    //  #define USE_DISPATCH_SOURCE_FOR_TIMERS 1
    //  #define USE_MK_TIMER_TOO 1
    // #else
    //  #define USE_DISPATCH_SOURCE_FOR_TIMERS 0
    //  #define USE_MK_TIMER_TOO 1
    // #endif
    
#if USE_DISPATCH_SOURCE_FOR_TIMERS
    // macOS 下，使用 dispatch_source 表示 timer
    dispatch_source_t _timerSource; // GCD 定时器
    dispatch_queue_t _queue; // 队列
    Boolean _timerFired; // set to true by the source when a timer has fired 计时器触发时由 source 设置为 true
    Boolean _dispatchTimerArmed;
#endif

#if USE_MK_TIMER_TOO
    // iOS 下，使用 MK 表示 timer 
    mach_port_t _timerPort; // MK_TIMER 的 port
    Boolean _mkTimerArmed;
#endif

#if DEPLOYMENT_TARGET_WINDOWS
    DWORD _msgQMask;
    void (*_msgPump)(void);
#endif

    // timer 软临界点
    uint64_t _timerSoftDeadline; /* TSR */
    // timer 硬临界点
    uint64_t _timerHardDeadline; /* TSR */
};
```
### CFRunLoopSourceRef（struct __CFRunLoopSource *）
&emsp;CFRunLoopSourceRef 是事件源（输入源），通过源码可以发现，其分为 source0 和 source1 两个。
```c++
typedef struct __CFRunLoopSource * CFRunLoopSourceRef;

struct __CFRunLoopSource {
    CFRuntimeBase _base; // 所有 CF "instances" 都是从这个结构开始的
    uint32_t _bits;
    pthread_mutex_t _lock; // 互斥锁
    CFIndex _order; /* immutable */ source 的优先级，值为小，优先级越高
    CFMutableBagRef _runLoops; // run loop 集合
    union {
        CFRunLoopSourceContext version0; /* immutable, except invalidation */
        CFRunLoopSourceContext1 version1; /* immutable, except invalidation */
    } _context;
};
```
&emsp;CFRunLoopSourceContext 
```c++
typedef struct {
    CFIndex    version;
    void *    info; // source 的信息
    
    const void *(*retain)(const void *info); // retain 函数
    void    (*release)(const void *info); // release 函数
    
    CFStringRef    (*copyDescription)(const void *info);
    
    Boolean    (*equal)(const void *info1, const void *info2); // 判断 source 相等的函数
    
    CFHashCode    (*hash)(const void *info);
    void    (*schedule)(void *info, CFRunLoopRef rl, CFStringRef mode);
    void    (*cancel)(void *info, CFRunLoopRef rl, CFStringRef mode);
    
    void    (*perform)(void *info); // source 要执行的任务块
} CFRunLoopSourceContext;
```
&emsp;CFRunLoopSourceContext1
```c++
typedef struct {
    CFIndex    version;
    void *    info; // source 的信息
    
    const void *(*retain)(const void *info); // retain 函数
    void    (*release)(const void *info); // release 函数
    
    CFStringRef    (*copyDescription)(const void *info);
    
    Boolean    (*equal)(const void *info1, const void *info2); // 判断 source 相等的函数
    
    // #if __LLP64__
    //  typedef unsigned long long CFOptionFlags;
    //  typedef unsigned long long CFHashCode;
    //  typedef signed long long CFIndex;
    // #else
    //  typedef unsigned long CFOptionFlags;
    //  typedef unsigned long CFHashCode;
    //  typedef signed long CFIndex;
    // #endif
    
    CFHashCode    (*hash)(const void *info);
#if (TARGET_OS_MAC && !(TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) || (TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)
    mach_port_t    (*getPort)(void *info);
    void *    (*perform)(void *msg, CFIndex size, CFAllocatorRef allocator, void *info);
#else
    void *    (*getPort)(void *info);
    void    (*perform)(void *info);
#endif
} CFRunLoopSourceContext1;
```
#### CFAllocatorRef
&emsp;在大多数情况下，为创建函数指定分配器时，NULL 参数表示 “使用默认值”；这与使用 kCFAllocatorDefault 或 `CFAllocatorGetDefault()` 的返回值相同。这样可以确保你将使用当时有效的分配器。
```c++
typedef const struct __CFAllocator * CFAllocatorRef;

struct __CFAllocator {
    CFRuntimeBase _base;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
    // CFAllocator structure must match struct _malloc_zone_t!
    // The first two reserved fields in struct _malloc_zone_t are for us with CFRuntimeBase
    size_t     (*size)(struct _malloc_zone_t *zone, const void *ptr); /* returns the size of a block or 0 if not in this zone; must be fast, especially for negative answers */
    void     *(*malloc)(struct _malloc_zone_t *zone, size_t size);
    void     *(*calloc)(struct _malloc_zone_t *zone, size_t num_items, size_t size); /* same as malloc, but block returned is set to zero */
    void     *(*valloc)(struct _malloc_zone_t *zone, size_t size); /* same as malloc, but block returned is set to zero and is guaranteed to be page aligned */
    void     (*free)(struct _malloc_zone_t *zone, void *ptr);
    void     *(*realloc)(struct _malloc_zone_t *zone, void *ptr, size_t size);
    void     (*destroy)(struct _malloc_zone_t *zone); /* zone is destroyed and all memory reclaimed */
    const char    *zone_name;

    /* Optional batch callbacks; these may be NULL */
    unsigned    (*batch_malloc)(struct _malloc_zone_t *zone, size_t size, void **results, unsigned num_requested); /* given a size, returns pointers capable of holding that size; returns the number of pointers allocated (maybe 0 or less than num_requested) */
    void    (*batch_free)(struct _malloc_zone_t *zone, void **to_be_freed, unsigned num_to_be_freed); /* frees all the pointers in to_be_freed; note that to_be_freed may be overwritten during the process */

    struct malloc_introspection_t    *introspect;
    unsigned    version;
    
    /* aligned memory allocation. The callback may be NULL. */
    void *(*memalign)(struct _malloc_zone_t *zone, size_t alignment, size_t size);
    
    /* free a pointer known to be in zone and known to have the given size. The callback may be NULL. */
    void (*free_definite_size)(struct _malloc_zone_t *zone, void *ptr, size_t size);
#endif
    CFAllocatorRef _allocator;
    CFAllocatorContext _context;
};
```
### CFRunLoopObserverRef（struct __CFRunLoopObserver *）
&emsp;CFRunLoopObserverRef 是观察者，每个 Observer 都包含了一个回调(函数指针)，当 RunLoop 的状态发生变化时，观察者就能通过回调接受到这个变化。主要是用来向外界报告 Runloop 当前的状态的更改。
```c++
typedef struct __CFRunLoopObserver * CFRunLoopObserverRef;

struct __CFRunLoopObserver {
    CFRuntimeBase _base; // 所有 CF "instances" 都是从这个结构开始的
    pthread_mutex_t _lock; // 互斥锁
    CFRunLoopRef _runLoop; // observer 所观察的 run loop
    CFIndex _rlCount; // observer 观察了多少个 run loop
    CFOptionFlags _activities; /* immutable */ // 所监听的事件，通过位异或，可以监听多种事件，_activities 用来说明要观察 runloop 的哪些状态，一旦指定了就不可变。
    CFIndex _order; /* immutable */ // observer 优先级
    
    // typedef void (*CFRunLoopObserverCallBack)(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *info);
    CFRunLoopObserverCallBack _callout; /* immutable */ // observer 回调函数，观察到 run loop 状态变化后的回调
    
    CFRunLoopObserverContext _context; /* immutable, except invalidation */ // observer 上下文
};
```
#### CFRunLoopActivity
&emsp;运行循环观察者活动。
```c++
/* Run Loop Observer Activities */
typedef CF_OPTIONS(CFOptionFlags, CFRunLoopActivity) {
    kCFRunLoopEntry = (1UL << 0), // 进入 RunLoop 循环(这里其实还没进入)
    kCFRunLoopBeforeTimers = (1UL << 1), // Run Loop 要处理 timer 了
    kCFRunLoopBeforeSources = (1UL << 2), // Run Loop 要处理 source 了
    kCFRunLoopBeforeWaiting = (1UL << 5), // Run Loop 要休眠了
    kCFRunLoopAfterWaiting = (1UL << 6), // Run Loop 醒了
    kCFRunLoopExit = (1UL << 7), // Run Loop 退出（和 kCFRunLoopEntry 对应，Entry 和 Exit 在每次 Run Loop 循环中仅调用一次，用于表示即将进入循环和退出循环。）
    kCFRunLoopAllActivities = 0x0FFFFFFFU
};
```
#### CFRunLoopObserverContext
```c++
typedef struct {
    CFIndex    version;
    void *    info;
    const void *(*retain)(const void *info);
    void    (*release)(const void *info);
    CFStringRef    (*copyDescription)(const void *info);
} CFRunLoopObserverContext;
```







## 参考链接
**参考链接:🔗**
+ [runloop 源码](https://opensource.apple.com/tarballs/CF/)
+ [Run Loops 官方文档](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Multithreading/RunLoopManagement/RunLoopManagement.html#//apple_ref/doc/uid/10000057i-CH16-SW1)
+ [iOS RunLoop完全指南](https://blog.csdn.net/u013378438/article/details/80239686)
+ [iOS源码解析: runloop的底层数据结构](https://juejin.cn/post/6844904090330234894)
+ [iOS源码解析: runloop的运行原理](https://juejin.cn/post/6844904090166624270)
+ [深入理解RunLoop](https://blog.ibireme.com/2015/05/18/runloop/)
+ [iOS底层学习 - 深入RunLoop](https://juejin.cn/post/6844903973665636360)
+ [一份走心的runloop源码分析](https://cloud.tencent.com/developer/article/1633329)
+ [NSRunLoop](https://www.cnblogs.com/wsnb/p/4753685.html)
+ [iOS刨根问底-深入理解RunLoop](https://www.cnblogs.com/kenshincui/p/6823841.html)
+ [RunLoop总结与面试](https://www.jianshu.com/p/3ccde737d3f3)
