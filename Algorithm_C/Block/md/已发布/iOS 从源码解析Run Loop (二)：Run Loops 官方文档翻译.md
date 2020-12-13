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

### Run Loop Modes
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

### Input Sources
&emsp;Input sources 将事件异步传递到你的线程。事件的 source 取决于 input source 的类型，通常是两个类别之一（Input source 和 Timer source 两种类型）。基于端口的输入源（port-based input sources）监视你的应用程序的 Mach 端口。定制输入源监视事件的定制源。就你的 run loop 而言，input source 是基于端口（port-based）的还是定制的（custom）都无关紧要。系统通常实现两种类型的输入源，你可以按原样使用。两种信号源之间的唯一区别是信号的发送方式。基于端口的源（port-based sources）由内核（kernel）自动发出信号，而自定义源（custom sources）必须从另一个线程手动发出信号。

&emsp;创建 input source 时，可以将其分配给 run loop 的一种或多种 modes。Modes 会影响在任何给定时刻监视哪些 input sources。大多数情况下，你会在默认模式下运行 run loop，但也可以指定自定义模式（custom modes）。如果 input sources 不在当前监视的 mode 下，则它生成的任何事件都将保留，直到 run loop 以对应（correct）的 mode 运行。

&emsp;以下各节描述了一些 input sources。
#### Port-Based Sources（基于端口的 source）
&emsp;Cocoa 和 Core Foundation 为使用端口相关（port-related）的对象和函数创建基于端口的输入源（port-based input sources）提供了内置支持（provide built-in support）。例如，在 Cocoa 中，你根本不需要直接创建 input source。你只需创建一个端口对象（NSPort object），然后使用 NSPort 的方法将该端口添加到 run loop 中即可。port object 为你处理所需 input source 的创建和配置。

&emsp;在 Core Foundation 中，你必须手动创建端口及其运行循环源（run loop source）。在这两种情况下，都可以使用与端口不透明类型（CFMachPortRef、CFMessagePortRef 或 CFSocketRef）关联的函数来创建适当的对象。

&emsp;有关如何设置和配置基于端口的定制源的示例，参考下文 Configuring a Port-Based Input Source。
#### Custom Input Sources（定制输入源）
&emsp;要创建自定义输入源，必须使用与 Core Foundation 中的 CFRunLoopSourceRef 不透明类型关联的函数。你可以使用几个回调函数（callback functions）配置一个自定义输入源（custom input source ）。Core Foundation 在不同的点调用这些函数来配置 source，处理任何传入事件，并在 source 从 run loop 中移除后时将其销毁。

&emsp;除了定义（defining）事件到达（event arrives）时自定义源（custom source）的行为（behavior）外，还必须定义事件传递机制（event delivery mechanism）。这部分 source（this part of the source）运行在一个单独的线程上，负责向输入源（input source）提供其数据，并在数据准备好进行处理时发出信号。事件传递机制（event delivery mechanism）由你决定，但不必过于复杂。

&emsp;有关如何创建自定义输入源的示例，请参阅 Defining a Custom Input Source。有关自定义输入源的参考信息，请参考 CFRunLoopSource Reference。
#### Cocoa Perform Selector Sources
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
### Timer Sources
&emsp;Timer sources 在将来的预设时间将事件同步传递到你的线程。Timers 是线程通知自己执行某事的一种方式。例如，搜索字段可以使用 timer 在用户连续按键之间经过一定时间后启动自动搜索，使用这个延迟时间，用户就有机会在开始搜索之前键入尽可能多的所需搜索字符串。

&emsp;尽管 timer 生成基于时间的通知（time-based notifications），但它不是一种实时机制（real-time mechanism）。与 input sources 一样，timer 与 run loop 的特定 mode 相关联。如果 timer 未处于 run loop 当前监视的模式，则在以 timer 支持的 mode 之一运行 run loop 之前，timer 不会触发。类似地，如果在 run loop 正在执行处理程序例程（handler routine）的过程中触发 timer ，则 timer 将等到下一次通过 run loop 调用其处理程序例程（handler routine）。如果 run loop 根本没有运行，计时器就不会触发。

