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
&emsp;Quartz 2D 使用画家的模型进行成像。在画家的模型中，每次连续的绘图操作都会对输出“画布”（通常称为页面）应用一层“绘制”。可以通过附加的绘图操作覆盖更多的绘图来修改页面上的绘图。不能修改页面上绘制的对象，除非覆盖更多的绘制。此模型允许您从少量强大的原语构建极其复杂的图像。




The Page

Quartz 2D uses the painter’s model for its imaging. In the painter’s model, each successive drawing operation applies a layer of “paint” to an output “canvas,” often called a page. The paint on the page can be modified by overlaying more paint through additional drawing operations. An object drawn on the page cannot be modified except by overlaying more paint. This model allows you to construct extremely sophisticated images from a small number of powerful primitives.

Figure 1-1 shows how the painter’s model works. To get the image in the top part of the figure, the shape on the left was drawn first followed by the solid shape. The solid shape overlays the first shape, obscuring all but the perimeter of the first shape. The shapes are drawn in the opposite order in the bottom of the figure, with the solid shape drawn first. As you can see, in the painter’s model the drawing order is important.






## 参考链接
**参考链接:🔗**
+ [Quartz 2D Programming Guide](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/drawingwithquartz2d/Introduction/Introduction.html#//apple_ref/doc/uid/TP30001066)
+ [Core Graphics Framework Reference](https://developer.apple.com/documentation/coregraphics)
