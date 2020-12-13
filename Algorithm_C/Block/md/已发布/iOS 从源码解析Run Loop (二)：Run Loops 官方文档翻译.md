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

&emsp;表 3-1 预定义的 run loop modes
| Mode | Name | Description |
| - | - | - |
| Default | NSDefaultRunLoopMode (Cocoa) kCFRunLoopDefaultMode (Core Foundation) | 默认模式是用于大多数操作的模式。大多数时候，你应该使用此模式启动 run loop 并配置 input sources。 |
| Connection | NSConnectionReplyMode (Cocoa) | Cocoa 将此模式与 NSConnection 对象结合使用来监视响应。你应该很少需要自己使用这种模式。 |
| Modal | NSModalPanelRunLoopMode (Cocoa) | Cocoa 使用此模式来识别用于 modal panels 的事件。 |
| Event tracking | NSEventTrackingRunLoopMode (Cocoa) | Cocoa 使用此模式在鼠标拖动循环和其他类型的用户界面跟踪循环期间限制传入事件。 |
| Common modes | NSRunLoopCommonModes (Cocoa) kCFRunLoopCommonModes (Core Foundation) | 这是一组可配置的常用模式。将 input source 与此模式关联也会将其与组中的每个模式相关联。对于 Cocoa 应用程序，默认情况下，此集合包括 default、modal、event tracking 模式。Core Foundation 最初只包括 default mode。你可以使用 CFRunLoopAddCommonMode 函数向集合添加自定义模式。 |

## Input Sources
&emsp;Input sources 将事件异步传递到你的线程。事件的 source 取决于 input source 的类型，通常是两个类别之一（Input source 和 Timer source 两种类型）。基于端口的输入源（port-based input sources）监视你的应用程序的 Mach 端口。定制输入源监视事件的定制源。就你的 run loop 而言，input source 是基于端口（port-based）的还是定制的（custom）都无关紧要。系统通常实现两种类型的输入源，你可以按原样使用。两种信号源之间的唯一区别是信号的发送方式。基于端口的源（port-based sources）由内核（kernel）自动发出信号，而自定义源（custom sources）必须从另一个线程手动发出信号。

&emsp;创建 input source 时，可以将其分配给 run loop 的一种或多种 modes。Modes 会影响在任何给定时刻监视哪些 input sources。大多数情况下，你会在默认模式下运行 run loop，但也可以指定自定义模式（custom modes）。如果 input sources 不在当前监视的 mode 下，则它生成的任何事件都将保留，直到 run loop 以对应（correct）的 mode 运行。

&emsp;以下各节描述了一些 input sources。
### Port-Based Sources（基于端口的 source）
&emsp;Cocoa 和 Core Foundation 为使用端口相关（port-related）的对象和函数创建基于端口的输入源（port-based input sources）提供了内置支持（provide built-in support）。例如，在 Cocoa 中，你根本不需要直接创建 input source。你只需创建一个端口对象（NSPort object），然后使用 NSPort 的方法将该端口添加到 run loop 中即可。port object 为你处理所需 input source 的创建和配置。

&emsp;在 Core Foundation 中，你必须手动创建端口及其运行循环源（run loop source）。在这两种情况下，都可以使用与端口不透明类型（CFMachPortRef、CFMessagePortRef 或 CFSocketRef）关联的函数来创建适当的对象。

&emsp;有关如何设置和配置基于端口的定制源的示例，参考下文 Configuring a Port-Based Input Source。
### Custom Input Sources（定制输入源）
&emsp;要创建自定义输入源，必须使用与 Core Foundation 中的 CFRunLoopSourceRef 不透明类型关联的函数。你可以使用几个回调函数（callback functions）配置一个自定义输入源（custom input source ）。Core Foundation 在不同的点调用这些函数来配置 source，处理任何传入事件，并在 source 从 run loop 中移除后时将其销毁。

&emsp;除了定义（defining）事件到达（event arrives）时自定义源（custom source）的行为（behavior）外，还必须定义事件传递机制（event delivery mechanism）。这部分 source（this part of the source）运行在一个单独的线程上，负责向输入源（input source）提供其数据，并在数据准备好进行处理时发出信号。事件传递机制（event delivery mechanism）由你决定，但不必过于复杂。

&emsp;有关如何创建自定义输入源的示例，请参阅 Defining a Custom Input Source。有关自定义输入源的参考信息，请参考 CFRunLoopSource Reference。
### Cocoa Perform Selector Sources
&emsp;除了基于端口的源（port-based sources）外，Cocoa 还定义了一个自定义输入源（custom input source），允许你在任何线程上执行 selector。与基于端口的源（port-based source）一样，perform selector 请求在目标线程上序列化（serialized），从而减轻了在一个线程上运行多个方法时可能出现的许多同步问题。与基于端口的源不同，执行选择器源（perform selector source）在执行其 selector 后将自身从 run loop 中移除。

&emsp;Note：在 OS X v10.5 之前，执行选择器源主要用于将消息发送到主线程，但是在 OS X v10.5 和更高版本以及 iOS 中，可以使用它们将消息发送到任何线程。

