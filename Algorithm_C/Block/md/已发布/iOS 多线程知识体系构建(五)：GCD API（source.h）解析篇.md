# iOS 多线程知识体系构建(五)：GCD API（source.h）解析篇

> &emsp;那么继续学习 dispath 中也挺重要的 <dispatch/source.h> 文件。

## <dispatch/source.h>
&emsp;dispatch framework 提供了一套接口，用于监视低级系统对象（file descriptors（文件描述符）, Mach ports, signals, VFS nodes, etc.）的活动，并在此类活动发生时自动向 dispatch queues 提交事件处理程序块（event handler blocks）。这套接口称为 Dispatch Source API。
### dispatch_source_t
&emsp;`dispatch_source_t` 表示 dispatch sources 类型，调度源（dispatch sources）用于自动提交事件处理程序块（event handler blocks）到调度队列（dispatch queues）以响应外部事件。
```c++
DISPATCH_SOURCE_DECL(dispatch_source);
```
+ 在 Swift（在 Swift 中使用 Objective-C）下宏定义展开是:
```c++
OS_EXPORT OS_OBJECT_OBJC_RUNTIME_VISIBLE
@interface OS_dispatch_source : OS_dispatch_object
- (instancetype)init OS_SWIFT_UNAVAILABLE("Unavailable in Swift");
@end

typedef OS_dispatch_source * dispatch_source_t

@protocol OS_dispatch_source <NSObject>
@end

@interface OS_dispatch_source () <OS_dispatch_source>
@end
```
&emsp;`OS_dispatch_source` 是继承自 `OS_dispatch_object` 的类，然后 `dispatch_source_t` 是一个指向 `OS_dispatch_source` 的指针。
+ 在 Objective-C 下宏定义展开是:
```c++
@protocol OS_dispatch_source <OS_dispatch_object>
@end
typedef NSObject<OS_dispatch_source> * dispatch_source_t;
```
&emsp;`OS_dispatch_source` 是继承自 `OS_dispatch_object` 协议的协议，并且为遵循该协议的 `NSObject` 实例对象类型的指针定义了一个 `dispatch_source_t` 的别名。
+ 在 C++ 下宏定义展开是:
```c++
typedef struct dispatch_source_s : public dispatch_object_s {} * dispatch_source_t;
```
&emsp;`dispatch_source_t` 是一个指向 `dispatch_source_s` 结构体的指针。
+ 在 C（Plain C）下宏定义展开是:
```c++
typedef struct dispatch_source_s *dispatch_source_t
```
&emsp;`dispatch_source_t` 是指向 `struct dispatch_source_s` 的指针。
### dispatch_source_type_t
&emsp;`dispatch_source_type_t` 定义类型别名。此类型的常量表示调度源（dispatch source）正在监视的低级系统对象的类（class of low-level system object）。此类型的常量作为参数传递给 `dispatch_source_create` 函数并确定如何解释 handle 参数（handle argument ）（i.e. as a file descriptor（文件描述符）, mach port, signal number, process identifier, etc.）以及如何解释 mask 参数（mask argument）。
```c++
typedef const struct dispatch_source_type_s *dispatch_source_type_t;
```
### DISPATCH_EXPORT
&emsp;不同平台下的 `extern` 外联标识。
```c++
#if defined(_WIN32)
#if defined(__cplusplus)
#define DISPATCH_EXPORT extern "C" __declspec(dllimport)
#else
#define DISPATCH_EXPORT extern __declspec(dllimport)
#endif
#elif __GNUC__
#define DISPATCH_EXPORT extern __attribute__((visibility("default")))
#else
#define DISPATCH_EXPORT extern
#endif
```
### DISPATCH_SOURCE_TYPE_DECL
&emsp;先学习展开一下 `DISPATCH_SOURCE_TYPE_DECL` 宏定义，下面多处都要用到它。
```c++
DISPATCH_SOURCE_TYPE_DECL(data_add);
```
+ 在 Swift（在 Swift 中使用 Objective-C）下宏定义展开是:
```c++
extern struct dispatch_source_type_s _dispatch_source_type_data_add;
@protocol OS_dispatch_source_data_add <OS_dispatch_source>
@end

@interface OS_dispatch_source () <OS_dispatch_source_data_add>
@end
```
+ 在 Objective-C/C++/C 下宏定义展开是:
```c++
extern const struct dispatch_source_type_s _dispatch_source_type_data_add;
```
### DISPATCH_SOURCE_TYPE_DATA_ADD
&emsp;一种调度源（dispatch source），它合并通过调用 `dispatch_source_merge_data` 获得的数据。ADD 用于合并数据。句柄未使用（暂时传递零），mask 未使用（暂时传递零）。
```c++
#define DISPATCH_SOURCE_TYPE_DATA_ADD (&_dispatch_source_type_data_add)
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_SOURCE_TYPE_DECL(data_add);
```
### DISPATCH_SOURCE_TYPE_DATA_OR
&emsp;一种调度源（dispatch source），它合并通过调用 `dispatch_source_merge_data` 获得的数据。按位或进行合并数据。句柄未使用（暂时传递零），mask 未使用（暂时传递零）。
```c++
#define DISPATCH_SOURCE_TYPE_DATA_OR (&_dispatch_source_type_data_or)
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_SOURCE_TYPE_DECL(data_or);
```
### DISPATCH_SOURCE_TYPE_DATA_REPLACE
&emsp;一种调度源（dispatch source），它跟踪通过调用 `dispatch_source_merge_data` 获得的数据。新获得的数据值替换了尚未传递到源处理程序（source handler）的现有数据值。数据值为零将不调用源处理程序（source handler）。句柄未使用（暂时传递零），mask 未使用（暂时传递零）。
```c++
#define DISPATCH_SOURCE_TYPE_DATA_REPLACE (&_dispatch_source_type_data_replace)
API_AVAILABLE(macos(10.13), ios(11.0), tvos(11.0), watchos(4.0))
DISPATCH_SOURCE_TYPE_DECL(data_replace);
```
### DISPATCH_SOURCE_TYPE_MACH_SEND
&emsp;一种调度源（dispatch source），用于监视 Mach port 的 dead name 通知（发送权限不再具有任何相应的接收权限）。句柄（handle）是一个 Mach port，具有 send 或 send once right（mach_port_t）。mask 是 `dispatch_source_mach_send_flags_t` 中所需事件的 mask。
```c++
#define DISPATCH_SOURCE_TYPE_MACH_SEND (&_dispatch_source_type_mach_send)
API_AVAILABLE(macos(10.6), ios(4.0)) DISPATCH_LINUX_UNAVAILABLE()
DISPATCH_SOURCE_TYPE_DECL(mach_send);
```
### DISPATCH_SOURCE_TYPE_MACH_RECV
&emsp;一种调度源（dispatch source），用于监视 Mach port 中的挂起消息。句柄（handle）是具有接收权限（mach_port_t）的 Mach port。mask 是来自 `dispatch_source_mach_recv_flags_t` 中所需事件的 mask，但是当前未定义任何标志（现在传递零）。
```c++
#define DISPATCH_SOURCE_TYPE_MACH_RECV (&_dispatch_source_type_mach_recv)
API_AVAILABLE(macos(10.6), ios(4.0)) DISPATCH_LINUX_UNAVAILABLE()
DISPATCH_SOURCE_TYPE_DECL(mach_recv);
```
### DISPATCH_SOURCE_TYPE_MEMORYPRESSURE
&emsp;一种调度源（dispatch source），用于监视系统内存压力状况的变化。该句柄（handle）未使用（现在传递零）。mask 是来自 `dispatch_source_mach_recv_flags_t` 中所需事件的 mask。
```c++
#define DISPATCH_SOURCE_TYPE_MEMORYPRESSURE \
        (&_dispatch_source_type_memorypressure)
API_AVAILABLE(macos(10.9), ios(8.0)) DISPATCH_LINUX_UNAVAILABLE()
DISPATCH_SOURCE_TYPE_DECL(memorypressure);
```
### DISPATCH_SOURCE_TYPE_PROC
&emsp;一种调度源（dispatch source），用于监视外部进程中由 `dispatch_source_proc_flags_t` 定义的事件。句柄（handle）是进程标识符（pid_t）。mask 是来自 `dispatch_source_mach_recv_flags_t` 中所需事件的 mask。
```c++
#define DISPATCH_SOURCE_TYPE_PROC (&_dispatch_source_type_proc)
API_AVAILABLE(macos(10.6), ios(4.0)) DISPATCH_LINUX_UNAVAILABLE()
DISPATCH_SOURCE_TYPE_DECL(proc);
```
### DISPATCH_SOURCE_TYPE_READ
&emsp;一种调度源（dispatch source），用于监视文件描述符的待处理字节，以获取可读取的字节。句柄（handle）是文件描述符（int）。mask 未使用（现在传递零）。
```c++
#define DISPATCH_SOURCE_TYPE_READ (&_dispatch_source_type_read)
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_SOURCE_TYPE_DECL(read);
```
### DISPATCH_SOURCE_TYPE_SIGNAL
&emsp;监视当前进程以获取信号的调度源（dispatch source）。句柄（handle）是信号编号（int）。mask 未使用（现在传递零）。
```c++
#define DISPATCH_SOURCE_TYPE_SIGNAL (&_dispatch_source_type_signal)
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_SOURCE_TYPE_DECL(signal);
```
### DISPATCH_SOURCE_TYPE_TIMER
&emsp;基于计时器（based on a timer）提交（submits）事件处理程序块（event handler block）的调度源（dispatch source）。句柄（handle）未使用（现在传递零）。mask 指定要应用的来自 `dispatch_source_timer_flags_t` 的标志。
```c++
#define DISPATCH_SOURCE_TYPE_TIMER (&_dispatch_source_type_timer)
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_SOURCE_TYPE_DECL(timer);
```
### DISPATCH_SOURCE_TYPE_VNODE
&emsp;一种调度源（dispatch source），它监视由 `dispatch_source_vnode_flags_t` 定义的事件的文件描述符。句柄（handle）是文件描述符（int）。mask 是来自 `dispatch_source_vnode_flags_t` 的所需事件的 mask。
```c++
#define DISPATCH_SOURCE_TYPE_VNODE (&_dispatch_source_type_vnode)
API_AVAILABLE(macos(10.6), ios(4.0)) DISPATCH_LINUX_UNAVAILABLE()
DISPATCH_SOURCE_TYPE_DECL(vnode);
```
### DISPATCH_SOURCE_TYPE_WRITE
&emsp;一种调度源（dispatch source），它监视文件描述符以获取可用于写入字节的缓冲区空间。句柄（handle）是文件描述符（int）。mask 未使用（现在传递零）。
```c++
#define DISPATCH_SOURCE_TYPE_WRITE (&_dispatch_source_type_write)
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_SOURCE_TYPE_DECL(write);
```
### dispatch_source_mach_send_flags_t
&emsp;`dispatch_source_mach_send` 标志的类型。
```c++
// 表示与给定发送权限对应的接收权限已销毁。
#define DISPATCH_MACH_SEND_DEAD    0x1

typedef unsigned long dispatch_source_mach_send_flags_t;
```
### dispatch_source_mach_recv_flags_t
&emsp;`dispatch_source_mach_recv` 标志的类型。
```c++
typedef unsigned long dispatch_source_mach_recv_flags_t;
```
### dispatch_source_memorypressure_flags_t
&emsp;`dispatch_source_memorypressure` 标志的类型。
+ `DISPATCH_MEMORYPRESSURE_NORMAL` 系统内存压力状况已恢复正常。
+ `DISPATCH_MEMORYPRESSURE_WARN` 系统内存压力状况已更改为警告。
+ `DISPATCH_MEMORYPRESSURE_CRITICAL` 系统内存压力状况已变为严重。
&emsp;内存压力升高是一种系统范围内的情况，为此源注册的应用程序应通过更改其将来的内存使用行为来作出反应，例如：通过减少新启动操作的缓存大小，直到内存压力恢复正常。
&emsp;注意：当系统内存压力进入提升状态时，应用程序不应遍历并丢弃现有缓存以进行过去的操作，因为这很可能会触发 VM 操作，从而进一步加剧系统内存压力。
```c++
#define DISPATCH_MEMORYPRESSURE_NORMAL        0x01
#define DISPATCH_MEMORYPRESSURE_WARN        0x02
#define DISPATCH_MEMORYPRESSURE_CRITICAL    0x04

typedef unsigned long dispatch_source_memorypressure_flags_t;
```
### dispatch_source_proc_flags_t
&emsp;`dispatch_source_proc` 标志的类型。
+ `DISPATCH_PROC_EXIT` 该进程已经退出（也许是 cleanly，也许不是）。
+ `DISPATCH_PROC_FORK` 该进程已创建一个或多个子进程。
+ `DISPATCH_PROC_EXEC` 通过 `exec *()` 或 `posix_spawn *()`，该进程已成为另一个可执行映像（executable image）。
+ `DISPATCH_PROC_SIGNAL` Unix 信号已传递到该进程。
```c++
#define DISPATCH_PROC_EXIT        0x80000000
#define DISPATCH_PROC_FORK        0x40000000
#define DISPATCH_PROC_EXEC        0x20000000
#define DISPATCH_PROC_SIGNAL    0x08000000

typedef unsigned long dispatch_source_proc_flags_t;
```
### dispatch_source_vnode_flags_t
&emsp;`dispatch_source_vnode` 标志的类型。
+ `DISPATCH_VNODE_DELETE` filesystem 对象已从 namespace 中删除。
+ `DISPATCH_VNODE_WRITE` filesystem 对象数据已更改。
+ `DISPATCH_VNODE_EXTEND` filesystem 对象的大小已更改。
+ `DISPATCH_VNODE_ATTRIB` filesystem 对象 metadata 已更改。
+ `DISPATCH_VNODE_LINK` filesystem 对象 link计数已更改。
+ `DISPATCH_VNODE_RENAME` filesystem 对象在 namespace 中被重命名。
+ `DISPATCH_VNODE_REVOKE` filesystem 对象被 revoked。
+ `DISPATCH_VNODE_FUNLOCK` filesystem 对象已解锁。

