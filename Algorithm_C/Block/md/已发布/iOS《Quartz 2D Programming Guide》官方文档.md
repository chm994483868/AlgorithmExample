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

+ Quartz 2D Overview 描述了页面、绘图目标、Quartz 不透明数据类型、图形状态、坐标和内存管理，并介绍了Quartz如何 “under the hood” 工作








Overview of Quartz 2D describes the page, drawing destinations, Quartz opaque data types, graphics states, coordinates, and memory management, and it takes a look at how Quartz works “under the hood.”
Graphics Contexts describes the kinds of drawing destinations and provides step-by-step instructions for creating all flavors of graphics contexts.
Paths discusses the basic elements that make up paths, shows how to create and paint them, shows how to set up a clipping area, and explains how blend modes affect painting.
Color and Color Spaces discusses color values and using alpha values for transparency, and it describes how to create a color space, set colors, create color objects, and set rendering intent.
Transforms describes the current transformation matrix and explains how to modify it, shows how to set up affine transforms, shows how to convert between user and device space, and provides background information on the mathematical operations that Quartz performs.
Patterns defines what a pattern and its parts are, tells how Quartz renders them, and shows how to create colored and stenciled patterns.
Shadows describes what shadows are, explains how they work, and shows how to paint with them.
Gradients discusses axial and radial gradients and shows how to create and use CGShading and CGGradient objects.
Transparency Layers gives examples of what transparency layers look like, discusses how they work, and provides step-by-step instructions for implementing them.
Data Management in Quartz 2D discusses how to move data into and out of Quartz.
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
