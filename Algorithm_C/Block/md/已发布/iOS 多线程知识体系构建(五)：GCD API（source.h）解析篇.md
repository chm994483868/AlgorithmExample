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
&emsp;一个调度源（dispatch source），它合并通过调用 `dispatch_source_merge_data` 获得的数据。ADD 用于合并数据。句柄未使用（暂时传递零），mask 未使用（暂时传递零）。
```c++
#define DISPATCH_SOURCE_TYPE_DATA_ADD (&_dispatch_source_type_data_add)
API_AVAILABLE(macos(10.6), ios(4.0))
DISPATCH_SOURCE_TYPE_DECL(data_add);
```




/*!
* @const DISPATCH_SOURCE_TYPE_DATA_ADD
* @discussion A dispatch source that coalesces data obtained via calls to dispatch_source_merge_data(). An ADD is used to coalesce the data. The handle is unused (pass zero for now). The mask is unused (pass zero for now).
*/





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
