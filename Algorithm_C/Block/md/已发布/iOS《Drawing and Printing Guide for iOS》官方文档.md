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













System views are redrawn automatically. For custom views, you must override the drawRect: method and perform all your drawing inside it. Inside your drawRect: method, use the native drawing technologies to draw shapes, text, images, gradients, or any other visual content you want. The first time your view becomes visible, iOS passes a rectangle to the view’s drawRect: method that contains your view’s entire visible area. During subsequent calls, the rectangle includes only the portion of the view that actually needs to be redrawn. For maximum performance, you should redraw only affected content.

After calling your drawRect: method, the view marks itself as updated and waits for new actions to arrive and trigger another update cycle. If your view displays static content, then all you need to do is respond to changes in your view’s visibility caused by scrolling and the presence of other views.

If you want to change the contents of the view, however, you must tell your view to redraw its contents. To do this, call the setNeedsDisplay or setNeedsDisplayInRect: method to trigger an update. For example, if you were updating content several times a second, you might want to set up a timer to update your view. You might also update your view in response to user interactions or the creation of new content in your view.

Important: Do not call your view’s drawRect: method yourself. That method should be called only by code built into iOS during a screen repaint. At other times, no graphics context exists, so drawing is not possible. (Graphics contexts are explained in the next section.)









## 参考链接
**参考链接:🔗**
+ [Drawing and Printing Guide for iOS](https://developer.apple.com/library/archive/documentation/2DDrawing/Conceptual/DrawingPrintingiOS/Introduction/Introduction.html#//apple_ref/doc/uid/TP40010156)
