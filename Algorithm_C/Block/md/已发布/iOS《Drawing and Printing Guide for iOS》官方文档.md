# iOS《Drawing and Printing Guide for iOS》官方文档

## About Drawing and Printing in iOS
&emsp;本文档涉及三个相关主题：
+ 绘制自定义 UI 视图。自定义 UI 视图允许你绘制无法使用标准 UI 元素轻松绘制的内容。例如，绘图程序可能使用自定义视图来绘制用户的绘图，或者街机游戏可能使用自定义视图来绘制精灵（sprites）。
+ 绘制到 offscreen 位图和 PDF 内容。无论你是计划稍后显示图像、将它们导出到文件、还是将图像打印到启用 AirPrint 的打印机，offscreen 绘图都允许你在不中断用户的工作流的情况下显示图像。
+ 将 AirPrint 支持添加到应用。iOS 打印系统允许你以不同的方式绘制内容以适合 page。

&emsp;Figure I-1  You can combine custom views with standard views, and even draw things offscreen.（你可以将自定义视图与标准视图相结合，甚至可以将内容从 offscreen 绘制。）

![UI_overlay](https://p1-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/a9e70c6802d04c458b0bb4480d026670~tplv-k3u1fbpfcp-watermark.image)

### At a Glance
&emsp;iOS 原生图形系统结合了三种主要技术：UIKit、Core Graphics 和 Core Animation。UIKit 在这些视图中提供了视图和一些高级绘图功能，Core Graphics 在 UIKit 视图中提供了附加（低级）绘图支持，Core Animation 提供了将 transformations 和动画应用于 UIKit 视图的能力。Core Animation 还负责视图合成（view compositing）。

#### Custom UI Views Allow Greater Drawing Flexibility（自定义 UI 视图允许更大的绘图灵活性）
&emsp;本文档描述如何使用原生绘图技术绘制到自定义 UI 视图中。这些技术包括 Core Graphics 和 UIKit frameworks，支持 2D 绘图。

&emsp;在考虑使用自定义 UI 视图之前，应该确定确实需要这样做。原生绘图适用于处理更复杂的二维布局需求。但是，由于自定义视图是处理器密集型（processor-intensive）视图，因此应限制使用原生绘图技术进行的绘图量。

&emsp;作为自定义绘图的替代方法，iOS 应用程序可以用其他几种方式在屏幕上绘图。

+ Using standard (built-in) views（使用标准（内置）视图）。标准视图允许你绘制公共用户界面原语，包括列表、集合、警报、图像、进度条、表等，而无需自己显式绘制任何内容。使用内置视图不仅可以确保 iOS 应用程序之间的一致用户体验，还可以节省编程工作量。如果内置视图满足你的需要，你应该阅读 [View Programming Guide for iOS](https://developer.apple.com/library/archive/documentation/WindowsViews/Conceptual/ViewPG_iPhoneOS/Introduction/Introduction.html#//apple_ref/doc/uid/TP40009503)。
+ Using Core Animation layers（使用 Core Animation 提供的 layers）。Core Animation 使你可以使用 animation 和 transformations 创建复杂的分层二维视图。Core Animation 是一个很好的选择，用于设置标准视图的动画，或以复杂的方式组合视图以呈现深度错觉，并且可以与本文档中描述的自定义绘制视图相结合。要了解有关Core Animation 的更多信息，请阅读 Core Animation Overview。
+ Using OpenGL ES in a GLKit view or a custom view（在 GLKit 视图或自定义视图中使用 OpenGL ES）。OpenGL ES 框架提供了一组开放的标准图形库，主要面向需要高帧率的游戏开发或应用程序，如虚拟原型应用程序、机械和建筑设计应用程序。它符合 OpenGL ES 2.0 和 OpenGL ES v1.1 规范。要了解有关 OpenGL 绘图的更多信息，请阅读 [OpenGL ES Programming Guide](https://developer.apple.com/library/archive/documentation/3DDrawing/Conceptual/OpenGLES_ProgrammingGuide/Introduction/Introduction.html#//apple_ref/doc/uid/TP40008793)。
+ Using web content（使用 web 内容）。UIWebView 类允许你在 iOS 应用程序中显示基于 web 的用户界面。要了解有关在 web 视图中显示 web 内容的更多信息，请阅读 [Using UIWebView to display select document types](https://developer.apple.com/library/archive/qa/qa1630/_index.html#//apple_ref/doc/uid/DTS40008749)  和 [UIWebView Class Reference](https://developer.apple.com/documentation/uikit/uiwebview?language=objc)。

&emsp;根据要创建的应用程序的类型，可以很少使用或不使用自定义绘图代码。尽管沉浸（immersive）式应用程序通常广泛使用自定义绘图代码，但实用程序和生产力应用程序通常可以使用标准视图和控件来显示其内容。

&emsp;自定义绘图代码的使用应限于显示的内容需要动态更改的情况。例如，绘图应用程序通常需要使用自定义绘图代码来跟踪用户的绘图命令，而街机风格的游戏可能需要不断更新屏幕以反映不断变化的游戏环境。在这些情况下，应选择适当的绘图技术并创建自定义视图类来处理事件并适当地更新显示。

&emsp;另一方面，如果应用程序的大部分界面是固定的，则可以提前将该界面渲染为一个或多个图像文件，并在运行时使用 UIImageView 类显示这些图像。可以根据需要将图像视图与其他内容分层以构建界面。你还可以使用 UILabel 类来显示可配置的文本，并包含按钮或其他控件以提供交互性。例如，电子版的棋盘游戏通常可以创建很少或没有自定义绘图代码。

&emsp;因为自定义视图通常更为处理器密集型（GPU 的帮助更少），如果你可以使用标准视图执行所需的操作，那么你应该始终这样做。另外，你应该使你的自定义视图尽可能小，只包含你不能以任何其他方式绘制的内容，其他所有内容都使用标准视图。如果需要将标准 UI 元素与自定义绘图相结合，请考虑使用 Core Animation  layer 将自定义视图与标准视图叠加，以便尽可能少地进行绘图。

##### A Few Key Concepts Underpin Drawing With the Native Technologies（一些关键概念支持使用原生技术进行绘图）
&emsp;使用 UIKit 和 Core Graphics 绘制内容时，除了视图绘制周期（view drawing cycle）外，还应该熟悉一些概念。

+ 对于 `drawRect:` 方法，UIKit 创建一个用于渲染到 display 的图形上下文（graphics context）。此图形上下文包含图形系统执行绘图命令所需的信息，包括填充（fill）和笔划颜色（stroke color）、字体（font）、剪裁区域（clipping area）和线宽（line width）等属性。你还可以为位图图像和 PDF 内容创建和绘制自定义图形上下文。
+ UIKit 有一个默认坐标系，其中图形原点位于视图的左上角；正值向下延伸到该原点的右侧。通过修改当前变换矩阵（transformation matrix）（将视图的坐标空间映射到设备屏幕），可以更改默认坐标系相对于 underlying view 或 window 的大小、方向和位置。
+ 在 iOS 中，以点为单位度量距离的逻辑坐标空间不等于以像素为单位度量的设备坐标空间。为了获得更高的精度，点用浮点值表示。

&emsp;Relevant Chapter: iOS Drawing Concepts

##### UIKit, Core Graphics, and Core Animation Give Your App Many Tools For Drawing（为绘图提供许多工具）
&emsp;UIKit 和 Core Graphics 具有许多互补的图形功能，包括图形上下文（graphics contexts）、Bézier路径、图像、位图（bitmaps）、透明层（transparency layers）、颜色、字体、PDF 内容以及绘图矩形和剪切区域。此外，Core Graphics 还具有与线属性、颜色空间、图案颜色、渐变、阴影和图像遮罩相关的功能。Core Animation 框架使你可以通过操纵和显示使用其他技术创建的内容来创建流畅的动画。

&emsp;Relevant Chapters: iOS Drawing Concepts, Drawing Shapes Using Bézier Paths, Drawing and Creating Images, Generating PDF Content

#### Apps Can Draw Into Offscreen Bitmaps or PDFs（应用程序可以绘制离屏位图或 PDF）
&emsp;应用程序在屏幕外（offscreen）绘制内容通常很有用：

+ 当缩小照片以供上传、将内容渲染到图像文件以供存储或使用 Core Graphics 生成复杂图像以供显示时，通常使用屏幕外位图上下文（Offscreen bitmap contexts）。
+ 为打印目的绘制用户生成的内容时，通常使用屏幕外（Offscreen）的 PDF 上下文。

&emsp;创建屏幕外上下文（offscreen context）后，可以像在自定义视图的 `drawRect:` 方法中绘制一样将其绘制到其中。

&emsp;Relevant Chapters: Drawing and Creating Images, Generating PDF Content

#### Apps Have a Range of Options for Printing Content（应用程序具有一系列用于打印内容的选项）
&emsp;从 iOS 4.2 开始，应用程序可以使用 AirPrint 将内容无线打印到支持的打印机上。组装打印作业时，它们有三种方式为 UIKit 提供要打印的内容：

+ 它们可以为框架提供一个或多个可直接打印的对象；这样的对象只需要最少的应用程序参与。这些是包含或引用图像数据或 PDF 内容的 NSData、NSURL、UIImage 或 ALAsset 类的实例。
+ 他们可以为打印作业指定一个打印格式化程序。打印格式化程序是一个对象，它可以将特定类型的内容（如纯文本或 HTML）放置在多个页面上。
+ 他们可以为打印作业指定页面呈现器。页面呈现程序通常是 UIPrintPageRenderer 的自定义子类的实例，它绘制要部分或全部打印的内容。页面呈现程序可以使用一个或多个打印格式化程序来帮助它绘制和格式化其可打印内容。

&emsp;Relevant Chapter: Printing

#### It’s Easy to Update Your App for High-Resolution Screens（为高分辨率屏幕更新你的应用程序很容易）
&emsp;一些 iOS 设备具有高分辨率屏幕，因此你的应用程序必须准备好在这些设备和具有低分辨率屏幕的设备上运行。iOS 处理了处理不同分辨率所需的大部分工作，但你的应用程序必须完成其余的工作。你的任务包括提供特别命名的高分辨率图像，并修改与层和图像相关的代码以考虑当前的比例因子。

&emsp;Relevant Appendix: Supporting High-Resolution Screens In Views

### See Also
&emsp;有关完整的打印示例，请参阅：[PrintPhoto: Using the Printing API with Photos](https://developer.apple.com/library/archive/samplecode/PrintPhoto/Introduction/Intro.html#//apple_ref/doc/uid/DTS40010366)、[Sample Print Page Renderer](https://developer.apple.com/library/archive/samplecode/PrintPhoto/Introduction/Intro.html#//apple_ref/doc/uid/DTS40010366) 和 [UIKit Printing with UIPrintInteractionController and UIViewPrintFormatter](https://developer.apple.com/library/archive/samplecode/PrintWebView/Introduction/Intro.html#//apple_ref/doc/uid/DTS40010311) 示例代码。

## iOS Drawing Concepts（iOS 绘图概念）
&emsp;高质量的图形是应用程序用户界面的重要组成部分。提供高质量的图形不仅使你的应用程序看起来很好，而且使你的应用程序看起来像是系统其他部分的自然扩展。iOS 提供了两种在系统中创建高质量图形的主要路径：OpenGL 或使用 Quartz、Core Animation 和 UIKit 进行原生渲染。本文档描述原生渲染。（要了解 OpenGL 绘图，请参见 [OpenGL ES Programming Guide](https://developer.apple.com/library/archive/documentation/3DDrawing/Conceptual/OpenGLES_ProgrammingGuide/Introduction/Introduction.html#//apple_ref/doc/uid/TP40008793)。）

&emsp;Quartz 是主要的绘图接口，支持基于路径的绘图（path-based drawing）、抗锯齿渲染（anti-aliased rendering）、渐变填充模式（gradient fill patterns）、图像、颜色、坐标空间转换（coordinate-space transformations）以及 PDF 文档的创建、显示和解析。UIKit 为线条艺术（line art）、石英图像（Quartz images）和颜色处理（color manipulations）提供 Objective-C 包装器。 Core Animation 为许多 UIKit 视图属性中的动画更改提供底层支持，还可以用于实现自定义动画。

&emsp;本章概述 iOS 应用程序的绘图过程，以及每种受支持绘图技术的特定绘图技术。你还将找到有关如何优化 iOS 平台绘图代码的提示和指导。

> Important: 并非所有 UIKit 类都是线程安全的。在应用程序主线程以外的线程上执行与绘图相关的操作之前，请务必检查文档。

### The UIKit Graphics System（UIKit 绘图系统）
&emsp;在 iOS 中，所有到屏幕的绘图，不管它是否涉及 OpenGL、Quartz、UIKit 或 Core Animation，都发生在 UIView 类或其子类的实例范围内。视图定义了发生绘图的屏幕部分。如果使用系统提供的视图，则会自动处理此绘图。但是，如果定义自定义视图，则必须自己提供图形代码。如果使用 Quartz、Core Animation 和 UIKit 进行绘制，则使用以下各节中描述的绘制概念。

&emsp;除了直接绘制到屏幕之外，UIKit 还允许你绘制到 offscreen bitmap 和 PDF 图形上下文中。在 offscreen 上下文中绘制时，你不是在视图中绘制，这意味着诸如视图绘制周期等概念不适用（除非你随后获得该图像并在图像视图或类似视图中绘制）。

#### The View Drawing Cycle（View 绘图周期）
&emsp;UIView 类的子类的基本绘图模型（basic drawing model）涉及按需更新内容。UIView 类使更新过程更简单、更高效；但是，通过收集你提出的更新请求，并在最合适的时间将它们传递到绘图代码。

&emsp;当视图第一次显示或视图的一部分需要重新绘制时，iOS 会通过调用视图的 `drawRect:` 方法来请求视图绘制其内容。

&emsp;有几个操作可以触发视图更新：

+ 移动或删除部分遮挡视图的另一个视图
+ 通过将以前隐藏的视图的 hidden 属性设置为 NO，使其再次可见
+ 滚动屏幕外的视图，然后返回到屏幕上
+ 显式调用视图的 `setNeedsDisplay` 或 `setNeedsDisplayInRect:` 方法

&emsp;系统视图将自动重绘。对于自定义视图，必须重写 `drawRect:` 方法并在其中执行所有绘图。在 `drawRect:` 方法中，使用原生绘图技术绘制形状（shapes）、文本（text）、图像（images）、渐变（gradients）或任何其他所需的视觉内容。当视图第一次可见时，iOS 会将一个矩形传递给视图的 `drawRect:` 方法，该方法包含视图的整个可见区域。在随后的调用中，矩形只包含实际需要重绘的视图部分。为了获得最佳性能，你应该只重新绘制受影响的内容。

&emsp;调用 `drawRect:` 方法后，视图将自身标记为已更新（标记为需要更新），并等待新操作到达并触发另一个更新周期。如果你的视图显示静态内容，那么你所需要做的就是响应由于滚动和其他视图的存在而导致的视图可见性的更改。

&emsp;但是，如果要更改视图的内容，则必须告诉视图重新绘制其内容。为此，请调用 `setNeedsDisplay` 或 `setNeedsDisplayInRect:` 方法来触发更新。例如，如果一秒钟要更新几次内容，可能需要设置一个计时器来更新视图。你还可以更新视图以响应用户交互或视图中新内容的创建。

> Important: 不要自己调用视图的 `drawRect:` 方法。只有在屏幕重新绘制期间，iOS 中内置的代码才应该调用该方法。在其他时候，不存在图形上下文，因此无法绘制。（下一节将解释图形上下文。） 

#### Coordinate Systems and Drawing in iOS（iOS 中的坐标系和绘图）
&emsp;当一个应用程序在 iOS 中绘制一些东西时，它必须在一个由坐标系定义的二维空间中定位绘制的内容。这个概念乍一看似乎很简单，但事实并非如此。iOS 中的应用程序有时在绘图时必须处理不同的坐标系。

&emsp;在 iOS 中，所有绘图都发生在图形上下文中。从概念上讲，图形上下文是一个对象，描述绘图的位置和方式，包括基本绘图属性，例如绘图时要使用的颜色（colors）、剪裁区域（clipping area）、线宽（line width）和样式信息（style information）、字体信息（font information）、合成选项（compositing options）等。

&emsp;此外，如图 1-1 所示，每个图形上下文都有一个坐标系。更准确地说，每个图形上下文有三个坐标系：

+ 绘图（用户）坐标系。发出绘图命令时将使用此坐标系。
+ 视图坐标系（基础空间）。该坐标系是相对于视图的固定坐标系。
+ （物理）设备坐标系。此坐标系表示物理屏幕上的像素。

&emsp;Figure 1-1  The relationship between drawing coordinates, view coordinates, and hardware coordinates（绘图坐标、视图坐标和硬件坐标之间的关系）

![coordinate_differences](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/f3d89122be6d41718c6dec4ed24af288~tplv-k3u1fbpfcp-watermark.image)

&emsp;iOS 的绘图框架创建用于绘制特定目的地（屏幕、位图、PDF 内容等）的图形上下文，这些图形上下文为该目的地建立初始绘图坐标系。此初始绘图坐标系称为默认坐标系（default coordinate system），是视图基础坐标系的 1:1 映射。

&emsp;每个视图还有一个当前变换矩阵（CTM），一个将当前绘图坐标系中的点映射到（固定）视图坐标系的数学矩阵。应用程序可以修改此矩阵（如后面所述），以更改未来绘图操作的行为。

&emsp;iOS 的每个绘图框架基于当前图形上下文建立一个默认坐标系。在 iOS 中，有两种主要类型的坐标系：

+ 一种左上角原点坐标系（ULO），其中绘图操作的原点位于绘图区域的左上角，正值向下和向右延伸。UIKit 和 Core Animation 框架使用的默认坐标系是基于 ULO 的。
+ 一种左下原点坐标系（LLO），其中绘图操作的原点位于绘图区域的左下角，正值向上和向右延伸。Core Graphics 框架使用的默认坐标系是基于 LLO 的。

&emsp;These coordinate systems are shown in Figure 1-2.

&emsp;Figure 1-2  Default coordinate systems in iOS（iOS 中的默认坐标系）

![flipped_coordinates-2](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/94793bd596954b66be04c18580b230d1~tplv-k3u1fbpfcp-watermark.image)

> Note: OS X 中的默认坐标系是基于 LLO 的。尽管  Core Graphics 和 AppKit 框架的绘图功能和方法非常适合此默认坐标系，但 AppKit 为翻转绘图坐标系使其具有左上角原点提供了编程支持。

&emsp;在调用视图的 `drawRect:` 方法之前，UIKit 通过使图形上下文（graphics context）可用于绘图操作来建立默认坐标系，以便在屏幕上绘图。在视图的 `drawRect:` 方法中，应用程序可以设置图形状态参数（graphics-state parameters）（例如填充颜色）并绘制到当前图形上下文，而无需显式引用图形上下文。此隐式图形上下文建立 ULO 默认坐标系。

#### Points Versus Pixels（点与像素）
&emsp;在 iOS 中，在绘图代码中指定的坐标与基础设备的像素之间存在区别。使用原生绘图技术（如 Quartz、UIKit 和 Core Animation）时，绘图坐标空间和视图的坐标空间都是逻辑坐标空间（logical coordinate spaces），距离以点（points）为单位。这些逻辑坐标系与系统框架用于管理屏幕上像素的设备坐标空间（device coordinate space）分离。

&emsp;系统会自动将视图坐标空间中的点映射到设备坐标空间中的像素，但这种映射并不总是一对一的。这种行为导致一个重要的事实，你应该永远记住：

&emsp;**一个点（point）不一定对应于一个物理像素（physical pixel）。**

&emsp;使用点（points）（和逻辑坐标系（logical coordinate system））的目的是提供与设备无关的一致大小的输出。在大多数情况下，点的实际大小无关紧要。点的目标是提供一个相对一致的比例，你可以在代码中使用它来指定视图和渲染内容的大小和位置。点如何实际映射到像素是系统框架处理的一个细节。例如，在具有高分辨率屏幕的设备上，一点宽的线实际上可能导致两个物理像素宽的线。结果是，如果在两个类似的设备上绘制相同的内容，其中只有一个具有高分辨率屏幕，则两个设备上的内容大小似乎大致相同。

> Note: 在 PDF 渲染和打印环境中，Core Graphics 使用 1 point 到 1/72 inch 的行业标准映射来定义 "point"。

&emsp;在 iOS 中，UIScreen、UIView、UIImage 和 CALayer 类提供属性来获取（在某些情况下，还可以设置）描述特定对象的点和像素之间关系的比例因子。例如，每个 UIKit 视图都有一个 contentScaleFactor 属性。在标准分辨率屏幕上，比例因子通常为 1.0。在高分辨率屏幕上，比例因子通常为 2.0。在未来，其他规模因素也可能出现。（在 iOS 4 之前的 iOS 中，应假定比例因子为 1.0。）

&emsp;原生绘图技术（如 Core Graphics）会为你考虑当前的比例因子。例如，如果其中一个视图实现了 `drawRect:` 方法，UIKit 会自动将该视图的比例因子设置为屏幕的比例因子。此外，UIKit 会自动修改绘图期间使用的任何图形上下文的当前变换矩阵，以考虑视图的比例因子。因此，在 `drawRect:` 方法中绘制的任何内容都会根据底层设备的屏幕进行适当的缩放。

&emsp;由于这种自动映射，在编写绘图代码时，像素通常无关紧要。但是，有时你可能需要根据点如何映射到像素来更改应用程序的绘图行为，以便在具有高分辨率屏幕的设备上绘制更高分辨率的图像，或者避免在低分辨率屏幕上绘图时出现缩放瑕疵。

&emsp;在 iOS 中，当你在屏幕上绘制东西时，图形子系统（graphics subsystem）使用一种称为抗锯齿（antialiasing）的技术在低分辨率屏幕上近似显示高分辨率图像。解释此技术的最佳方法是通过示例：在纯白背景上绘制黑色垂直线时，如果该线正好落在一个像素上，则该线将显示为白色区域中的一系列黑色像素。但是，如果它正好出现在两个像素之间，则它会并排出现为两个灰色像素，如图 1-3 所示。

&emsp;Figure 1-3  A one-point line centered at a whole-numbered point value（一条以整数点值为中心的单点线）

![pixel_alignment](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/333745f82d4845428660498d1407caff~tplv-k3u1fbpfcp-watermark.image)

&emsp;由整数点定义的位置位于像素之间的中点。例如，如果在（1.0，1.0）到（1.0，10.0）之间绘制一条一像素宽的垂直线，则会得到一条模糊的灰色线。如果你画一条两像素宽的线，你会得到一条实心的黑线，因为它完全覆盖了两个像素（一个在指定点的两边）。通常，奇数个物理像素宽的线看起来比偶数个物理像素宽的线更柔和，除非调整它们的位置使它们完全覆盖像素。

&emsp;当确定一条一点宽的线（one-point-wide line）覆盖了多少像素时，比例因子（scale factor）起作用。

&emsp;在低分辨率显示器（比例因子为 1.0）上，一点宽的线是一个像素宽。为避免绘制一点宽的水平线或垂直线时出现抗锯齿（antialiasing），如果该线的宽度为奇数像素，则必须将该位置偏移0.5 个点到整个编号位置的任一侧。如果线条的宽度是偶数点，要避免线条模糊，千万不要这样做。

&emsp;Figure 1-4  Appearance of one-point-wide lines on standard and retina displays（标准显示器和视网膜显示器上出现的一点宽线（one-point-wide））

![regular_vs_retina](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/8fdf3990a0e541a5b4a9b25375713096~tplv-k3u1fbpfcp-watermark.image)

&emsp;在高分辨率显示器（比例因子为 2.0）上，一点宽的线根本不抗锯齿，因为它占用两个完整像素（从 -0.5 到 +0.5）。要绘制一条仅覆盖单个物理像素的线，需要使其厚度为 0.5 点，并将其位置偏移 0.25 点。两种屏幕之间的比较如图 1-4 所示。

&emsp;当然，基于比例因子更改绘图特征可能会产生意想不到的后果。在某些设备上，1 像素宽的线条可能看起来不错，但在高分辨率设备上，线条可能太细，很难看清。是否做出这样的改变由你来决定。

#### Obtaining Graphics Contexts（获取图形上下文）
&emsp;大多数情况下，图形上下文（graphics contexts）都是为你配置的。每个视图对象都会自动创建一个图形上下文，以便在调用自定义 `drawRect:` 方法后，代码可以立即开始绘制。作为此配置的一部分，基础 UIView 类为当前绘图环境创建图形上下文（一个不透明的类型：CGContextRef）。

&emsp;如果要在视图以外的位置绘制（例如，捕获 PDF 或位图文件中的一系列绘图操作），或者需要调用需要上下文对象的 Core Graphics 函数，则必须执行其他步骤以获取图形上下文对象。下面的各节进行解释。

&emsp;有关图形上下文（graphics contexts）、修改图形状态信息以及使用图形上下文（graphics contexts）创建自定义内容的更多信息，请参见 [Quartz 2D Programming Guide](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/drawingwithquartz2d/Introduction/Introduction.html#//apple_ref/doc/uid/TP30001066)。有关与图形上下文一起使用的函数的列表，请参见 [CGContext Reference](https://developer.apple.com/documentation/coregraphics/cgcontextref?language=objc)、[CGBitmapContext Reference](https://developer.apple.com/documentation/coregraphics/cgbitmapcontext) 和 [CGPDFContext Reference](https://developer.apple.com/documentation/coregraphics/cgpdfcontext)。

##### Drawing to the Screen（绘制到屏幕）
&emsp;如果在 `drawRect:` 方法或其他地方使用 Core Graphics 函数来绘制视图，则需要用于绘制的图形上下文。（其中许多函数的第一个参数必须是 CGContextRef 对象。）你可以调用函数 `UIGraphicsGetCurrentContext` 来获取在 `drawRect:` 中隐式创建的同一图形上下文的显式版本。因为它是相同的图形上下文，所以绘图函数也应该引用 ULO 默认坐标系。

&emsp;如果要使用 Core Graphics 函数在 UIKit 视图中绘制，则应使用 UIKit 的 ULO 坐标系进行绘制操作。或者，可以对 CTM 应用翻转变换，然后使用 Core Graphics 本地坐标系在 UIKit 视图中绘制对象。Flipping the Default Coordinate System 详细讨论了翻转变换。

&emsp;`UIGraphicsGetCurrentContext` 函数始终返回当前有效的图形上下文。例如，如果你创建一个 PDF 上下文，然后调用 `UIGraphicsGetCurrentContext`，你将收到该 PDF 上下文。如果使用 Core Graphics 函数绘制视图，则必须使用 `UIGraphicsGetCurrentContext` 返回的图形上下文。

> Note: UIPrintPageRenderer 类声明了几个用于绘制可打印内容的方法。以类似于 `drawRect:` 的方式，UIKit 为这些方法的实现安装隐式图形上下文。此图形上下文建立 ULO 默认坐标系。

##### Drawing to Bitmap Contexts and PDF Contexts（绘图到位图上下文和 PDF 上下文）
&emsp;UIKit 提供了在位图图形上下文中呈现图像以及通过在 PDF 图形上下文中绘制来生成 PDF 内容的函数。这两种方法都要求你首先调用一个函数来创建图形上下文，即位图上下文或 PDF 上下文。返回的对象用作后续绘图和状态设置调用的当前（隐式）图形上下文。在上下文中完成绘制后，可以调用另一个函数来关闭上下文。

&emsp;UIKit 提供的位图上下文和 PDF 上下文都建立了 ULO 默认坐标系。Core Graphics 具有在位图图形上下文中渲染和在 PDF 图形上下文中绘制的相应功能。然而，应用程序通过 Core Graphics 直接创建的上下文建立了一个默认坐标系。

> Note: 在 iOS 中，建议使用 UIKit 函数绘制位图上下文和 PDF 上下文。但是，如果确实使用 Core Graphics 替代方案并打算显示渲染结果，则必须调整代码以补偿默认坐标系中的差异。

#### Color and Color Spaces（颜色和颜色空间）
&emsp;iOS 支持 Quartz 提供的所有颜色空间；但是，大多数应用程序应该只需要 RGB 颜色空间。因为 iOS 是在嵌入式硬件上运行的，在屏幕上显示图形，所以 RGB 颜色空间是最合适的。

&emsp;UIColor 对象提供了使用 RGB、HSB 和灰度（grayscale）值指定颜色值的便利方法。以这种方式创建颜色时，不需要指定颜色空间。它由 UIColor 对象自动确定。

&emsp;你还可以使用 Core Graphics 框架中的 `CGContextSetRGBStrokeColor` 和 `CGContextSetRGBFillColor` 函数来创建和设置颜色。尽管 Core Graphics 框架支持使用其他颜色空间创建颜色，以及创建自定义颜色空间，但不建议在绘图代码中使用这些颜色。绘图代码应始终使用 RGB 颜色。

### Drawing with Quartz and UIKit（使用 Quartz 和 UIKit 绘图）
&emsp;Quartz 是 iOS 中原生绘图技术的总称。Core Graphics 框架位于 Quartz 的核心，是用于绘制内容的主要接口。此框架提供用于操作以下内容的数据类型和函数：

+ Graphics contexts
+ Paths
+ Images and bitmaps
+ Transparency layers
+ Colors, pattern colors, and color spaces
+ Gradients and shadings
+ Fonts
+ PDF content

&emsp;UIKit 以 Quartz 的 basic features 为基础，为图形相关操作（graphics-related operations）提供了一组集中的类。UIKit 图形类并不打算作为一套全面的绘图工具，Core Graphics 已经提供了这一点。相反，它们为其他 UIKit 类提供绘图支持。UIKit 支持包括以下类和函数：

+ UIImage，实现了用于显示图像的不可变类
+ UIColor，为设备颜色提供基本支持
+ UIFont，它为需要它的类提供字体信息
+ UIScreen，提供有关屏幕的基本信息
+ UIBezierPath，使你的应用可以绘制直线、圆弧、椭圆形和其他形状。
+ 用于生成 UIImage 对象的 JPEG 或 PNG representation 的函数
+ 用于绘制到位图图形上下文的函数
+ 通过绘制到 PDF 图形上下文生成 PDF 数据的功能
+ 绘制矩形和剪切绘图区域的功能
+ 更改和获取当前图形上下文的功能

&emsp;有关组成 UIKit 的类和方法的信息，请参见 [UIKit Framework Reference](https://developer.apple.com/documentation/uikit?language=objc)。有关构成 Core Graphics 框架的不透明类型和函数的更多信息，请参见 Core Graphics Framework Reference。

#### Configuring the Graphics Context（配置图形上下文）
&emsp;在调用 `drawRect:` 方法之前，视图对象创建一个图形上下文并将其设置为当前上下文。此上下文仅在 `drawRect:` 调用的生存期内存在。你可以通过调用 `UIGraphicsGetCurrentContext` 函数来检索指向此图形上下文的指针。此函数返回对 CGContextRef 类型的引用，将其传递给 Core Graphics 函数以修改当前图形状态。表 1-1 列出了用于设置图形状态不同方面的主要函数。有关函数的完整列表，请参见 CGContext Reference。此表还列出了存在的 UIKit 替代方案。

&emsp;Table 1-1  Core graphics functions for modifying graphics state（用于修改图形状态的 Core Graphics 功能）

| Graphics state | Core Graphics functions | UIKit alternatives |
| --- | --- | --- |
| Current transformation matrix (CTM) | CGContextRotateCTM CGContextScaleCTM CGContextTranslateCTM CGContextConcatCTM | None |
| Clipping area | CGContextClipToRect | UIRectClip function |
| Line: Width, join, cap, dash, miter limit | CGContextSetLineWidth CGContextSetLineJoin CGContextSetLineCap CGContextSetLineDash CGContextSetMiterLimit | None |
| Accuracy of curve estimation（曲线估计的准确性） | CGContextSetFlatness | None |
| Anti-aliasing setting（抗锯齿设置） | CGContextSetAllowsAntialiasing | None |
| Color: Fill and stroke settings | CGContextSetRGBFillColor CGContextSetRGBStrokeColor | UIColor class |
| Alpha global value (transparency) | CGContextSetAlpha | None |
| Rendering intent | CGContextSetRenderingIntent | None |
| Color space: Fill and stroke settings | CGContextSetFillColorSpace CGContextSetStrokeColorSpace | UIColor class |
| Text: Font, font size, character spacing, text drawing mode | CGContextSetFont CGContextSetFontSize CGContextSetCharacterSpacing | UIFont class |
| Blend mode | CGContextSetBlendMode | The UIImage class and various drawing functions let you specify which blend mode to use. UIImage 类和各种绘图功能可让你指定要使用的混合模式。|

&emsp;图形上下文（graphics context）保存图形状态（graphics states）在堆栈中。当 Quartz 创建图形上下文时，堆栈为空。使用 `CGContextSaveGState` 函数将当前图形状态的副本推入到堆栈上。此后，对图形状态所做的修改会影响后续的绘图操作，但不会影响存储在堆栈上的副本。完成修改后，可以使用 `CGContextRestoreGState` 函数从堆栈顶部弹出保存的状态，从而返回到以前的图形状态。以这种方式推入和弹出图形状态是返回到以前状态的一种快速方法，并且无需逐个撤消每个状态更改。这也是将某些状态（如剪裁路径）恢复到其原始设置的唯一方法。

&emsp;有关图形上下文以及使用它们配置绘图环境的一般信息，请参见 [Quartz 2D Programming Guide](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/drawingwithquartz2d/Introduction/Introduction.html#//apple_ref/doc/uid/TP30001066) 中的 [Graphics Contexts](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/drawingwithquartz2d/dq_context/dq_context.html#//apple_ref/doc/uid/TP30001066-CH203)。

#### Creating and Drawing Paths（创建和绘制路径）
&emsp;路径是由一系列直线和 Bézier 曲线创建的基于向量的形状。UIKit 包括 UIRectFrame 和 UIRectFill 函数（除其他函数外），用于在视图中绘制矩形等简单路径。Core Graphics 还包括创建简单路径（如矩形和椭圆）的便利功能。

&emsp;对于更复杂的路径，必须使用 UIKit 的 UIBezierPath 类，或者使用在 Core Graphics 框架中的 CGPathRef 这个不透明类型上操作的函数，自己创建路径。尽管可以使用任何一个 API 构造没有图形上下文的路径，但路径中的点仍然必须引用当前坐标系（该坐标系具有 ULO 或 LLO 方向），并且仍然需要图形上下文来实际渲染路径。

&emsp;绘制路径时，必须设置当前上下文。此上下文可以是自定义视图的上下文（在 `drawRect:`、位图上下文或 PDF 上下文中）。坐标系确定如何渲染路径。UIBezierPath 采用 ULO 坐标系。因此，如果你的视图被翻转（使用 LLO 坐标），则生成的形状可能会以不同于预期的方式渲染。为获得最佳结果，应始终指定相对于用于渲染的图形上下文的当前坐标系原点的点。

> Note: 圆弧是需要额外工作的路径的一个方面，即使遵循此 "rule"。如果使用在 ULO 坐标系中定位点的 Core Graphic 函数创建路径，然后在 UIKit 视图中渲染路径，则弧 "points" 的方向不同。有关此主题的详细信息，请参见 Side Effects of Drawing with Different Coordinate Systems。

&emsp;对于在 iOS 中创建路径，建议你使用 UIBezierPath 而不是 CGPath 函数，除非你需要一些只有 Core Graphics 提供的功能，例如向路径添加省略号。有关在 UIKit 中创建和渲染路径的详细信息，请参见 Drawing Shapes Using Bézier Paths。

&emsp;有关使用 UIBezierPath 绘制路径的信息，请参见 Drawing Shapes Using Bézier Paths。有关如何使用 Core Graphics 绘制路径的信息，包括有关如何为复杂路径元素指定点的信息，请参见 Quartz 2D Programming Guide 中的 Paths。有关用于创建路径的函数的信息，请参见 CGContext Reference 和 CGPath Reference。

#### Creating Patterns, Gradients, and Shadings（创建图案、渐变和着色）
&emsp;Core Graphics 框架包括用于创建图案、渐变和着色的附加功能。你可以使用这些类型创建非单色颜色，并使用它们填充你创建的路径。Patterns 是从重复的图像或内容创建的。渐变和着色提供了不同的方法来创建从一种颜色到另一种颜色的平滑过渡。

&emsp;创建和使用 Patterns、Gradients 和 Shadings 的详细信息都包含在 Quartz 2D Programming Guide。

#### Customizing the Coordinate Space（自定义坐标空间）
&emsp;默认情况下，UIKit 创建一个直接的当前变换矩阵，将点映射到像素上。尽管你可以在不修改矩阵的情况下绘制所有图形，但有时这样做会很方便。

&emsp;当你的视图的 `drawRect:` 方法第一次被调用时，CTM 被配置为坐标系的原点与你视图的原点相匹配，它的正 X 轴向右延伸，正 Y 轴向下延伸。但是，可以通过向 CTM 添加缩放、旋转和平移因子来更改 CTM，从而更改默认坐标系相对于基础视图或  window 的大小、方向和位置。

##### Using Coordinate Transforms to Improve Drawing Performance（使用坐标变换来提高绘图性能）
&emsp;修改 CTM 是在视图中绘制内容的标准技术，因为它允许重用路径，这可能会减少绘制时所需的计算量。例如，如果要从点（20，20）开始绘制正方形，可以创建一条移动到（20，20）的路径，然后绘制所需的线集以完成正方形。但是，如果以后决定将该正方形移动到点（10，10），则必须使用新的起点重新创建路径。因为创建路径是相对昂贵的操作，所以最好创建原点在（0，0）处的正方形，并且修改 CTM 以便在所需原点处绘制正方形。

&emsp;在 Core Graphics 框架中，有两种方法可以修改 CTM。你可以使用 CGContext Reference 中定义的 CTM 操作函数直接修改 CTM。你还可以创建 CGAffineTransform 结构，应用所需的任何转换，然后将该转换连接到 CTM 上。使用仿射变换可以将变换分组，然后将它们一次应用到 CTM。还可以计算和反转仿射变换，并使用它们修改代码中的点、大小和矩形值。有关使用仿射变换的更多信息，请参见  Quartz 2D Programming Guide 和 CGAffineTransform Reference。

##### Flipping the Default Coordinate System（翻转默认坐标系）
&emsp;在 UIKit 图形中翻转会修改 backing CALayer，使具有 LLO 坐标系的图形环境与 UIKit 的默认坐标系对齐。如果只使用 UIKit 方法和函数进行绘图，则不需要翻转 CTM。但是，如果将 Core Graphics 或图像 I/O 函数调用与 UIKit 调用混合使用，则可能需要翻转 CTM。

&emsp;具体来说，如果你通过直接调用 Core Graphics 函数来绘制图像或 PDF 文档，则对象将在视图的上下文中呈现为上下颠倒。必须翻转 CTM 才能正确显示图像和页面。

&emsp;要将绘制的对象翻转到 Core Graphics 上下文，以便在 UIKit 视图中正确显示，必须分两步修改 CTM。将原点平移到绘图区域的左上角，然后应用比例平移，将 y 坐标修改为 -1。执行此操作的代码如下所示：
```c++
CGContextSaveGState(graphicsContext);
CGContextTranslateCTM(graphicsContext, 0.0, imageHeight);
CGContextScaleCTM(graphicsContext, 1.0, -1.0);
CGContextDrawImage(graphicsContext, image, CGRectMake(0, 0, imageWidth, imageHeight));
CGContextRestoreGState(graphicsContext);
```
&emsp;如果使用 Core Graphics 图像对象初始化创建 UIImage 对象，UIKit 将为你执行翻转变换。每个 UIImage 对象都由 CGImageRef 不透明类型支持。你可以通过 CGImage 属性访问 Core Graphics 对象，并对图像进行一些处理。（Core Graphics 具有 UIKit 中不可用的图像相关功能。）完成后，可以从修改后的 CGImageRef 对象重新创建 UIImage 对象。

> Note: 可以使用 Core Graphics 函数 `CGContextDrawImage` 将图像绘制到任何渲染目标。此函数有两个参数，第一个参数用于图形上下文，第二个参数用于定义图像大小及其在图形表面（如视图）中的位置的矩形。使用 `CGContextDrawImage` 绘制图像时，如果不将当前坐标系调整为 LLO 方向，则图像在 UIKit 视图中显示为反转。此外，传入该函数的矩形的原点与调用该函数时当前坐标系的原点相对。

##### Side Effects of Drawing with Different Coordinate Systems（具有不同坐标系的图形的副作用）
&emsp;当你参照一种绘图技术的默认坐标系绘制对象，然后在另一种绘图技术的图形上下文中对其进行渲染时，会发现一些渲染异常。你可能需要调整代码以考虑这些副作用。

###### Arcs and Rotations（）
&emsp;如果使用 `CGContextAddArc` 和 `CGPathAddArc` 等函数绘制路径并采用 LLO 坐标系，则需要翻转 CTM 以在 UIKit 视图中正确渲染圆弧。但是，如果使用相同的函数创建点位于 ULO 坐标系中的圆弧，然后在 UIKit 视图中渲染路径，则会注意到该圆弧是其原始视图的更改版本。弧的终止端点现在指向与使用 UIBezierPath 类创建的弧相反的方向。例如，一个向下的箭头现在指向上方（如图 1-5 所示），弧 “弯曲” 的方向也不同。必须更改 Core Graphics 绘制弧的方向，以考虑基于 ULO 的坐标系；此方向由这些函数的 startAngle 和 endAngle 参数控制。

&emsp;Figure 1-5  Arc rendering in Core Graphics versus UIKit（Core Graphics 与 UIKit 中的 Arc 渲染）

![flipped_coordinates-1](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/f48ecd95cacf43a4b29863d46c909fc3~tplv-k3u1fbpfcp-watermark.image)

&emsp;如果旋转对象（例如，通过调用 `CGContextRotateCTM`），可以观察到相同类型的镜像效果。如果使用引用ULO坐标系的核心图形调用旋转对象，则在UIKit中渲染时对象的方向将反转。必须考虑代码中不同的旋转方向；使用CGContextRotateCTM，可以通过反转角度参数的符号来实现这一点（例如，负值变为正值）。

###### Shadows（）



























## 参考链接
**参考链接:🔗**
+ [Drawing and Printing Guide for iOS](https://developer.apple.com/library/archive/documentation/2DDrawing/Conceptual/DrawingPrintingiOS/Introduction/Introduction.html#//apple_ref/doc/uid/TP40010156)
