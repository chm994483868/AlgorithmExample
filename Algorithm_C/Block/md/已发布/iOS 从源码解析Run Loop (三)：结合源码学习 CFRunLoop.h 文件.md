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
    pthread_mutex_t _lock; /* must have the run loop locked before locking this */ 必须在锁定之前将运行循环锁定
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
    
    dispatch_source_t _timerSource;
    dispatch_queue_t _queue;
    Boolean _timerFired; // set to true by the source when a timer has fired
    Boolean _dispatchTimerArmed;
#endif

#if USE_MK_TIMER_TOO
    // iOS 下，使用 MK 表示 timer 
    
    mach_port_t _timerPort;
    Boolean _mkTimerArmed;
#endif

#if DEPLOYMENT_TARGET_WINDOWS
    DWORD _msgQMask;
    void (*_msgPump)(void);
#endif

    uint64_t _timerSoftDeadline; /* TSR */
    uint64_t _timerHardDeadline; /* TSR */
};
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