```c++
#define DISPATCH_VNODE_DELETE    0x1
#define DISPATCH_VNODE_WRITE    0x2
#define DISPATCH_VNODE_EXTEND    0x4
#define DISPATCH_VNODE_ATTRIB    0x8
#define DISPATCH_VNODE_LINK        0x10
#define DISPATCH_VNODE_RENAME    0x20
#define DISPATCH_VNODE_REVOKE    0x40
#define DISPATCH_VNODE_FUNLOCK    0x100

typedef unsigned long dispatch_source_vnode_flags_t;
```
### dispatch_source_timer_flags_t
&emsp;`dispatch_source_timer` 标志的类型。
+ `DISPATCH_TIMER_STRICT` 指定系统应尽最大努力严格遵守通过 `dispatch_source_set_timer` 为计时器指定的 leeway value，even if that value is smaller than the default leeway value that would be applied to the timer otherwise. 即使指定了此标志，也会有 minimal amount of leeway 应用于计时器。注意：使用此标志可能会覆盖系统采用的节电（power-saving）技术，并导致更高的功耗，因此必须谨慎使用它，并且仅在绝对必要时使用。
```c++
#define DISPATCH_TIMER_STRICT 0x1

typedef unsigned long dispatch_source_timer_flags_t;
```
### dispatch_source_create
&emsp;`dispatch_source_create` 创建一个新的调度源（dispatch source）来监视低级系统对象（low-level system objects），并根据事件自动将处理程序块（handler block）提交给调度队列（dispatch queue）。
```c++
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_EXPORT DISPATCH_MALLOC DISPATCH_RETURNS_RETAINED DISPATCH_WARN_RESULT
DISPATCH_NOTHROW
dispatch_source_t
dispatch_source_create(dispatch_source_type_t type,
    uintptr_t handle,
    unsigned long mask,
    dispatch_queue_t _Nullable queue);
```
&emsp;Dispatch sources 不可重入。在调度源被挂起或事件处理程序块当前正在执行时，接收到的任何事件都将在调度源恢复或事件处理程序块返回后合并和传递。

