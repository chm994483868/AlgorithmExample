# iOS 多线程知识体系构建(四)：GCD API（object.h）解析篇

> &emsp;那么继续学习 dispath 中也挺重要的 object.h 文件。

## dispatch_object_t
&emsp;`dispatch_object_t` 是所有调度对象（dispatch objects）的抽象基类型。类型定义的详细信息是特定于语言的（Swift 和 Objective-C 下不同）。调度对象通过调用 `dispatch_retain` 和 `dispatch_release` 进行引用计数。

&emsp;默认情况下，使用 Objective-C 编译器进行构建时，调度对象被声明为 Objective-C 类型。这使他们可以参与 ARC，通过 Blocks 运行时参与 RR 管理以及通过静态分析器参与泄漏检查，并将它们添加到 Cocoa 集合（NSMutableArray、NSMutableDictionary...）中。有关详细信息，参见 <os/object.h> 。
```c++
OS_OBJECT_DECL_CLASS(dispatch_object);
```
&emsp;在 Objective-C 下宏定义展开即为:
```c++
@protocol OS_dispatch_object <NSObject> 
@end
typedef NSObject<OS_dispatch_object> * dispatch_object_t;  
```
&emsp;`OS_dispatch_object` 是继承自 `NSObject` 协议的协议，并且为遵循该协议的 `NSObject` 实例对象类型的指针定义了一个 `dispatch_object_t` 的别名。（`dispatch_object_t` 具体是不是 NSObject 后面待确认）
&emsp;下面看一下 <os/object.h> 文件。
## <os/object.h> 文件
>
>  @header
>  @preprocinfo
>  &emsp;By default, libSystem objects such as GCD and XPC objects are declared as
>  Objective-C types when building with an Objective-C compiler. This allows
>  them to participate in ARC, in RR management by the Blocks runtime and in
>  leaks checking by the static analyzer, and enables them to be added to Cocoa
>  collections.
> 
>  NOTE: this requires explicit cancellation of dispatch sources and xpc
>        connections whose handler blocks capture the source/connection object,
>        resp. ensuring that such captures do not form retain cycles (e.g. by
>        declaring the source as __weak).
> 
>  &emsp;To opt-out of this default behavior, add -DOS_OBJECT_USE_OBJC=0 to your
>  compiler flags.
> 
>  &emsp;This mode requires a platform with the modern Objective-C runtime, the
>  Objective-C GC compiler option to be disabled, and at least a Mac OS X 10.8
>  or iOS 6.0 deployment target.
> 
> &emsp;默认情况下，在使用 Objective-C 编译器进行构建时，libSystem 对象（例如 GCD 和 XPC 对象）被声明为 Objective-C 类型。这使他们可以参与 ARC，通过 Blocks 运行时参与 RR 管理以及通过静态分析器参与泄漏检查，并将它们添加到 Cocoa 集合中。
> 
> &emsp;注意：这需要显式取消调度源和 xpc 连接，其处理程序块捕获源/连接对象 resp。确保此类捕获不会形成保留周期（例如，通过将来源声明为 __weak）。
>
> &emsp;要选择退出此默认行为，请将 -DOS_OBJECT_USE_OBJC = 0 添加到您的编译器标志中。
> 
> &emsp;此模式要求平台具有现代的 Objective-C 运行时，要禁用的 Objective-C GC 编译器选项以及至少一个 Mac OS X 10.8 或 iOS 6.0 部署目标。

&emsp;然后中间是一段针对 Swift 和 Objective-C 已经类型定义的一些宏，都比较简单可以

/*
* To provide backward deployment of ObjC objects in Swift on pre-10.12 SDKs, OS_object classes can be marked as OS_OBJECT_OBJC_RUNTIME_VISIBLE. When compiling with a deployment target earlier than OS X 10.12 (iOS 10.0, tvOS 10.0, watchOS 3.0) the Swift compiler will only refer to this type at runtime (using the ObjC runtime).
*/

## os_retain




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