&emsp;在另一个线程上执行 selector 时，目标线程必须具有活动的 run loop。对于你创建的子线程，这意味着等待直到你的代码显式启动当前线程的 run loop。但是，由于主线程启动了自己的 run loop，因此你可以在应用程序调用应用程序委托的 `applicationDidFinishLaunching:` 函数后立即开始在主线程上发出调用（添加 selector 在主线程执行）。每次循环时，run loop 都会处理队列中所有的执行 selector 的调用，而不是在每次循环迭代时仅处理一个。

&emsp;表 3-2 列出了在 NSObject 上定义的可用于在其它线程上执行 selector 的方法。因为这些方法是在 NSObject 上声明的（NSObject 的 NSThreadPerformAdditions 和 NSDelayedPerforming 分类中声明 ），所以你可以从任何可以访问 Objective-C 对象的线程中使用它们，包括 POSIX 线程。这些方法实际上并不创建新线程来执行选择器，仅在你指定的或当前的开启了 run loop 的线程中执行。

&emsp;表 3-2 在指定的 thread 执行 selector
| Methods | Description |
| - | - |
| performSelectorOnMainThread:withObject:waitUntilDone: performSelectorOnMainThread:withObject:waitUntilDone:modes: | 在应用程序的主线程的下一个 run loop 周期中执行指定的 selector。这些方法提供了在执行 selector 之前阻塞当前线程的选项，即 waitUntilDone 参数表示是否等待 selector 执行完成再执行接下来的语句。 |
| performSelector:onThread:withObject:waitUntilDone:
performSelector:onThread:withObject:waitUntilDone:modes: | 在具有 NSThread 对象的任何线程上执行指定的选择器。这些方法使你可以选择阻塞当前线程，直到 selector 执行完成为止。 |
| performSelector:withObject:afterDelay:
performSelector:withObject:afterDelay:inModes: | 在下一个 run loop 周期中以及可选的延迟时间之后，在当前线程上执行指定的 selector。因为它一直等到下一个 run loop 周期执行 selector，所以这些方法提供了当前执行代码的最小自动延迟（执行等待时间至少大于等于 afterDelay 参数）。多个排队的 selectors 按照排队的顺序依次执行。 |
| cancelPreviousPerformRequestsWithTarget:
cancelPreviousPerformRequestsWithTarget:selector:object: | 使你可以取消使用 performSelector:withObject:afterDelay: 或 performSelector:withObject:afterDelay:inModes: 方法发送到当前线程的消息。 |

&emsp;有关每种方法的详细信息，可参考 NSObject Class Reference。
## Timer Sources
&emsp;Timer sources 在将来的预设时间将事件同步传递到你的线程。Timers 是线程通知自己执行某事的一种方式。例如，搜索字段可以使用 timer 在用户连续按键之间经过一定时间后启动自动搜索，使用这个延迟时间，用户就有机会在开始搜索之前键入尽可能多的所需搜索字符串。

&emsp;尽管 timer 生成基于时间的通知（time-based notifications），但它不是一种实时机制（real-time mechanism）。与 input sources 一样，timer 与 run loop 的特定 mode 相关联。如果 timer 未处于 run loop 当前监视的模式，则在以 timer 支持的 mode 之一运行 run loop 之前，timer 不会触发。类似地，如果在 run loop 正在执行处理程序例程（handler routine）的过程中触发 timer ，则 timer 将等到下一次通过 run loop 调用其处理程序例程（handler routine）。如果 run loop 根本没有运行，计时器就不会触发。

&emsp;你可以将 timer 配置为仅生成一次事件或重复生成事件。重复 timer 根据预定的触发时间而不是实际触发时间自动重新调度自己。例如，如果一个 timer 被安排在某个特定时间触发，并且此后每隔 5 秒触发一次，那么即使实际触发时间被延迟，计划的触发时间也将始终落在原来的 5 秒时间间隔上。如果触发时间延迟太久，以致错过了一个或多个预定的触发时间，则对于错过的时间段，timer 只触发一次。在为错过的时间段触发后，timer 将重新安排为下一个预定的触发时间。

&emsp;有关配置 timer sources 的更多信息，参考 Configuring Timer Sources。有关参考信息，可参见 NSTimer Coass Reference 或 CFRunLoopTimer Reference。
## Run Loop Observers
&emsp;与在发生适当的异步或同步事件时触发的 sources 不同，run loop observers 在 run loop 本身执行期间在特殊位置激发。你可以使用 run loop observers 来准备线程来处理给定的事件，或者在线程进入休眠状态之前对其进行准备。可以将 run loop observers 与 run loop 中的以下事件关联：
+ run loop 进入入口。
+ run loop 将要处理 timer 时。
+ run loop 将要处理 input source 时。
+ run loop 即将进入睡眠状态时。
+ 当 run loop 已唤醒时，但在它处理唤醒它的事件之前。
+ run loop 退出。



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