&emsp;`Dispatch sources` 在非活动状态下创建。创建源并设置任何所需的属性（即处理程序，上下文等）之后，必须调用 `dispatch_activate` 才能开始事件传递。

&emsp;一旦被激活，就不允许在源上调用 `dispatch_set_target_queue`（参阅 `dispatch_activate` 和 `dispatch_set_target_queue`）。

&emsp;出于向后兼容性的原因，在非活动且未暂停的源上的 `dispatch_resume` 与调用 `dispatch_activate` 具有相同的效果。对于新代码，首选使用 `dispatch_activate`。

&emsp;`type`：声明调度源的类型。必须是已定义的 `dispatch_source_type_t` 常量之一。

&emsp;`handle`：要监视的基础系统句柄（handle）。此参数的解释由 `type` 参数中提供的常量确定。

&emsp;`mask`：指定所需事件的标志 mask。此参数的解释由 `type` 参数中提供的常量确定。

&emsp;`queue`：事件处理程序块（event handler block）将提交到的调度队列。如果队列为 `DISPATCH_TARGET_QUEUE_DEFAULT`，则源（source）将事件处理程序块提交到默认优先级全局队列。

&emsp;`result`：新创建的调度源（dispatch source）。如果传递了无效的参数，则为 `NULL`。
### dispatch_source_set_event_handler
&emsp;为给定的调度源（dispatch source）设置事件处理程序块（event handler block）。
```c++
#ifdef __BLOCKS__
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NOTHROW
void
dispatch_source_set_event_handler(dispatch_source_t source,
    dispatch_block_t _Nullable handler);
#endif /* __BLOCKS__ */
```
&emsp;`source`：要进行修改的调度源。在此参数中传递 `NULL` 的结果是未定义的