&emsp;你可以将 timer 配置为仅生成一次事件或重复生成事件。重复 timer 根据预定的触发时间而不是实际触发时间自动重新调度自己。例如，如果一个 timer 被安排在某个特定时间触发，并且此后每隔 5 秒触发一次，那么即使实际触发时间被延迟，计划的触发时间也将始终落在原来的 5 秒时间间隔上。如果触发时间延迟太久，以致错过了一个或多个预定的触发时间，则对于错过的时间段，timer 只触发一次。在为错过的时间段触发后，timer 将重新安排为下一个预定的触发时间。

&emsp;有关配置 timer sources 的更多信息，参考 Configuring Timer Sources。有关参考信息，可参见 NSTimer Coass Reference 或 CFRunLoopTimer Reference。
### Run Loop Observers
&emsp;与在发生适当的异步或同步事件时触发的 sources 不同，run loop observers 在 run loop 本身执行期间在特殊位置激发。你可以使用 run loop observers 来准备线程来处理给定的事件，或者在线程进入休眠状态之前对其进行准备。可以将 run loop observers 与 run loop 中的以下事件关联：
+ run loop 进入入口。
+ run loop 将要处理 timer 时。
+ run loop 将要处理 input source 时。
+ run loop 即将进入睡眠状态时。
+ 当 run loop 已唤醒时，但在它处理唤醒它的事件之前。
+ run loop 退出。

&emsp;你可以使用 Core Foundation 向应用程序添加 run loop observers。要创建 run loop observers，需要创建 CFRunLoopObserverRef 不透明类型的新实例。此类型跟踪自定义回调函数（keeps track of your custom callback function）及其感兴趣的活动。

&emsp;与 timers 类似， run loop observers 可以使用一次或重复使用。一次性 observer 在激发后从 run loop 中移除自己，而重复 observer 保持连接。你可以指定在创建 observer 时是运行一次还是重复运行。

&emsp;有关如何创建 run loop observer 的示例，请参见 Configuring the Run Loop。有关参考信息，请参见 CFRunLoopObserver Reference。
### The Run Loop Sequence of Events
&emsp;每次运行它时，线程的 run loop 都会处理待办事件（pending events），并为所有附加的 observers 生成通知。它执行此操作的顺序非常 具体/明确，如下所示：

1. 通知 observers 已进入 run loop。
2. 通知 observers 任何准备就绪的 timers 即将触发。
3. 通知 observers 任何不基于端口（not port based）的 input sources 都将被触发。
4. 触发所有准备触发的非基于端口的输入源（non-port-based input sources）。
5. 如果基于端口的输入源（port-based input source）已准备好并等待启动，请立即处理事件。**转到步骤 9**。
6. 通知 observers，线程即将进入休眠状态。
7. 使线程进入休眠状态，直到发生以下事件之一：
  + 基于端口的输入源（port-based input source）的事件到达。
  + timers 触发。
  + 为 run loop 设置的超时时间过期。
  + run loop 被显式唤醒。
8. 通知 observer，线程刚刚唤醒。
9. 处理待办事件。
  + 如果触发了用户定义的 timer，处理 timer 事件并重新启动循环。**转到步骤2**。
  + 如果触发了 input source，传递事件。
  + 如果 run loop 被明确唤醒但尚未超时，重新启动循环。**转到步骤2**。
10. 通知 observer，run loop 已退出。

&emsp;因为 timer 和 input sources 的 observer 通知是在这些事件实际发生之前传递的，所以通知的时间和实际事件的时间之间可能会有间隔。如果这些事件之间的时间安排很关键，那么可以使用 sleep 和 aweak-from-sleep 通知来帮助你关联实际事件之间的时间安排。

&emsp;因为 timers 和其他周期性事件是在运行 run loop 时传递的，因此绕过该循环会中断这些事件的传递。这种行为的典型示例是，每当你通过进入循环并从应用程序反复请求事件来实现鼠标跟踪例程（mouse-tracking routine）时，都会发生这种行为。因为你的 code 直接撷取事件，而不是让应用程序正常 dispatch 这些事件，所以在你的鼠标跟踪例程（mouse-tracking routine）退出并将控制权传回应用程序之前，活动 timer 将无法触发（unable to fire）。

