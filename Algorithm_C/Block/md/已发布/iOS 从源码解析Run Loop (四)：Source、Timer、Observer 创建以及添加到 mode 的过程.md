# iOS 从源码解析Run Loop (四)：Source、Timer、Observer 创建以及添加到 mode 的过程

> &emsp;首先接上上篇由于文字个数限制没有放下的内容。

## \__CFRunLoopFindMode
&emsp;`__CFRunLoopFindMode` 函数根据 modeName 从 rl 的 _modes 中找到其对应的 CFRunLoopModeRef，如果找到的话则加锁并返回。如果未找到，并且 create 为真的话，则新建 __CFRunLoopMode 加锁并返回，如果 create 为假的话，则返回 NULL。
```c++
static CFRunLoopModeRef __CFRunLoopFindMode(CFRunLoopRef rl, CFStringRef modeName, Boolean create) {
    // 用于检查给定的进程是否被 fork
    CHECK_FOR_FORK();
    
    // struct __CFRunLoopMode 结构体指针
    CFRunLoopModeRef rlm;
    
    // 创建一个 struct __CFRunLoopMode 结构体实例，
    // 并调用 memset 把 srlm 内存空间全部置为 0。
    struct __CFRunLoopMode srlm;
    memset(&srlm, 0, sizeof(srlm));
    
    // __kCFRunLoopModeTypeID 现在正是表示 CFRunLoopMode 类，实际值是 run loop mode 类在全局类表 __CFRuntimeClassTable 中的索引。
    
    // 前面 __CFRunLoopCreate 函数内部会调用 CFRunLoopGetTypeID() 函数，
    // 其内部是全局执行一次在 CF 运行时中注册两个新类 run loop（CFRunLoop）和 run loop mode（CFRunLoopMode），
    // 其中 __kCFRunLoopModeTypeID = _CFRuntimeRegisterClass(&__CFRunLoopModeClass)，那么 __kCFRunLoopModeTypeID 此时便是 run loop mode 类在全局类表中的索引。
    //（__CFRunLoopModeClass 可以理解为一个静态全局的 "类对象"（实际值是一个），_CFRuntimeRegisterClass 函数正是把它放进一个全局的 __CFRuntimeClassTable 类表中。）

    // 本身 srlm 是一片空白内存，现在相当于把 srlm 设置为一个 run loop mode 类的对象。 
    //（实际就是设置 CFRuntimeBase 的 _cfinfo 成员变量，srlm 里面目前包含的内容就是 run loop mode 的类信息。）
    _CFRuntimeSetInstanceTypeIDAndIsa(&srlm, __kCFRunLoopModeTypeID);
    
    // 把 srlm 的 mode 名称设置为入参 modeName
    srlm._name = modeName;
    
    // 从 rl->_modes 哈希表中找 &srlm 对应的 CFRunLoopModeRef
    rlm = (CFRunLoopModeRef)CFSetGetValue(rl->_modes, &srlm);
    
    // 如果找到了则加锁，然后返回 rlm。
    if (NULL != rlm) {
        __CFRunLoopModeLock(rlm);
        return rlm;
    }
    
    // 如果没有找到，并且 create 值为 false，则表示不进行创建，直接返回 NULL。
    if (!create) {
    return NULL;
    }
    
    // 创建一个 CFRunLoopMode 对并返回其地址
    rlm = (CFRunLoopModeRef)_CFRuntimeCreateInstance(kCFAllocatorSystemDefault, __kCFRunLoopModeTypeID, sizeof(struct __CFRunLoopMode) - sizeof(CFRuntimeBase), NULL);
    
    // 如果 rlm 创建失败，则返回 NULL
    if (NULL == rlm) {
        return NULL;
    }
    
    // 初始化 rlm 的 pthread_mutex_t _lock 为一个互斥递归锁。
    //（__CFRunLoopLockInit 内部使用的 PTHREAD_MUTEX_RECURSIVE 表示递归锁，允许同一个线程对同一锁加锁多次，且需要对应次数的解锁操作）
    __CFRunLoopLockInit(&rlm->_lock);
    
    // 初始化 _name
    rlm->_name = CFStringCreateCopy(kCFAllocatorSystemDefault, modeName);
    
    // 下面是一组成员变量的初始赋值
    rlm->_stopped = false;
    rlm->_portToV1SourceMap = NULL;
    
    // _sources0、_sources1、_observers、_timers 初始状态都是空的
    rlm->_sources0 = NULL;
    rlm->_sources1 = NULL;
    rlm->_observers = NULL;
    rlm->_timers = NULL;
    
    rlm->_observerMask = 0;
    rlm->_portSet = __CFPortSetAllocate(); // CFSet 申请空间初始化
    rlm->_timerSoftDeadline = UINT64_MAX;
    rlm->_timerHardDeadline = UINT64_MAX;
    
    // ret 是一个临时变量初始值是 KERN_SUCCESS，用来表示向 rlm->_portSet 中添加 port 时的结果，
    // 如果添加失败的话，会直接 CRASH， 
    kern_return_t ret = KERN_SUCCESS;
    
#if USE_DISPATCH_SOURCE_FOR_TIMERS
    // macOS 下，使用 dispatch_source 构造 timer
    
    // _timerFired 首先赋值为 false，然后在 timer 的回调函数执行的时候会赋值为 true
    rlm->_timerFired = false;
    
    // 队列
    rlm->_queue = _dispatch_runloop_root_queue_create_4CF("Run Loop Mode Queue", 0);
    
    // 构建 queuePort，入参是 mode 的 _queue
    mach_port_t queuePort = _dispatch_runloop_root_queue_get_port_4CF(rlm->_queue);
    
    // 如果 queuePort 为 NULL，则 crash。（无法创建运行循环模式队列端口。）
    if (queuePort == MACH_PORT_NULL) CRASH("*** Unable to create run loop mode queue port. (%d) ***", -1);
    
    // 构建 dispatch_source 类型使用的是 DISPATCH_SOURCE_TYPE_TIMER，表示是一个 timer
    rlm->_timerSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, rlm->_queue);
    
    // 这里为了在下面的 block 内部修改 _timerFired 的值，用了一个 __block 指针变量。（觉的如果这里只是改值，感觉用指针就够了可以不用 __block 修饰）
    // 当 _timerSource（计时器）回调时会执行这个 block。
    __block Boolean *timerFiredPointer = &(rlm->_timerFired);
    dispatch_source_set_event_handler(rlm->_timerSource, ^{
        *timerFiredPointer = true;
    });
    
    // Set timer to far out there. The unique leeway makes this timer easy to spot in debug output.
    // 将计时器设置在远处。独特的回旋余地使该计时器易于发现调试输出。（从 DISPATCH_TIME_FOREVER 启动，DISPATCH_TIME_FOREVER 为时间间隔）
    _dispatch_source_set_runloop_timer_4CF(rlm->_timerSource, DISPATCH_TIME_FOREVER, DISPATCH_TIME_FOREVER, 321);
    // 启动
    dispatch_resume(rlm->_timerSource);
    
    // 把运行循环模式队列端口 queuePort 添加到 rlm 的 _portSet（端口集合）中。
    ret = __CFPortSetInsert(queuePort, rlm->_portSet);
    // 如果添加失败则 crash。（无法将计时器端口插入端口集中。）
    if (KERN_SUCCESS != ret) CRASH("*** Unable to insert timer port into port set. (%d) ***", ret);
#endif

#if USE_MK_TIMER_TOO
    // mk 构造 timer
    
    // 构建 timer 端口
    rlm->_timerPort = mk_timer_create();
    // 同样把 rlm 的 _timerPort 添加到 rlm 的 _portSet（端口集合）中。
    ret = __CFPortSetInsert(rlm->_timerPort, rlm->_portSet);
    // 如果添加失败则 crash。（无法将计时器端口插入端口集中。）
    if (KERN_SUCCESS != ret) CRASH("*** Unable to insert timer port into port set. (%d) ***", ret);
#endif
    
    // 然后这里把 rl 的 _wakeUpPort 也添加到 rlm 的 _portSet（端口集合）中。
    //（这里要特别注意一下，run loop 的 _wakeUpPort 会被插入到所有 mode 的 _portSet 中。）
    ret = __CFPortSetInsert(rl->_wakeUpPort, rlm->_portSet);
    // 如果添加失败则 crash。（无法将唤醒端口插入端口集中。）
    if (KERN_SUCCESS != ret) CRASH("*** Unable to insert wake up port into port set. (%d) ***", ret);
    
#if DEPLOYMENT_TARGET_WINDOWS
    rlm->_msgQMask = 0;
    rlm->_msgPump = NULL;
#endif

    // 这里把 rlm 添加到 rl 的 _modes 中，
    //（本质是把 rlm 添加到 _modes 哈希表中）
    CFSetAddValue(rl->_modes, rlm);
    
    // 释放，rlm 被 rl->_modes 持有，并不会被销毁
    CFRelease(rlm);
    
    // 加锁，然后返回 rlm
    __CFRunLoopModeLock(rlm);    /* return mode locked */
    return rlm;
}
```
&emsp;其中 `ret = __CFPortSetInsert(rl->_wakeUpPort, rlm->_portSet)` 会把 run loop 对象的 `_wakeUpPort` 添加到每个 run loop mode 对象的 `_portSet` 端口集合里。即当一个 run loop 有多个 run loop mode 时，那么每个 run loop mode 都会有 run loop 的 `_wakeUpPort`。