&emsp;`handler`：事件处理程序块将提交到源的目标队列。
### dispatch_source_set_event_handler_f
&emsp;为给定的调度源设置事件处理函数。
```c++
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NOTHROW
void
dispatch_source_set_event_handler_f(dispatch_source_t source,
    dispatch_function_t _Nullable handler);
```
&emsp;`handler`：事件处理程序函数提交到源的目标队列。传递给事件处理程序（函数）的 context 参数是设置事件处理程序时当前调度源的上下文。
### dispatch_source_set_cancel_handler
&emsp;为给定的调度源设置取消处理程序块（cancellation handler block）。
```c++
#ifdef __BLOCKS__
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NOTHROW
void
dispatch_source_set_cancel_handler(dispatch_source_t source,
    dispatch_block_t _Nullable handler);
#endif /* __BLOCKS__ */
```
&emsp;一旦系统释放了对源基础句柄的所有引用，并且返回了源的事件处理程序块，则取消处理程序（如果已指定）将被提交到源的目标队列，以响应对 `dispatch_source_cancel` 的调用。

&emsp;IMPORTANT：file descriptor 和基于 mach port 的源需要源取消（source cancellation）和取消处理程序（a cancellation handler），以便安全地关闭描述符或销毁端口。在调用取消处理程序之前关闭描述符或端口可能会导致竞态。如果在源的事件处理程序仍在运行时为新描述符分配了与最近关闭的描述符相同的值，则事件处理程序可能会将数据读/写到错误的描述符。

