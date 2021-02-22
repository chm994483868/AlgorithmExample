# iOS《Quartz 2D Programming Guide》官方文档

## Introduction
&emsp;Core Graphics，也被称为 Quartz 2D，是一个先进的二维绘图引擎，可用于 iOS、tvOS 和 macOS 应用程序开发。Quartz 2D 提供低级别、轻量级的 2D 渲染，无论显示或打印设备如何，都具有无与伦比的输出保真度。Quartz 2D 是分辨率和设备无关。

&emsp;Quartz 2D API 易于使用，并提供对强大功能的使用，如透明层（transparency layers）、基于路径的绘图（path-based drawing）、离屏渲染（offscreen rendering）、高级颜色管理（advanced color management）、抗锯齿渲染（anti-aliased rendering）以及 PDF 文档创建（creation）、显示（display）和解析（parsing）。

### Who Should Read This Document?
&emsp;本文档适用于需要执行以下任何任务的开发人员：

+ Draw graphics（绘制图形）
+ Provide graphics editing capabilities in an application（在应用程序中提供图形编辑功能）
+ Create or display bitmap images（创建或显示位图图像）
+ Work with PDF documents（处理 PDF 文档）

### Organization of This Document（本文档的组织结构）
&emsp;本文档分为以下章节：

+ Quartz 2D Overview 描述了页面、绘图目标、Quartz 不透明数据类型、图形状态、坐标和内存管理，并介绍了 Quartz 如何 “under the hood” 工作（如何在引擎下工作）。
+ Graphics Contexts 描述了各种绘图目标，并提供了创建各种图形上下文的分步说明。
+ Paths 讨论构成路径的基本元素，演示如何创建和绘制路径，演示如何设置剪裁区域，并解释混合模式如何影响绘制。
+ Color and Color Spaces 讨论颜色值和透明度的 alpha 值，并描述如何创建颜色空间、设置颜色、创建颜色对象和设置渲染意图。
+ Transforms 描述当前变换矩阵并说明如何修改它，说明如何设置仿射变换，说明如何在用户和设备空间之间进行转换，并提供有关 Quartz 执行的数学运算的背景信息。
+ Patterns 定义了一个模式及其部分，告诉 Quartz 如何渲染它们，并展示了如何创建彩色和模版模式。
+ Shadows 描述了什么是阴影，解释了它们是如何工作的，并演示了如何使用它们进行绘制。
+ Gradients 讨论轴向和径向渐变，并演示如何创建和使用 CGShading 和 CGGradient 对象。
+ Transparency Layers 给出了透明层的外观示例，讨论了它们的工作方式，并提供了实现它们的逐步说明。
+ Data Management in Quartz 2D 讨论了如何将数据移入和移出 Quartz。
+ Bitmap Images and Image Masks 描述了构成位图图像定义的内容，并演示了如何将位图图像用作 Quartz 绘图原语（primitive）。它还描述了可以在图像上使用的遮罩技术，并显示了在绘制图像时使用混合模式可以实现的各种效果。
+ Core Graphics Layer Drawing 描述了如何创建和使用 drawing layers 来实现高性能的图形化绘制或在屏幕外绘制（to draw offscreen）。
+ PDF Document Creation, Viewing, and Transforming 演示如何打开和查看 PDF 文档、对其应用转换、创建 PDF 文件、访问 PDF 元数据、添加链接以及添加安全功能（如密码保护）。
+ PDF Document Parsing 描述了如何使用 CGPDFScanner 和 CGPDFContentStream 对象来解析和检查 PDF 文档。
+ PostScript Conversion 概述了可在 Mac OS X 中用于将 PostScript 文件转换为 PDF 文档的函数。这些功能在 iOS 中不可用。
+ Text 描述了 Quartz 2D 对文本和 glyph 的低级支持，以及提供高级和 Unicode 文本支持的替代方案。本文还讨论了如何复制字体变体。
+ Glossary 定义了本指南中使用的术语。

