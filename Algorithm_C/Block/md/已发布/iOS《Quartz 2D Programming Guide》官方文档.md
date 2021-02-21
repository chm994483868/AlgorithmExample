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








The opaque data types available in Quartz 2D include the following:

CGPathRef, used for vector graphics to create paths that you fill or stroke. See Paths.
CGImageRef, used to represent bitmap images and bitmap image masks based on sample data that you supply. See Bitmap Images and Image Masks.
CGLayerRef, used to represent a drawing layer that can be used for repeated drawing (such as for backgrounds or patterns) and for offscreen drawing. See Core Graphics Layer Drawing
CGPatternRef, used for repeated drawing. See Patterns.
CGShadingRef and CGGradientRef, used to paint gradients. See Gradients.
CGFunctionRef, used to define callback functions that take an arbitrary number of floating-point arguments. You use this data type when you create gradients for a shading. See Gradients.
CGColorRef and CGColorSpaceRef, used to inform Quartz how to interpret color. See Color and Color Spaces.
CGImageSourceRef and CGImageDestinationRef, which you use to move data into and out of Quartz. See Data Management in Quartz 2D and Image I/O Programming Guide.
CGFontRef, used to draw text. See Text.
CGPDFDictionaryRef, CGPDFObjectRef, CGPDFPageRef, CGPDFStream, CGPDFStringRef, and CGPDFArrayRef, which provide access to PDF metadata. See PDF Document Creation, Viewing, and Transforming.
CGPDFScannerRef and CGPDFContentStreamRef, which parse PDF metadata. See PDF Document Parsing.
CGPSConverterRef, used to convert PostScript to PDF. It is not available in iOS. See PostScript Conversion.










## 参考链接
**参考链接:🔗**
+ [Quartz 2D Programming Guide](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/drawingwithquartz2d/Introduction/Introduction.html#//apple_ref/doc/uid/TP30001066)
+ [Core Graphics Framework Reference](https://developer.apple.com/documentation/coregraphics)