&emsp;`handler`：取消处理程序块将提交到源的目标队列。
### dispatch_source_set_cancel_handler_f
&emsp;设置给定调度源的取消处理函数。
```c++
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NOTHROW
void
dispatch_source_set_cancel_handler_f(dispatch_source_t source,
    dispatch_function_t _Nullable handler);
```
&emsp;同上 `dispatch_source_set_cancel_handler` 函数。
### dispatch_source_cancel
&emsp;异步取消调度源（dispatch source），以防止进一步调用其事件处理程序块。
```c++
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_NOTHROW
void
dispatch_source_cancel(dispatch_source_t source);
```
&emsp;取消操作（dispatch_source_cancel）将阻止对指定调度源的事件处理程序块（event handler block）的任何进一步调用，但不会中断已在进行中的事件处理程序块。

&emsp;一旦源的事件处理程序完成，取消处理程序将提交到源的目标队列，这表明现在可以安全地关闭源的句柄（i.e. file descriptor or mach port）。
### dispatch_source_testcancel
&emsp;测试给定的调度源是否已取消。
```c++
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_WARN_RESULT DISPATCH_PURE
DISPATCH_NOTHROW
long
dispatch_source_testcancel(dispatch_source_t source);
```
&emsp;`result`：取消则非零，未取消则为零。
### dispatch_source_get_handle
&emsp;返回与此调度源关联的基础系统句柄（underlying system handle）。
```c++
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_WARN_RESULT DISPATCH_PURE
DISPATCH_NOTHROW
uintptr_t
dispatch_source_get_handle(dispatch_source_t source);
```
&emsp;返回值应根据调度源的类型进行解释，并且可以是以下句柄之一:
```c++
DISPATCH_SOURCE_TYPE_DATA_ADD:        n/a
DISPATCH_SOURCE_TYPE_DATA_OR:         n/a
DISPATCH_SOURCE_TYPE_DATA_REPLACE:    n/a
DISPATCH_SOURCE_TYPE_MACH_SEND:       mach port (mach_port_t)
DISPATCH_SOURCE_TYPE_MACH_RECV:       mach port (mach_port_t)
DISPATCH_SOURCE_TYPE_MEMORYPRESSURE   n/a
DISPATCH_SOURCE_TYPE_PROC:            process identifier (pid_t)
DISPATCH_SOURCE_TYPE_READ:            file descriptor (int)
DISPATCH_SOURCE_TYPE_SIGNAL:          signal number (int)
DISPATCH_SOURCE_TYPE_TIMER:           n/a
DISPATCH_SOURCE_TYPE_VNODE:           file descriptor (int)
DISPATCH_SOURCE_TYPE_WRITE:           file descriptor (int)
```
### dispatch_source_get_mask
&emsp;返回由调度源监视的事件的 mask。
```c++
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_WARN_RESULT DISPATCH_PURE
DISPATCH_NOTHROW
unsigned long
dispatch_source_get_mask(dispatch_source_t source);
```
&emsp;`result`：返回值应根据调度源的类型进行解释，并且可以是以下 flag 之一：
```c++
DISPATCH_SOURCE_TYPE_DATA_ADD:        n/a
DISPATCH_SOURCE_TYPE_DATA_OR:         n/a
DISPATCH_SOURCE_TYPE_DATA_REPLACE:    n/a
DISPATCH_SOURCE_TYPE_MACH_SEND:       dispatch_source_mach_send_flags_t
DISPATCH_SOURCE_TYPE_MACH_RECV:       dispatch_source_mach_recv_flags_t
DISPATCH_SOURCE_TYPE_MEMORYPRESSURE   dispatch_source_memorypressure_flags_t
DISPATCH_SOURCE_TYPE_PROC:            dispatch_source_proc_flags_t
DISPATCH_SOURCE_TYPE_READ:            n/a
DISPATCH_SOURCE_TYPE_SIGNAL:          n/a
DISPATCH_SOURCE_TYPE_TIMER:           dispatch_source_timer_flags_t
DISPATCH_SOURCE_TYPE_VNODE:           dispatch_source_vnode_flags_t
DISPATCH_SOURCE_TYPE_WRITE:           n/a
```
### dispatch_source_get_data
&emsp;返回调度源的待处理数据。
```c++
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_WARN_RESULT DISPATCH_PURE
DISPATCH_NOTHROW
unsigned long
dispatch_source_get_data(dispatch_source_t source);
```
&emsp;该函数旨在从事件处理程序块中调用。在事件处理程序回调之外调用此函数的结果是未定义的。