### See Also
&emsp;以下是使用 Quartz 2D 的所有人的必备阅读资料：
+ [Core Graphics Framework Reference](https://developer.apple.com/documentation/coregraphics) 提供了一个完整的参考 Quartz 2D 应用程序编程接口。
+ [Color Management Overview](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/csintro/csintro_intro/csintro_intro.html#//apple_ref/doc/uid/TP30001148) 是对色彩感知原理、色彩空间和色彩管理系统的简要介绍。
+ Mailing lists。加入 [quartz-dev](https://lists.apple.com) mailing list，讨论使用 Quartz 2D 的问题。

## Overview of Quartz 2D（Quartz 2D 概述）
&emsp;Quartz 2D 是一个二维绘图引擎，可以在 iOS 环境中访问，也可以从内核之外的所有 Mac OS X 应用程序环境访问。你可以使用 Quartz 2D 应用程序编程接口（API）访问功能，如基于路径的绘图（path-based drawing）、透明绘制（painting with transparency）、着色（shading）、图形阴影（drawing shadows）、透明层（transparency layers）、颜色管理（color management）、抗锯齿渲染（anti-aliased rendering）、PDF 文档生成和 PDF 元数据访问。只要有可能，Quartz 2D 就会利用图形硬件的强大功能。

&emsp;在 MacOS X 中，Quartz 2D 可以与所有其他图形和成像技术—Core Image、Core Video、OpenGL 和 QuickTime —配合使用。可以使用 QuickTime 函数 GraphicsImportCreateCGImage 从 QuickTime 图形导入器在 Quartz 中创建图像。有关详细信息，请参见 QuickTime 框架参考。在 Mac OS X 中，在 Quartz 2D 和 Core Image 之间移动数据（Moving Data Between Quartz 2D and Core Image in Mac OS X）描述了如何向 Core Image 提供图像，Core Image 是一个支持图像处理的框架。

&emsp;类似地，在 iOS 中，Quartz 2D 使用所有可用的图形和动画技术，如 Core Animation、OpenGL ES 和 UIKit 类。

### The Page
&emsp;Quartz 2D 使用 painter 的模型进行成像。在 painter 的模型中，每个连续的绘图操作都会对输出 "canvas"（通常称为 page）应用一层 "paint"。可以通过附加的绘图操作覆盖更多的绘图来修改 page 上的绘图。不能修改 page 上绘制的对象，除非覆盖更多的绘制。此模型允许你从少量强大的原语构建极其复杂的图像。

&emsp;图 1-1 显示了 painter 模型的工作原理。为了获得图形顶部的图像，首先绘制左侧的形状，然后绘制实体形状。实心形状覆盖第一个形状，除了第一个形状的周长外，其他形状都被遮挡。图 1-1 底部按相反顺序绘制图形，首先绘制实体图形。正如你所见，在 painter 的模型中，绘画顺序很重要。

![painters_model](https://p1-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/b5473fac8a0043799e793bfb04450a6c~tplv-k3u1fbpfcp-watermark.image)

&emsp;page 可以是一张真正的纸（如果输出设备是打印机）；也可以是一张虚拟的纸（如果输出设备是 PDF 文件）；甚至可以是位图图像。page 的确切性质取决于你使用的特定图形上下文。

### Drawing Destinations: The Graphics Context
&emsp;图形上下文是一种不透明的数据类型（CGContextRef），它封装了 Quartz 用于将图像绘制到输出设备（如 PDF 文件、位图或显示器上的窗口）的信息。图形上下文中的信息包括图形绘图参数和 page 上的特定于设备的绘画表示。Quartz 中的所有对象都被绘制到或包含在图形上下文中。

&emsp;可以将图形上下文视为绘图目标，如图 1-2 所示。使用 Quartz 绘制时，所有设备特定的特征都包含在所使用的特定类型的图形上下文中。换言之，你只需为同一序列的 Quartz 绘图例程提供不同的图形上下文，就可以将相同的图像绘制到不同的设备上。你不需要执行任何特定于设备的计算；Quartz 会为你执行。

&emsp;Figure 1-2  Quartz drawing destinations

![draw_destinations](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/0b76c444cb854fda9aa3e221e00bff86~tplv-k3u1fbpfcp-watermark.image)

&emsp;这些图形上下文可用于你的应用程序：
+ （bitmap graphics context）位图图形上下文允许你在位图中绘制 RGB 颜色、CMYK 颜色或灰度。位图是像素的矩形阵列（或光栅），每个像素代表图像中的一个点。位图图像（Bitmap images）也称为采样图像（sampled images）。请参见 Creating a Bitmap Graphics Context。

+ （PDF graphics context）PDF 图形上下文允许你创建 PDF 文件。在 PDF 文件中，drawing 将作为命令序列保留。PDF 文件和位图之间存在一些显著的差异：
  + 与位图不同，PDF文件可能包含多个 page。
  + 从不同设备上的 PDF 文件绘制 page 时，生成的图像将针对该设备的显示特性进行优化。
  + PDF 文件本质上是独立于分辨率的，在不牺牲图像细节的情况下，可以无限地增大或减小文件的大小。用户对位图图像的感知质量与要查看位图的分辨率有关。
请参见 Creating a PDF Graphics Context。

+ （window graphics context）window 图形上下文是可用于绘制到窗口中的图形上下文。请注意，因为 Quartz 2D 是一个图形引擎，而不是一个窗口管理系统，所以你可以使用其中一个应用程序框架来获取窗口的图形上下文。有关详细信息，请参见 Creating a Window Graphics Context in Mac OS X 。

+ （layer context ）图层上下文（CGLayerRef）是与其他图形上下文关联的屏幕外图形目标。它的设计是为了在将层绘制到创建它的图形上下文时获得最佳性能。对于屏幕外绘制，图层上下文可能比位图图形上下文更好。请参见 Core Graphics Layer Drawing。

+ （PostScript graphics context）如果要在 Mac OS X 中打印，则将内容发送到由打印框架管理的 PostScript 图形上下文。有关详细信息，请参见 Obtaining a Graphics Context for Printing。

### Quartz 2D Opaque Data Types
&emsp;除了图形上下文之外，Quartz 2D API 还定义了各种不透明的数据类型。因为 API 是 Core Graphics 框架的一部分，所以对其进行操作的数据类型和例程使用 CG 前缀。

&emsp;Quartz 2D 从不透明的数据类型创建对象，应用程序对这些数据类型进行操作以实现特定的图形输出。图 1-3 显示了将绘图操作应用于 Quartz 2D 提供的三个对象时可以获得的各种结果。例如：

+ 通过创建 PDF 页面对象，对图形上下文应用旋转操作，并要求 Quartz 2D 将页面绘制到图形上下文，可以旋转和显示 PDF 页面。
+ 可以通过创建图案对象、定义构成图案的形状，以及设置 Quartz 2D 以在图案绘制到图形上下文时将图案用作绘画来绘制图案。
+ 通过创建着色对象，提供确定着色中每个点的颜色的函数，然后要求 Quartz 2D 将着色用作填充颜色，可以使用轴向或径向着色填充区域。

&emsp;Figure 1-3  Opaque data types are the basis of drawing primitives in Quartz 2D

![drawing_primitives](https://p1-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/f2598ce6b52f4764aedf65690eb7302a~tplv-k3u1fbpfcp-watermark.image)

&emsp;Quartz 2D 中可用的不透明数据类型包括：

+ CGPathRef，用于矢量图形以创建填充或笔划的路径。请参见 Paths。
+ CGImageRef，用于根据你提供的示例数据表示位图图像和位图图像掩码。请参见 Bitmap Images and Image Masks。
+ CGLayerRef，用于表示可用于重复绘制（如背景或图案）和屏幕外绘制的绘图层。请参见 Core Graphics Layer Drawing。
+ CGPatternRef，用于重复绘制。请参见 Patterns。
+ CGShadingRef 和 CGGradientRef，用于绘制渐变。请参见 Gradients。
+ CGFunctionRef，用于定义接受任意数量浮点参数的回调函数。为着色（shading）创建渐变（gradients）时，将使用此数据类型。请参见 Gradients。
+ CGColorRef 和 CGColorSpaceRef，用来告诉 Quartz 如何解释颜色。请参见 Color and Color Spaces。
+ CGImageSourceRef 和 CGImageDestinationRef，用于将数据移入和移出 Quartz。请参见 Data Management in Quartz 2D 和 Image I/O Programming Guide。
+ CGFontRef，用于绘制文本。请参见 Text。
+ CGPDFDictionaryRef、CGPDFObjectRef、CGPDFPageRef、CGPDFStream、CGPDFStringRef 和 CGPDFArrayRef，提供对 PDF 元数据的访问。请参见 PDF Document Creation, Viewing, and Transforming。
+ CGPDFScannerRef 和 CGPDFContentStreamRef，用于解析 PDF 元数据。请参见 PDF Document Parsing。
+ CGPSConverterRef，用于将 PostScript 转换为 PDF。它在 iOS 中不可用。请参见 PostScript Conversion。

### Graphics States
&emsp;Quartz 根据当前 graphics state 中的参数修改绘图操作的结果。graphics state 包含参数，否则这些参数将作为绘图例程（drawing routines）的参数。绘制到图形上下文的例程参考 graphics state 以确定如何呈现其结果。例如，当你调用函数来设置填充颜色时，你正在修改存储在当前 graphics state 中的值。当前 graphics state 的其他常用元素包括线宽、当前位置和文本字体大小。

&emsp;graphics context 包含一堆（a stack of） graphics states。（应该翻译为 “图形上下文包含一个图形状态堆栈”）当 Quartz 创建图形上下文时，堆栈为空。保存图形状态时，Quartz 会将当前图形状态的副本推入到堆栈上。恢复图形状态时，Quartz 会将图形状态从堆栈顶部弹出。弹出状态变为当前图形状态。

&emsp;要保存当前图形状态，请使用函数 `CGContextSaveGState` 将当前图形状态的副本推入到堆栈上。要还原以前保存的图形状态，请使用函数 `CGContextRestoreGState` 将当前图形状态替换为堆栈顶部的图形状态。

&emsp;请注意，并非当前绘图环境的所有 aspects 都是图形状态的元素。例如，当前路径不被视为图形状态的一部分，因此在调用函数 `CGContextSaveGState` 时不会保存。调用此函数时保存的图形状态参数如表 1-1 所示。

&emsp;Table 1-1  Parameters that are associated with the graphics state（与图形状态关联的参数）
| Parameters | Discussed in this chapter |
| --- | --- |
| Current transformation matrix (CTM) | Transforms |
| Clipping area | Paths |
| Line: width, join, cap, dash, miter limit | Paths |
| Accuracy of curve estimation (flatness) | Paths |
| Anti-aliasing setting | Graphics Contexts |
| Color: fill and stroke settings | Color and Color Spaces |
| Alpha value (transparency) | Color and Color Spaces |
| Rendering intent | Color and Color Spaces |
| Color space: fill and stroke settings | Color and Color Spaces |
| Text: font, font size, character spacing, text drawing mode | Text |
| Blend mode | Paths and Bitmap Images and Image Masks |

### Quartz 2D Coordinate Systems
&emsp;如图 1-4 所示，坐标系定义了用于表示要在页面上绘制的对象的位置和大小的位置范围。可以在用户空间坐标系或更简单的用户空间中指定图形的位置和大小。坐标被定义为浮点值。

&emsp;Figure 1-4  The Quartz coordinate system

![quartz_coordinates](https://p1-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/b454ea014df644feb3ca038f87927c5f~tplv-k3u1fbpfcp-watermark.image)

&emsp;由于不同的设备具有不同的基本成像能力，因此必须以独立于设备的方式定义图形的位置和大小。例如，屏幕显示设备可以每英寸显示不超过 96 个像素，而打印机可以每英寸显示 300 个像素。如果在设备级别定义坐标系（在本例中为 96 像素或 300 像素），则在该空间中绘制的对象无法在其他设备上复制（reproduced 复制、重现、再版），而不会产生可见的失真。它们会显得太大或太小。

&emsp;Quartz 通过使用 current transformation matrix（CTM）将单独的坐标系用户空间映射到输出设备空间的坐标系来实现设备独立性。矩阵是用来有效描述一组相关方程的数学结构。当前变换矩阵是一种特殊类型的矩阵，称为仿射变换，它通过应用平移、旋转和缩放操作（移动、旋转和调整坐标系大小的计算）将点从一个坐标空间映射到另一个坐标空间。

&emsp;当前变换矩阵还有一个次要用途：它允许你变换对象的绘制方式。例如，要绘制旋转 45 度的长方体，请先旋转页面的坐标系（CTM），然后再绘制长方体。Quartz 使用旋转坐标系绘制到输出设备。

&emsp;用户空间中的点由坐标对（x，y）表示，其中 x 表示沿水平轴（左和右）的位置，y 表示垂直轴（上和下）。用户坐标空间的原点是点（0,0）。原点位于页面的左下角，如图 1-4 所示。在 Quartz 的默认坐标系中，x 轴从页面的左侧向右侧移动时会增加。当 y 轴从页面底部向顶部移动时，其值会增加。

&emsp;一些技术使用不同于 Quartz 使用的默认坐标系来设置图形上下文。相对于 Quartz，这种坐标系是一种修改后的坐标系，在执行某些 Quartz 绘图操作时必须进行补偿。最常见的修改坐标系将原点放置在上下文的左上角，并将 y 轴更改为指向页面底部。你可能会看到使用此特定坐标系的几个地方如下所示：

+ 在 Mac OS X中，NSView 的子类重写其 isFlipped 方法以返回 YES。
+ 在 iOS 中，UIView 返回的绘图上下文。
+ 在 iOS 中，通过调用 `UIGraphicsBeginImageContextWithOptions` 函数创建的图形上下文。

&emsp;UIKit 返回具有修改坐标系的 Quartz 图形上下文的原因是 UIKit 使用不同的默认坐标约定；它将转换应用于它创建的 Quartz 上下文，以便它们与它的约定相匹配。如果应用程序希望使用相同的绘图例程绘制 UIView 对象和 PDF 图形上下文（由 Quartz 创建并使用默认坐标系），则需要应用变换，以便 PDF 图形上下文接收相同的修改坐标系。为此，应用一个转换，将原点转换到 PDF 上下文的左上角，并将 y 坐标缩放 -1（应该是乘以负 1）。

&emsp;使用缩放变换来消除 y 坐标会改变 Quartz 绘图中的一些约定。例如，如果调用 `CGContextDrawImage` 将图像绘制到上下文中，则在将图像绘制到目标中时，转换会对图像进行修改。类似地，路径绘制例程接受指定在默认坐标系中是顺时针还是逆时针方向绘制圆弧的参数。如果修改了坐标系，结果也会被修改，就像图像在镜子中反射一样。在图 1-5 中，将相同的参数传递到 Quartz 会在默认坐标系中产生一个顺时针的弧，在 y 坐标被变换取反后产生一个逆时针的弧。

&emsp;Figure 1-5  Modifying the coordinate system creates a mirrored image.（修改坐标系将创建一个镜像图像。）

![flipped_coordinates](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/c76ff5ca622143a9b28b038db19904c1~tplv-k3u1fbpfcp-watermark.image)

&emsp;由你的应用程序来调整它对应用了转换的上下文所做的任何 Quartz 调用。例如，如果希望图像或 PDF 正确地绘制到图形上下文中，应用程序可能需要临时调整图形上下文的 CTM。在 iOS 中，如果使用 UIImage 对象包装创建的 CGImage 对象，则不需要修改 CTM。UIImage 对象自动补偿 UIKit 应用的修改后的坐标系。

> Important: 如果你计划在 iOS 上直接针对 Quartz 编写应用程序，那么上面的讨论对于理解这一点非常重要，但这还不够。在 iOS 3.2 及更高版本上，当 UIKit 为应用程序创建图形上下文时，它还会对上下文进行其他更改，以匹配默认的 UIKit 约定。特别是，不受 CTM 影响的模式和阴影将分别进行调整，以便它们的约定与 UIKit 的坐标系匹配。在这种情况下，你的应用程序无法使用与 CTM 等效的机制来更改 Quartz 创建的上下文，以匹配 UIKit 提供的上下文的行为；你的应用程序必须识别它所引入的上下文类型，并调整其行为以匹配上下文的期望。

### Memory Management: Object Ownership
&emsp;Quartz 使用 Core Foundation 内存管理模型，其中对象是引用计数的。创建 Core Foundation 对象时，引用计数为 1。你可以通过调用函数来保留对象来增加引用计数，通过调用函数来释放对象来减少引用计数。当引用计数减少到 0 时，对象被释放。此模型允许对象安全地共享对其他对象的引用。

&emsp;需要牢记一些简单的规则：

+ 如果你创建或复制一个对象，你就拥有它，因此你必须释放它。也就是说，一般来说，如果从名称中带有 “Create” 或 “Copy” 字样的函数中获取对象，则必须在处理完该对象后释放该对象。否则，会导致内存泄漏。
+ 如果从名称中不包含单词 “Create” 或 “Copy” 的函数中获取对象，则不拥有对该对象的引用，并且不能释放该对象。该对象将在将来某个时候由其所有者释放。
+ 如果你不拥有某个对象并且需要将其保留在周围，则必须保留该对象，并在处理完该对象后将其释放。可以使用特定于对象的 Quartz 2D 函数来保留和释放该对象。例如，如果收到对 CGColorspace 对象的引用，则可以使用函数 `CGColorSpaceRetain` 和 `CGColorSpaceRelease` 根据需要保留和释放该对象。你也可以使用 Core Foundation  础函数 `CFRetain` 和 `CFRelease`，但必须注意不要将 NULL 传递给这些函数。












## 参考链接
**参考链接:🔗**
+ [Quartz 2D Programming Guide](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/drawingwithquartz2d/Introduction/Introduction.html#//apple_ref/doc/uid/TP30001066)
+ [Core Graphics Framework Reference](https://developer.apple.com/documentation/coregraphics)