&emsp;在 macOS 下 run loop mode 的 `_timerSource` 的计时器的回调事件内部会把 run loop mode 的 `_timerFired` 字段置为 true，表示计时器被触发。

&emsp;run loop mode 创建好了，看到 source/timer/observer 三者对应的 `_sources0`、`_sources1`、`_observers`、`_timers` 四个字段初始状态都是空，需要我们自己添加 run loop mode item，它们在代码层中对应的数据类型分别是: CFRunLoopSourceRef、CFRunLoopObserverRef、CFRunLoopTimerRef，那么就把它们放在下篇进行分析吧！

> &emsp;下面开始进入本篇内容。

## CFRunLoopSourceRef（struct \__CFRunLoopSource *）
&emsp;CFRunLoopSourceRef 是事件源（输入源），通过源码可以发现它内部的 `_context` 联合体中有两个成员变量 `version0` 和 `version1`，它们正分别对应了我们前面提到过多次的 source0 和 source1。
```c++
typedef struct __CFRunLoopSource * CFRunLoopSourceRef;

struct __CFRunLoopSource {
    CFRuntimeBase _base; // 所有 CF "instances" 都是从这个结构开始的
    uint32_t _bits;
    pthread_mutex_t _lock; // 互斥递归锁
    CFIndex _order; /* immutable */ source 的优先级，值为小，优先级越高
    CFMutableBagRef _runLoops; // run loop 集合
    union {
        CFRunLoopSourceContext version0; /* immutable, except invalidation */
        CFRunLoopSourceContext1 version1; /* immutable, except invalidation */
    } _context;
};
```
&emsp;当 \__CFRunLoopSource 表示 source0 的数据结构时 `_context` 中使用 `CFRunLoopSourceContext version0`，下面是 CFRunLoopSourceContext 的定义。 
```c++
// #if __LLP64__
//  typedef unsigned long long CFOptionFlags;
//  typedef unsigned long long CFHashCode;
//  typedef signed long long CFIndex;
// #else
//  typedef unsigned long CFOptionFlags;
//  typedef unsigned long CFHashCode;
//  typedef signed long CFIndex;
// #endif

typedef struct {
    CFIndex version;
    void * info; // source 的信息
    const void *(*retain)(const void *info); // retain 函数
    void (*release)(const void *info); // release 函数
    CFStringRef (*copyDescription)(const void *info); // 返回描述字符串的函数
    Boolean (*equal)(const void *info1, const void *info2); // 判断 source 对象是否相等的函数
    CFHashCode (*hash)(const void *info); // 哈希函数
    
    // 上面是 CFRunLoopSourceContext 和 CFRunLoopSourceContext1 的基础内容双方完成等同，
    // 两者的区别主要在下面，同时它们也表示了 source0 和 source1 的不同功能。
    
    void (*schedule)(void *info, CFRunLoopRef rl, CFStringRef mode); // 当 source 加入到 run loop 时触发的回调函数
    void (*cancel)(void *info, CFRunLoopRef rl, CFStringRef mode); // 当 source 从 run loop 中移除时触发的回调函数
    void (*perform)(void *info); // source 要执行的任务块，当 source 事件被触发时的回调, 使用 CFRunLoopSourceSignal 函数触发
} CFRunLoopSourceContext;
```
&emsp;当 \__CFRunLoopSource 表示 source1 的数据结构时 `_context` 中使用 `CFRunLoopSourceContext1 version1`，下面是 CFRunLoopSourceContext1 的定义。
```c++
typedef struct {
    CFIndex version;
    void * info; // source 的信息
    const void *(*retain)(const void *info); // retain 函数
    void (*release)(const void *info); // release 函数
    CFStringRef (*copyDescription)(const void *info); // 返回描述字符串的函数
    Boolean (*equal)(const void *info1, const void *info2); // 判断 source 对象是否相等的函数
    CFHashCode (*hash)(const void *info); // 哈希函数
    
    // 上面是 CFRunLoopSourceContext 和 CFRunLoopSourceContext1 的基础内容双方完成等同，
    // 两者的区别主要在下面，同时它们也表示了 source0 和 source1 的不同功能。
    
#if (TARGET_OS_MAC && !(TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)) || (TARGET_OS_EMBEDDED || TARGET_OS_IPHONE)
    mach_port_t (*getPort)(void *info); // getPort 函数指针，用于当 source 被添加到 run loop 中的时候，从该函数中获取具体的 mach_port_t 对象.
    void * (*perform)(void *msg, CFIndex size, CFAllocatorRef allocator, void *info); // perform 函数指针即指向 run loop 被唤醒后将要处理的事情
#else
    void * (*getPort)(void *info);
    void (*perform)(void *info);
#endif
} CFRunLoopSourceContext1;
```
&emsp;上面是 `__CFRunLoopSource` 相关的数据结构，下面我们看一下 source 的创建函数。
### CFRunLoopSourceGetTypeID
&emsp;`__CFRunLoopSourceClass` 
```c++
static const CFRuntimeClass __CFRunLoopSourceClass = {
    _kCFRuntimeScannedObject,
    "CFRunLoopSource",
    NULL, // init
    NULL, // copy
    __CFRunLoopSourceDeallocate, // 销毁函数
    __CFRunLoopSourceEqual, // 判等函数
    __CFRunLoopSourceHash, // 哈希函数
    NULL,
    __CFRunLoopSourceCopyDescription // 描述函数
};
```
&emsp;`CFRunLoopSourceGetTypeID` 函数把 \__CFRunLoopSourceClass 注册到类表中，并返回其在类表中的索引。
```c++
CFTypeID CFRunLoopSourceGetTypeID(void) {
    static dispatch_once_t initOnce;
    dispatch_once(&initOnce, ^{ 
        __kCFRunLoopSourceTypeID = _CFRuntimeRegisterClass(&__CFRunLoopSourceClass); 
    });
    return __kCFRunLoopSourceTypeID;
}
```
### CFRunLoopSourceCreate
&mesp;`CFRunLoopSourceCreate` 是根据入参 `context` 来创建 source0 或 source1。
```c++
CFRunLoopSourceRef CFRunLoopSourceCreate(CFAllocatorRef allocator, CFIndex order, CFRunLoopSourceContext *context) {
    // 用于检查给定的进程是否被 fork
    CHECK_FOR_FORK();
    
    // 局部变量
    CFRunLoopSourceRef memory;
    uint32_t size;
    
    // context 不能为 NULL，否则直接 crash
    if (NULL == context) CRASH("*** NULL context value passed to CFRunLoopSourceCreate(). (%d) ***", -1);
    
    // 计算 __CFRunLoopSource 结构除 CFRuntimeBase base 字段之外的内存空间长度
    size = sizeof(struct __CFRunLoopSource) - sizeof(CFRuntimeBase);
    
    // 创建 __CFRunLoopSource 实例并返回其指针
    memory = (CFRunLoopSourceRef)_CFRuntimeCreateInstance(allocator, CFRunLoopSourceGetTypeID(), size, NULL);
    if (NULL == memory) {
        return NULL;
    }
    
    // 设置 memory 的 _cfinfo 字段的值
    __CFSetValid(memory);
    
    // 设置 _bits 字段的值
    __CFRunLoopSourceUnsetSignaled(memory);
    
    // 初始化 _lock 为互斥递归锁
    __CFRunLoopLockInit(&memory->_lock);
    
    memory->_bits = 0;
    memory->_order = order; // order 赋值
    memory->_runLoops = NULL;
    
    size = 0;
    
    // 根据 context 判断是 source0 还是 source1，它们有不同的内存长度
    switch (context->version) {
        case 0:
            size = sizeof(CFRunLoopSourceContext);
        break;
        case 1:
            size = sizeof(CFRunLoopSourceContext1);
        break;
    }
    
    // 设置 memory 的 context 区域的内存值为 context
    objc_memmove_collectable(&memory->_context, context, size);
    
    // 如果 context 的 retain 函数不为 NULL，则对其 info 执行 retain 函数
    if (context->retain) {
        memory->_context.version0.info = (void *)context->retain(context->info);
    }
    
    return memory;
}
```
&emsp;基本就是申请空间，然后进行一些字段进行初始化。下面看一下 source 是如何添加到 mode 中的。
### CFRunLoopAddSource
&emsp;`CFRunLoopAddSource` 函数是把 source 添加到 mode 中，不出意外，mode 参数是一个字符串。
```c++
void CFRunLoopAddSource(CFRunLoopRef rl, CFRunLoopSourceRef rls, CFStringRef modeName) {    /* DOES CALLOUT */
    // 用于检查给定的进程是否被 fork
    CHECK_FOR_FORK();
    
    // 如果 rl 被标记为正在进行释放，则直接返回。
    if (__CFRunLoopIsDeallocating(rl)) return;
    
    // 如果 rls 无效，则 return。
    if (!__CFIsValid(rls)) return;
    
    // 
    Boolean doVer0Callout = false;
    
    // 加锁
    __CFRunLoopLock(rl);
    
    // 如果 modeName 是 kCFRunLoopCommonModes，即把 source 添加到 common mode 中
    if (modeName == kCFRunLoopCommonModes) {
        // 如果 rl 的 _commonModes 不为 NULL，则创建一个副本。
        CFSetRef set = rl->_commonModes ? CFSetCreateCopy(kCFAllocatorSystemDefault, rl->_commonModes) : NULL;
        
        if (NULL == rl->_commonModeItems) {
            rl->_commonModeItems = CFSetCreateMutable(kCFAllocatorSystemDefault, 0, &kCFTypeSetCallBacks);
        }
        
        CFSetAddValue(rl->_commonModeItems, rls);
        if (NULL != set) {
            CFTypeRef context[2] = {rl, rls};
            /* add new item to all common-modes */
            CFSetApplyFunction(set, (__CFRunLoopAddItemToCommonModes), (void *)context);
            CFRelease(set);
        }
    } else {
        CFRunLoopModeRef rlm = __CFRunLoopFindMode(rl, modeName, true);
        if (NULL != rlm && NULL == rlm->_sources0) {
            rlm->_sources0 = CFSetCreateMutable(kCFAllocatorSystemDefault, 0, &kCFTypeSetCallBacks);
            rlm->_sources1 = CFSetCreateMutable(kCFAllocatorSystemDefault, 0, &kCFTypeSetCallBacks);
            rlm->_portToV1SourceMap = CFDictionaryCreateMutable(kCFAllocatorSystemDefault, 0, NULL, NULL);
        }
        
    if (NULL != rlm && !CFSetContainsValue(rlm->_sources0, rls) && !CFSetContainsValue(rlm->_sources1, rls)) {
        if (0 == rls->_context.version0.version) {
            CFSetAddValue(rlm->_sources0, rls);
        } else if (1 == rls->_context.version0.version) {
            CFSetAddValue(rlm->_sources1, rls);
        __CFPort src_port = rls->_context.version1.getPort(rls->_context.version1.info);
        if (CFPORT_NULL != src_port) {
            CFDictionarySetValue(rlm->_portToV1SourceMap, (const void *)(uintptr_t)src_port, rls);
            __CFPortSetInsert(src_port, rlm->_portSet);
            }
        }
        __CFRunLoopSourceLock(rls);
        if (NULL == rls->_runLoops) {
            rls->_runLoops = CFBagCreateMutable(kCFAllocatorSystemDefault, 0, &kCFTypeBagCallBacks); // sources retain run loops!
        }
        CFBagAddValue(rls->_runLoops, rl);
        __CFRunLoopSourceUnlock(rls);
        if (0 == rls->_context.version0.version) {
            if (NULL != rls->_context.version0.schedule) {
                doVer0Callout = true;
            }
        }
    }
        if (NULL != rlm) {
        __CFRunLoopModeUnlock(rlm);
    }
    }
    __CFRunLoopUnlock(rl);
    if (doVer0Callout) {
        // although it looses some protection for the source, we have no choice but
        // to do this after unlocking the run loop and mode locks, to avoid deadlocks
        // where the source wants to take a lock which is already held in another
        // thread which is itself waiting for a run loop/mode lock
    rls->_context.version0.schedule(rls->_context.version0.info, rl, modeName);    /* CALLOUT */
    }
}
```