&emsp;`result`：返回值应根据调度源的类型进行解释，并且可以是以下之一：
```c++
DISPATCH_SOURCE_TYPE_DATA_ADD:        application defined data
DISPATCH_SOURCE_TYPE_DATA_OR:         application defined data
DISPATCH_SOURCE_TYPE_DATA_REPLACE:    application defined data
DISPATCH_SOURCE_TYPE_MACH_SEND:       dispatch_source_mach_send_flags_t
DISPATCH_SOURCE_TYPE_MACH_RECV:       dispatch_source_mach_recv_flags_t
DISPATCH_SOURCE_TYPE_MEMORYPRESSURE   dispatch_source_memorypressure_flags_t
DISPATCH_SOURCE_TYPE_PROC:            dispatch_source_proc_flags_t
DISPATCH_SOURCE_TYPE_READ:            estimated bytes available to read
DISPATCH_SOURCE_TYPE_SIGNAL:          number of signals delivered since the last handler invocation
DISPATCH_SOURCE_TYPE_TIMER:           number of times the timer has fired since the last handler invocation
DISPATCH_SOURCE_TYPE_VNODE:           dispatch_source_vnode_flags_t
DISPATCH_SOURCE_TYPE_WRITE:           estimated buffer space available
```
### dispatch_source_merge_data
&emsp;将数据合并到类型为 `DISPATCH_SOURCE_TYPE_DATA_ADD`，`DISPATCH_SOURCE_TYPE_DATA_OR` 或 `DISPATCH_SOURCE_TYPE_DATA_REPLACE` 的调度源中，并将其事件处理程序块提交到其目标队列。
```c++
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_NOTHROW
void
dispatch_source_merge_data(dispatch_source_t source, unsigned long value);
```
&emsp;`value`：使用调度源类型指定的逻辑 OR 或 ADD 与待处理数据合并的值。零值无效并且也不会导致事件处理程序块的提交。
### dispatch_source_set_timer
&emsp;设置 timer source 的开始时间，间隔和回程值（leeway value）。
```c++
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_EXPORT DISPATCH_NONNULL_ALL DISPATCH_NOTHROW
void
dispatch_source_set_timer(dispatch_source_t source,
    dispatch_time_t start,
    uint64_t interval,
    uint64_t leeway);
```
&emsp;此函数返回后，将清除先前计时器值累积的所有未决源数据；计时器的下一次触发将在 `start` 时发生，此后每隔 `interval` 纳秒，直到计时器源被取消。

