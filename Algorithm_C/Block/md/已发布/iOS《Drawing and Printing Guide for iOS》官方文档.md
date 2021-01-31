# iOS《Drawing and Printing Guide for iOS》官方文档

## About Drawing and Printing in iOS
&emsp;本文档涉及三个相关主题：
+ 绘制自定义 UI 视图。自定义 UI 视图允许你绘制无法使用标准 UI 元素轻松绘制的内容。例如，绘图程序可能使用自定义视图来绘制用户的绘图，或者街机游戏可能使用自定义视图来绘制精灵。
+ 绘制到 offscreen 位图和 PDF 内容。无论你是计划稍后显示图像、将它们导出到文件、还是将图像打印到启用 AirPrint 的打印机，offscreen 绘图都允许你在不中断用户的工作流的情况下显示图像。
+ 将 AirPrint 支持添加到应用。iOS 打印系统允许你以不同的方式绘制内容以适合 page。

&emsp;Figure I-1  You can combine custom views with standard views, and even draw things offscreen.（你可以将自定义视图与标准视图相结合，甚至可以将内容从 offscreen 绘制。）

![UI_overlay](https://p1-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/a9e70c6802d04c458b0bb4480d026670~tplv-k3u1fbpfcp-watermark.image)

### At a Glance
&emsp;iOS 原生图形系统结合了三种主要技术：UIKit、Core Graphics 和 Core Animation。UIKit 在这些视图中提供了视图和一些高级绘图功能，Core Graphics 在 UIKit 视图中提供了附加（低级）绘图支持，Core Animation 提供了将 transformations 和动画应用于 UIKit 视图的能力。Core Animation 还负责视图合成（view compositing）。

#### Custom UI Views Allow Greater Drawing Flexibility（自定义 UI 视图允许更大的绘图灵活性）
&emsp;本文档描述如何使用原生绘图技术绘制到自定义 UI 视图中。这些技术包括 Core Graphics 和 UIKit frameworks，支持 2D 绘图。




















## 参考链接
**参考链接:🔗**
+ [Drawing and Printing Guide for iOS](https://developer.apple.com/library/archive/documentation/2DDrawing/Conceptual/DrawingPrintingiOS/Introduction/Introduction.html#//apple_ref/doc/uid/TP40010156)