## CFRunLoopObserverRef（struct \__CFRunLoopObserver *）
&emsp;CFRunLoopObserverRef 是观察者，每个 observer 都包含了一个回调（函数指针），当 run loop 的状态发生变化时，观察者就能通过回调接受到这个变化。主要是用来向外界报告 run loop 当前的状态的更改。
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
&emsp;observer 也包含一个回调函数，在监听的 run loop 状态出现时触发该回调函数。run loop 对 observer 的使用逻辑，基本与 timer 一致，都需要指定 callback 函数，然后通过 context 可传递参数。

&emsp;`CFRunLoopActivity` 是一组枚举值用于表示 run loop 的活动。
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
&emsp;CFRunLoopObserverContext 的定义。
```c++
typedef struct {
    CFIndex version;
    void * info;
    const void *(*retain)(const void *info);
    void (*release)(const void *info);
    CFStringRef (*copyDescription)(const void *info);
} CFRunLoopObserverContext;
```
## CFRunLoopTimerRef（struct \__CFRunLoopTimer *）
&emsp;NSTimer 是与 run loop 息息相关的，CFRunLoopTimerRef 与 NSTimer 是可以 toll-free bridged（免费桥接转换）的。当 timer 加到 run loop 的时候，run loop 会注册对应的触发时间点，时间到了，run loop 若处于休眠则会被唤醒，执行 timer 对应的回调函数。
```c++
typedef struct CF_BRIDGED_MUTABLE_TYPE(NSTimer) __CFRunLoopTimer * CFRunLoopTimerRef;

struct __CFRunLoopTimer {
    CFRuntimeBase _base; // 所有 CF "instances" 都是从这个结构开始的
    uint16_t _bits; // 标记 timer 的状态
    pthread_mutex_t _lock; // 互斥锁
    CFRunLoopRef _runLoop; // timer 对应的 run loop，注册在哪个 run loop 中
    CFMutableSetRef _rlModes; // timer 对应的 run loop modes，内部保存的也是 run loop mode 的名字，也验证了 timer 可以在多个 run loop mode 中使用
    CFAbsoluteTime _nextFireDate; // timer 的下次触发时机，每次触发后都会再次设置该值
    CFTimeInterval _interval; /* immutable */
    CFTimeInterval _tolerance; /* mutable */ // timer 的允许时间偏差
    uint64_t _fireTSR; /* TSR units */ // timer 本次需要被触发的时间
    CFIndex _order; /* immutable */
    
    // typedef void (*CFRunLoopTimerCallBack)(CFRunLoopTimerRef timer, void *info);
    CFRunLoopTimerCallBack _callout; /* immutable */ // timer 回调
    
    CFRunLoopTimerContext _context; /* immutable, except invalidation */ // timer 上下文，可用于传递参数到 timer 对象的回调函数中。
};
```
&emsp;CFRunLoopTimerContext 的定义。
```c++
typedef struct {
    CFIndex version;
    void * info;
    const void *(*retain)(const void *info);
    void (*release)(const void *info);
    CFStringRef (*copyDescription)(const void *info);
} CFRunLoopTimerContext;
```
&emsp;本篇主要内容聚焦在 run loop 获取（查找+创建）和 run loop mode 的获取（查找+创建），并大致看下 run loop mode item: CFRunLoopSourceRef、CFRunLoopObserverRef、CFRunLoopTimerRef 的结构定义，下篇我们再详细看它们的操作。




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
+ [Runloop-实际开发你想用的应用场景](https://juejin.cn/post/6889769418541252615)
+ [RunLoop 源码阅读](https://juejin.cn/post/6844903592369848328#heading-17)
+ [do {...} while (0) 在宏定义中的作用](https://www.cnblogs.com/lanxuezaipiao/p/3535626.html)
+ [CFRunLoop 源码学习笔记(CF-1151.16)](https://www.cnblogs.com/chengsh/p/8629605.html)
+ [操作系统大端模式和小端模式](https://www.cnblogs.com/wuyuankun/p/3930829.html)