&emsp;系统可能会延迟计时器的任何触发时间，以改善功耗和系统性能。允许延迟的上限可以使用 `leeway` 参数进行配置，下限在系统的控制下。

&emsp;对于 `start` 时的初始计时器触发，允许延迟的上限设置为 `leeway` 纳秒。对于随后的计时器以 `start + N * interval` 触发的情况，上限为 `MIN（leeway，interval / 2）`。

&emsp;允许延迟的下限可能随过程状态（例如应用程序 UI 的可见性）而变化。如果指定的计时器源是使用 `DISPATCH_TIMER_STRICT` 的 mask 创建的，则系统将尽最大努力严格遵守所提供的 `leeway` 值，即使该值小于当前下限。请注意，即使指定了此标志，也希望有最小的延迟量。

&emsp;`start` 参数还确定将使用哪个时钟作为计时器：如果 `start` 是 `DISPATCH_TIME_NOW` 或由 `dispatch_time(3)` 创建的，则计时器基于正常运行时间（从 Apple 平台上的 `mach_absolute_time` 获取） 。如果使用 `dispatch_walltime(3)` 创建了 `start`，则计时器基于 `gettimeofday(3)`。

&emsp;如果 timer source 已被取消，则调用此函数无效。

&emsp;`start`：计时器的开始时间。

&emsp;`interval`：计时器的纳秒间隔。将 `DISPATCH_TIME_FOREVER` 用于一键式计时器（a one-shot timer）。

&emsp;`leeway`：timer 的纳秒 leeway。
### dispatch_source_set_registration_handler
&emsp;设置给定调度源的注册处理程序块（registration handler block）。
```c++
#ifdef __BLOCKS__
API_AVAILABLE(macos(10.7), ios(4.3))
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NOTHROW
void
dispatch_source_set_registration_handler(dispatch_source_t source,
    dispatch_block_t _Nullable handler);
#endif /* __BLOCKS__ */
```
&emsp;一旦相应的 `kevent` 已在源中的初始 `dispatch_resume` 之后向系统注册，注册处理程序（如果已指定）将被提交到源的目标队列。

&emsp;如果在设置注册处理程序时已经注册了源，则会立即调用注册处理程序。

&emsp;`handler`：注册处理程序块将提交到源的目标队列。
### dispatch_source_set_registration_handler_f
&emsp;设置给定调度源（dispatch source）的注册处理函数。
```c++
API_AVAILABLE(macos(10.7), ios(4.3))
DISPATCH_EXPORT DISPATCH_NONNULL1 DISPATCH_NOTHROW
void
dispatch_source_set_registration_handler_f(dispatch_source_t source,
    dispatch_function_t _Nullable handler);
```
&emsp;同上 `dispatch_source_set_registration_handler`。


## 参考链接
**参考链接:🔗**
+ [swift-corelibs-libdispatch-main](https://github.com/apple/swift-corelibs-libdispatch)
+ [Dispatch 官方文档](https://developer.apple.com/documentation/dispatch?language=objc)
+ [iOS libdispatch浅析](https://juejin.im/post/6844904143174238221)
+ [GCD--百度百科词条](https://baike.baidu.com/item/GCD/2104053?fr=aladdin)
+ [iOS多线程：『GCD』详尽总结](https://juejin.im/post/6844903566398717960)
+ [iOS底层学习 - 多线程之GCD初探](https://juejin.im/post/6844904096973979656)
+ [GCD 中的类型](https://blog.csdn.net/u011374318/article/details/87870585)
+ [iOS Objective-C GCD之queue（队列）篇](https://www.jianshu.com/p/d0017f74f9ca)
+ [变态的libDispatch结构分析-object结构](https://blog.csdn.net/passerbysrs/article/details/18223845)
