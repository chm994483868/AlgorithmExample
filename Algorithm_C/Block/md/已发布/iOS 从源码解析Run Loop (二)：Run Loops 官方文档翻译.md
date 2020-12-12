# iOS 从源码解析Run Loop (二)：Run Loops 官方文档翻译 

&emsp;Run loops 是与 threads 关联的基本基础结构的一部分。Run loop 是一个 event processing loop （**事件处理循环**），可用于计划工作并协调收到的事件的接收。Run loop 的目的是让 thread 在有工作要做时保持忙碌，而在没有工作时让 thread 进入睡眠状态。

&emsp;Run loop 管理不是完全自动的。你仍然必须设计 thread 的代码以在适当的时候启动 run loop 并响应传入的事件。Cocoa 和 Core Foundation 都提供运行循环对象（NSRunLoop 和 __CFRunLoop），以帮助你配置和管理 thread 的 run loop。你的应用程序不需要显式创建这些对象；每个 thread，包括应用程序的 main thread，都有一个与之关联的 run loop 对象。但是，只有子线程需要显式地运行其 run loop。在应用程序启动过程中，应用程序框架会自动在 main thread 上设置并运行其 run loop。

&emsp;以下各节提供有关 run loop 以及如何为应用程序配置 run loop 的更多信息。

## Anatomy of a Run Loop（剖析 Run Loop）
&emsp;run loop 和它的名字听起来很像。它是 thread 进入并用于运行事件处理程序以响应传入事件的循环。你的代码提供了用于实现 run loop 的实际循环部分的控制语句—换句话说，你的代码提供了驱动 run loop 的 while 或 for 循环。在循环内部，你可以使用 run loop 对象来 "run"（运行） 事件处理代码，以接收事件并调用已安装的处理程序（installed handlers）。

&emsp;Run loop 从两种不同类型的 sources 来接收事件。 Input sources 传递异步事件，通常是来自另一个 thread 或其它应用程序的消息。Timer sources（计时器源 NSTimer）传递同步事件，这些事件在计划的时间点或重复的时间间隔发生。两种类型的 source  在事件到达时都使用特定于应用程序的处理程序例程（application-specific handler routine）来处理事件。（Input sources 和 Timer sources）

&emsp;图3-1显示了 run loop 和各种 sources 的概念结构。Input sources 将异步事件（asynchronous events）传递给相应的处理程序，并使 `runUntilDate:` 方法（在 thread 关联的 NSRunLoop 对象上调用）退出（这里是不是有问题：`runMode:beforeDate:` 是使 run loop 运行一次，在 run loop 处理了第一个 input source 后会退出，`runUntilDate:` 则是通过重复调用 `runMode:beforeDate:` 一直处理 input source 直到指定的到期日期后 run loop 退出）。Timer sources 将事件传递到其处理程序例程（handler routines），但不会导致 run loop 退出。

&emsp;图3-1 run loop 的结构及其 sources

![run loop 的结构及其 sources](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/708ef62547ec4f2bba83a95382d993d9~tplv-k3u1fbpfcp-watermark.image)

&emsp;除了处理 input sources 之外，run loop 还会生成有关 run loop behavior（行为）的通知。注册的运行循环观察者（run-loop observers）可以接收这些通知，并使用它们在 thread 上进行附加处理 。你可以使用 Core Foundation 在 thread 上安装运行循环观察者（install run-loop observers）。

&emsp;以下各节提供有关 run loop 的组件及其运行方式的更多信息。它们还描述了在处理事件期间的不同时间生成的通知。

## Run Loop Modes
&emsp;Run loop mode 是要监视的 input sources 和 timers 的集合，以及要通知的 run loop observers 的集合。每次运行 run loop 时，都要（显式或隐式）指定运行的特定 "mode"。在 run loop 的整个过程中，仅监视与该 mode 关联的 sources，并允许其传递事件。
在这个 run loop 的传递过程中（During that pass of the run loop），只监视与该 mode 关联的 sources，并允许它们传递事件。（类似地，只有与该 mode 相关联的 observers 才会收到 run loop 进度（loop’s progress）的通知。）与其它 mode 相关联的 sources 将保留任何新事件，直到随后以适当的 mode 通过循环。

&emsp;在代码中，你可以通过名称识别 modes。Cocoa 和 Core Foundation 都定义了默认模式和几种常用模式，以及用于在代码中指定这些 mode 的字符串。你可以通过简单地为 mode 名称指定自定义字符串来定义自定义 mode。尽管你分配给自​​定义 mode 的名称是任意的，但是这些 mode 的内容不是任意的。你必须确保将一个或多个 input sources，timers 或 run-loop observers 添加到你创建的任何 mode 中，以使其有用。

&emsp;您可以使用模式从运行循环的特定遍历中过滤掉有害来源的事件。大多数时候，您将需要在系统定义的“默认”模式下运行运行循环。但是，模式面板可以在“模式”模式下运行。在这种模式下，只有与模式面板相关的源才将事件传递给线程。对于辅助线程，您可以使用自定义模式来防止低优先级源在时间紧迫的操作期间传递事件。

&emsp;在 run loop 的特定传递过程中，可以使用 mode 从不需要的 sources 中筛选出事件。大多数情况下，你希望以系统定义的 "default" mode 运行 run loop。然而，modal panel 可能以 "modal" mode 运行，在这种 mode 下，只有与 modal panel 相关的 sources 才会向线程传递事件。对于子线程（secondary threads），可以使用自定义 mode 来防止低优先级源（low-priority sources）在时间关键型操作（time-critical operations）期间传递事件。

&emsp;Note: Modes 的区别基于事件的 source，而不是事件的类型（type of the event）。例如，你不会使用 modes 仅匹配鼠标按下事件或仅匹配键盘事件。你可以使用 modes 来监听不同的端口集（set of ports），暂时挂起 timers，或者更改当前正在监视的 sources 和 run loop observers。

&emsp;表 3-1 列出了 Cocoa 和 Core Foundation 定义的 standard modes，以及何时使用该 mode 的说明。 name 列列出了用于在代码中指定 mode 的实际常量。

&emsp;表 3-1  预定义的 run loop modes
| Mode | Name | Description |
| - | - | - |
| Default | NSDefaultRunLoopMode (Cocoa) kCFRunLoopDefaultMode (Core Foundation) | 默认模式是用于大多数操作的模式。大多数时候，你应该使用此模式启动 run loop 并配置 input sources。 |
| Connection | NSConnectionReplyMode (Cocoa) | Cocoa 将此模式与 NSConnection 对象结合使用来监视响应。你应该很少需要自己使用这种模式。 |
| Modal | NSModalPanelRunLoopMode (Cocoa) | Cocoa 使用此模式来识别用于 modal panels 的事件。 |
| Event tracking | NSEventTrackingRunLoopMode (Cocoa) | Cocoa 使用此模式在鼠标拖动循环和其他类型的用户界面跟踪循环期间限制传入事件。 |
| Common modes | NSRunLoopCommonModes (Cocoa) kCFRunLoopCommonModes (Core Foundation) | 这是一组可配置的常用模式。将 input source 与此模式关联也会将其与组中的每个模式相关联。对于 Cocoa 应用程序，默认情况下，此集合包括 default、modal、event tracking 模式。Core Foundation 最初只包括 default mode。你可以使用 CFRunLoopAddCommonMode 函数向集合添加自定义模式。 |

## Input Sources
&emsp;












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
