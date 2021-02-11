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
+ Bitmap Images and Image Masks 描述了构成位图图像定义的内容，并演示了如何将位图图像用作石英绘图原语。它还描述了可以在图像上使用的遮罩技术，并显示了在绘制图像时使用混合模式可以实现的各种效果。








Bitmap Images and Image Masks describes what makes up a bitmap image definition and shows how to use a bitmap image as a Quartz drawing primitive. It also describes masking techniques you can use on images and shows the various effects you can achieve by using blend modes when drawing images.

Core Graphics Layer Drawing describes how to create and use drawing layers to achieve high-performance patterned drawing or to draw offscreen.
PDF Document Creation, Viewing, and Transforming shows how to open and view PDF documents, apply transforms to them, create a PDF file, access PDF metadata, add links, and add security features (such as password protection).
PDF Document Parsing describes how to use CGPDFScanner and CGPDFContentStream objects to parse and inspect PDF documents.
PostScript Conversion gives an overview of the functions you can use in Mac OS X to convert a PostScript file to a PDF document. These functions are not available in iOS.
Text describes Quartz 2D low-level support for text and glyphs, and alternatives that provide higher-level and Unicode text support. It also discusses how to copy font variations.
Glossary defines the terms used in this guide.
See Also

These items are essential reading for anyone using Quartz 2D:

Core Graphics Framework Reference provides a complete reference for the Quartz 2D application programming interface.
Color Management Overview is a brief introduction to the principles of color perception, color spaces, and color management systems.
Mailing lists. Join the quartz-dev mailing list to discuss problems using Quartz 2D.




## 参考链接
**参考链接:🔗**
+ [Quartz 2D Programming Guide](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/drawingwithquartz2d/Introduction/Introduction.html#//apple_ref/doc/uid/TP30001066)
+ [Core Graphics Framework Reference](https://developer.apple.com/documentation/coregraphics)