&emsp;可以使用 run loop 对象显式地唤醒 run loop。其他事件也可能导致 run loop 被唤醒。例如，添加另一个非基于端口的输入源（non-port-based input source）会唤醒 run loop，以便可以立即处理该输入源，而不是等待其他事件发生。
## When Would You Use a Run Loop?
&emsp;唯一需要显式运行 run loop 的时机是在为应用程序创建子线程时。应用程序主线程的 run loop 是基础架构的重要组成部分。因此，应用程序框架（app frameworks）提供了用于运行主应用程序循环（main application loop）并自动启动该循环的代码。 iOS 中 UIApplication 的 run 方法（或 OS X 中 NSApplication）的 run 方法将启动应用程序的主循环（application's main loop），这是正常启动顺序的一部分。如果使用 Xcode 模板项目创建应用程序，则永远不必显式调用这些例程。

&emsp;对于子线程，你需要确定是否需要 run loop，如果需要，请自己配置并启动它。你不需要在所有情况下都启动线程的 run loop。例如，如果你使用一个线程来执行一些长期运行的预定任务，你可能可以避免启动 run loop。run loop 用于希望与线程进行更多交互的情况。例如，如果计划执行以下任一操作，则需要启动 run loop：
+ 使用端口（ports）或自定义输入源（custom input sources）与其他线程通信。
+ 在线程上使用 timer。
+ 在 Cocoa 应用程序中使用任何 `performSelector…` 方法。
+ 保持线程执行定期任务（periodic tasks）。

&emsp;如果选择使用 run loop，则配置和设置很简单。不过，与所有线程编程一样，你应该有一个在适当情况下退出子线程的计划。通过让线程退出而干净地结束它总是比强制它终止要好。有关如何配置和退出运行循环的信息，请参考 Using Run Loop Objects。
## Using Run Loop Objects
&emsp;运行循环对象提供了用于将输入源，计时器和运行循环观察器添加到您的运行循环然后运行它的主界面。每个线程都有一个与之关联的运行循环对象。在可可中，此对象是NSRunLoop类的实例。在低级应用程序中，它是指向CFRunLoopRef不透明类型的指针。

&emsp;Run loop 对象提供主接口，用于向 run loop 添加 input sources、timers 和 run loop observers，然后运行它。每个线程都有一个与之关联的 run loop 对象。在 Cocoa 中，此对象是 NSRunLoop 类的实例。在低级应用程序中（low-level application），它是指向 CFRunLoopRef 不透明类型的指针。
### Getting a Run Loop Object
&emsp;要获取当前线程的 run loop，请使用以下方法之一：
+ 在 Cocoa 应用程序中，使用 NSRunLoop 的 currentRunLoop 类方法检索 NSRunLoop 对象。
+ 使用 `CFRunLoopGetCurrent` 函数。

&emsp;尽管它们不是免费的桥接类型（not toll-free bridged types），但你可以在需要时从 NSRunLoop 对象获取 CFRunLoopRef 不透明类型。NSRunLoop 类定义一个 `getCFRunLoop` 方法，该方法返回可以传递给 Core Foundation 例程的 CFRunLoopRef 类型。因为这两个对象引用同一个 run loop，因此可以根据需要混合调用 NSRunLoop 对象和 CFRunLoopRef 不透明类型。
### Configuring the Run Loop
&emsp;在子线程上运行 run loop 之前，必须至少向其添加一个 input source 或 timer。如果 run loop 没有任何要监视的 source，则当你尝试运行它时，它会立即退出。有关如何向 run loop 添加 sources 的示例，请参考 Configuring Run Loop Sources。

&emsp;除了安装 sources 之外，还可以安装 run loop observers，并使用它们来检测 run loop 的不同执行阶段。要安装 run loop observers，需要创建一个 CFRunLoopObserverRef 不透明类型，并使用 `CFRunLoopAddObserver` 函数将其添加到 run loop 中。Run loop observers 必须使用 Core Foundation 创建，即使对于 Cocoa 应用程序也是如此。

&emsp;清单 3-1显示了一个线程的主例程，该线程将一个 run loop observer 附加到它的 run loop 上。该示例的目的是向你展示如何创建 run loop observer，因此代码只需设置一个 run loop observer 来监视所有 run loop 活动。基本处理程序例程（basic handler routine）（未显示）只是在处理 timer 请求时记录 run loop 活动。

&emsp;清单 3-1 创建一个 run loop observer
```c++

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
