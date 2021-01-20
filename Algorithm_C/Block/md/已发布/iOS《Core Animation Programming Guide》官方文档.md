# iOS《Core Animation Programming Guide》官方文档

> &emsp;The base layer class.

## CALayer
&emsp;管理基于图像的内容并允许你对该内容执行动画的对象。
```c++
API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0))
@interface CALayer : NSObject <NSSecureCoding, CAMediaTiming>
{
@private
  struct _CALayerIvars {
    int32_t refcount;
    uint32_t magic;
    void *layer;
#if TARGET_OS_MAC && !TARGET_RT_64_BIT
    void * _Nonnull unused1[8];
#endif
  } _attr;
}
```
### Overview
&emsp;Layers 通常用于为 view 提供备份存储，但也可以在没有 view 的情况下使用以显示内容。layer 的主要工作是管理你提供的视觉内容，但 layer 本身也具有可以设置的视觉属性，例如背景色、边框和阴影。除了管理视觉内容外，layer 还维护有关其内容的几何图形（例如其位置、大小和变换（transform））的信息，这些信息用于在屏幕上显示该内容。修改 layer 的属性是在 layer 的内容或几何体上启动动画的方式。layer 对象通过 CAMediaTiming 协议封装 layer 及其动画的持续时间和步调（pacing），该协议定义了 layer 的计时信息（timing information）。

&emsp;如果 layer 对象是由 view 创建的，则 view 通常会自动将自身指定为 layer 的代理，并且不应更改该关系。对于你自己创建的 layer，可以指定一个 delegate，并使用该对象动态提供 layer 的内容并执行其他任务。layer 还可以具有布局管理器（layout manager）对象（指定给 layoutManager 属性），以分别管理子视图（subviews）的布局。
### Creating a Layer（创建 layer）
#### + layer
&emsp;创建并返回 layer 对象的实例。
```c++
+ (instancetype)layer;
```
&emsp;Return Value: 初始化的 layer 对象；如果初始化失败，则返回 nil。

&emsp;如果你是 CALayer 的子类，则可以重写此方法，并使用该函数提供特定子类的实例。
#### - init
&emsp;返回一个初始化的 CALayer 对象。
```c++
- (instancetype)init;
```
&emsp;这是不在 presentation layer 中的 layer 对象的指定初始化程序。
#### - initWithLayer:
&emsp;重写以复制或初始化指定 layer 的自定义字段。

&emsp;CoreAnimation 使用此初始值设定项来创建 layers 的 shadow 副本，例如用作 presentation layers。子类可以重写此方法，以将其实例变量复制到 presentation layer 中（子类随后应调用超类）。在任何其他情况下调用此方法都将导致未定义的行为。
```c++
- (instancetype)initWithLayer:(id)layer;
```
&emsp;`layer`: 应从其复制自定义字段的 layer。

&emsp;Return Value:从 `layer` 复制的任何自定义实例变量的 layer 实例。

&emsp;此初始化程序用于创建 layer 的 shadow 副本，例如，用于 presentationLayer 方法。在任何其他情况下使用此方法都会产生不确定的行为。例如，请勿使用此方法用现有 layer 的内容初始化新 layer。

&emsp;如果要实现自定义 layer 子类，则可以重写此方法并将其用于将实例变量的值复制到新对象中。子类应始终调用超类实现。

&emsp;此方法是 presentation layer 中各 layer 对象的指定初始化器。
### Accessing Related Layer Objects（访问相关 layer 对象）
#### - presentationLayer
&emsp;返回 presentation layer 对象的副本，该对象表示当前在屏幕上显示的 layer 的状态。
```c++
- (nullable instancetype)presentationLayer;
```
&emsp;Return Value: 当前 presentation layer 对象的副本。

&emsp;通过此方法返回的 layer 对象提供了当前在屏幕上显示的 layer 的近似值。在动画制作过程中，你可以检索该对象并使用它来获取那些动画的当前值。

&emsp;返回 layer 的 sublayers、mask 和 superlayer 属性从表示树（presentation tree）（而不是模型树）返回相应的对象。此模式也适用于任何只读 layer 方法。例如，返回对象的 hitTest: 方法查询 presentation tree 中的 layer 对象。
#### - modelLayer

### Accessing the Delegate
#### delegate

### Providing the Layer’s Content
#### contents
#### contentsRect
#### contentsCenter
#### - display
#### - drawInContext:

### Modifying the Layer’s Appearance
#### contentsGravity
#### Contents Gravity Values
#### opacity
#### hidden
#### masksToBounds
#### mask
#### doubleSided
#### cornerRadius
#### maskedCorners
#### CACornerMask
#### borderWidth
#### borderColor
#### backgroundColor
#### shadowOpacity
#### shadowRadius
#### shadowOffset
#### shadowColor
#### shadowPath
#### style
#### allowsEdgeAntialiasing

### Layer Filters
#### filters
#### compositingFilter
#### backgroundFilters
#### minificationFilter
#### minificationFilterBias
#### magnificationFilter

### Configuring the Layer’s Rendering Behavior

### Modifying the Layer Geometry

### Managing the Layer’s Transform

### Managing the Layer Hierarchy

### Updating Layer Display

### Layer Animations

### Managing Layer Resizing and Layout

### Managing Layer Constraints

### Getting the Layer’s Actions

### Mapping Between Coordinate and Time Spaces

### Hit Testing

### Scrolling

### Identifying the Layer

### Key-Value Coding Extensions

### Constants

### Instance Properties

### Type Methods

## About Core Animation
&emsp;Core Animation 是可在 iOS 和 OS X 上使用的图形渲染（graphics rendering）和动画基础结构（animation infrastructure），可用于为应用程序的 view 和其他视觉元素制作动画。使用 Core Animation，绘制动画的每一帧所需的大部分工作都已为你完成。你所要做的就是配置一些动画参数（例如起点和终点），并告诉 Core Animation 开始。其余部分由 Core Animation 完成，将大部分实际绘图工作（actual drawing work）交给板载图形硬件（GPU）以加快渲染速度。这种自动图形加速功能可实现高帧率和流畅的动画效果，而不会给 CPU 造成负担并降低应用程序的运行速度。

&emsp;如果你在编写 iOS 应用程序，不管你是否知道，你都在使用 Core Animation。如果你正在编写 OS X 应用程序，则可以毫不费力地利用 Core Animation 的优势。Core Animation 位于 AppKit 和 UIKit 之下，并与 Cocoa 和 Cocoa Touch 的 view 工作流紧密集成。当然，Core Animation 还具有扩展应用程序 views 所公开功能的接口，并让你对应用程序动画进行更细粒度的控制。

![ca_architecture](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/c564c9a1e6f146018f27b3e546b6abf8~tplv-k3u1fbpfcp-watermark.image)

### At a Glance
&emsp;你可能永远不需要直接使用 Core Animation，但当你这样做时，你应该了解 Core Animation 作为应用程序基础结构的一部分所扮演的角色。
#### Core Animation Manages Your App’s Content（Core Animation 管理你应用的内容）
&emsp;Core Animation 本身不是一个绘图系统。它是在硬件中合成和操作应用程序内容的基础结构。此基础结构的核心是 layer 对象，你可以使用它来管理和操作内容。layer 将你的内容捕获到位图（bitmap）中，该位图可由图形硬件轻松操作。在大多数应用程序中，layer 被用作管理 view 内容的一种方式，但你也可以根据需要创建独立的 layer。

&emsp;相关章节：Core Animation Basics、Setting Up Layer Objects。
#### Layer Modifications Trigger Animations（Layer 修改触发动画）
&emsp;使用 Core Animation 创建的大多数动画都涉及对 layer 属性的修改。与 view 一样，layer 对象有一个 bounds 矩形、屏幕上的位置、不透明度、变换和许多其他可以修改的面向视觉的属性。对于大多数这些属性，更改属性的值会导致创建隐式动画，从而使 layer 从旧值动画到新值动画。在需要对生成的动画行为进行更多控制的情况下，也可以显式设置这些属性的动画。

&emsp;相关章节：Animating Layer Content、Advanced Animation Tricks、Layer Style Property Animations、Animatable Properties。
#### Layers Can Be Organized into Hierarchies（可以将 layers 组织到层次结构中）
&emsp;可以按层次排列 layers 以创建 parent-child 关系。layers 的排列以类似于 view 的方式影响它们管理的视觉内容。附加到 view 的一组 layers 的层次结构反映了相应的 view 层次结构。你还可以将独立 layers 添加到 layer 层次结构中，以将应用程序的视觉内容扩展到 views 之外。

&emsp;相关章节：Building a Layer Hierarchy。
#### Actions Let You Change a Layer’s Default Behavior（Actions 可让你更改 layer 的默认行为）
&emsp;隐式 layer 动画是使用 action 对象实现的，这些对象是实现预定义接口的通用对象。Core Animation 使用 action 对象来实现通常与 layers 关联的默认动画集。你可以创建自己的 action 对象来实现自定义动画，也可以使用它们来实现其他类型的行为。然后，将 action 对象分配给 layers 的属性之一。当该属性更改时，Core Animation 会检索你的 action 对象，并告诉它执行其 action。

&emsp;相关章节：Changing a Layer’s Default Behavior。
### How to Use This Document
&emsp;本文档适用于需要对应用程序动画进行更多控制或希望使用 layers 来提高应用程序绘图性能的开发人员。本文档还提供了有关 iOS 和 OS X 的 layers 和 views 之间 integration 的信息。iOS 和 OS X 的 layers 和 views 之间的 integration 是不同的，了解这些差异对于创建高效的动画非常重要。

### Prerequisites
&emsp;你应该已经了解目标平台的 view 架构，并且熟悉如何创建基于 view 的动画。如果没有，你应该阅读以下文档之一：
+ 对于iOS应用，你应该了解 [View Programming Guide for iOS](https://developer.apple.com/library/archive/documentation/WindowsViews/Conceptual/ViewPG_iPhoneOS/Introduction/Introduction.html#//apple_ref/doc/uid/TP40009503)  中描述的视图架构。
+ 对于OS X应用程序，你应该了解 [View Programming Guide](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/CocoaViewsGuide/Introduction/Introduction.html#//apple_ref/doc/uid/TP40002978) 中描述的视图架构。
### See Also
&emsp;有关如何使用 Core Animation 实现特定类型的动画的示例，请参见 [Core Animation Cookbook](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/CoreAnimation_Cookbook/Introduction/Introduction.html#//apple_ref/doc/uid/TP40005406)。
## Core Animation Basics（Core Animation 基础）
&emsp;Core Animation 提供了一个通用系统，用于为应用程序的 views 和其他视觉元素设置动画。Core Animation 不能代替应用程序的 views。相反，它是一种与 views integrates 的技术，可以为动画内容提供更好的性能和支持。它通过将 views 的内容缓存到位图（bitmaps）中来实现这种行为，位图可以由图形硬件（GPU）直接操作。在某些情况下，这种缓存行为可能要求你重新考虑如何呈现和管理应用程序的内容，但大多数情况下，你使用 Core Animation 时根本不知道它在那里。除了缓存 views 内容之外，Core Animation 还定义了一种方法来指定任意视觉内容，将该内容与 views integrates，并将其与其他内容一起设置动画。

&emsp;你可以使用 Core Animation 为应用程序 views 和视觉对象的更改设置动画。大多数更改与修改视觉对象的属性有关。例如，可以使用 Core Animation 来设置对 views position、size 或 opacity 的更改的动画。进行此类更改时，Core Animation 将在属性的当前值和指定的新值之间设置动画。通常不会使用 Core Animation 每秒替换 views 内容 60 次，例如在卡通动画（cartoon）中。相反，你可以使用 Core Animation 在屏幕上移动 views 的内容、淡入淡出该内容、对 views 应用任意图形变换（graphics transformations）或更改 view 的其他视觉属性。
### Layers Provide the Basis for Drawing and Animations（layers 提供绘图和动画的基础）
&emsp;layer 对象是在 3D 空间中组织的 2D 面，并且是你使用 Core Animation 所做的一切的核心。与 view 一样，layers 管理有关其 2D 面的几何图形、内容和视觉属性的信息。与 view 不同，layers 不定义自己的外观。layer 仅管理位图周围的状态信息。位图本身可以是 view 绘图本身的结果，也可以是指定的固定图像。因此，应用程序中使用的主要 layers 被认为是模型对象，因为它们主要管理数据。记住这个概念很重要，因为它会影响动画的行为。
#### The Layer-Based Drawing Model（基于 layer 的绘图模型）
&emsp;大多数 layers 不会在你的应用程序中进行任何实际绘制。相反，layer 捕获应用程序提供的内容并将其缓存在位图中，有时称为备份存储（backing store）。随后更改 layer 的属性时，所做的只是更改与 layer 对象关联的状态信息。当更改触发动画时，Core Animation 将 layer 的位图和状态信息传递给图形硬件，图形硬件使用新信息渲染位图，如图 1-1 所示。在硬件中操作位图会产生比在软件中更快的动画。

&emsp;Figure 1-1  How Core Animation draws content（Core Animation 如何绘制内容）

![basics_layer_rendering](https://p1-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/7c216ac63a2a433bb0a40be831633476~tplv-k3u1fbpfcp-watermark.image)

&emsp;基于 layer 的绘图由于它操纵静态位图，因此与传统的基于 view 的绘图技术大不相同。使用基于 view 的绘图时，对 view 本身的更改通常会导致调用该 view 的 `drawRect:` 方法，以使用新参数重新绘制内容。但是以这种方式绘图成本很高，因为它是使用主线程上的 CPU 完成的。Core Animation 尽可能通过在硬件中操纵缓存的位图来实现相同或相似的效果，从而避免了这种开销。

&emsp;尽管 Core Animation 尽可能多地使用缓存的内容，但你的应用仍必须提供初始内容并不时更新。你的应用可以通过多种方式为 layer 对象提供内容，有关详细信息，请参见 Providing a Layer’s Contents。
#### Layer-Based Animations（基于 layer 的动画）
&emsp;layer 对象的数据和状态信息与该 layer 内容在屏幕上的视觉呈现是分离的。这种解耦为 Core Animation 提供了一种方法，使其自身进行插入并设置从旧状态值到新状态值的变化的动画。例如，更改 layer 的 position 属性会使 Core Animation 将 layer 从其当前位置移动到新指定的位置。对其他属性的类似更改会导致适当的动画。图 1-2 说明了可以在 layer 上执行的几种动画类型。有关触发动画的 layer 属性的列表，请参见 Animatable Properties。

&emsp;Figure 1-2  Examples of animations you can perform on layers（可以在 layer 上执行的动画示例）

![basics_animation_types](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/529172157fb94024beb0d6da0dfb33a2~tplv-k3u1fbpfcp-watermark.image)

&emsp;在动画过程中，Core Animation 用硬件为你完成所有的逐帧绘制。你所要做的就是指定动画的起点和终点，然后让 Core Animation 完成其余的工作。你还可以根据需要指定自定义时间信息和动画参数。但是，如果没有，Core Animation 将提供合适的默认值。

&emsp;有关如何启动动画和配置动画参数的更多信息，请参见 Animating Layer Content。
### Layer Objects Define Their Own Geometry（layer 对象定义自己的几何）
&emsp;layer 的一项工作是管理其内容的视觉几何（visual geometry）。视觉几何包含有关该内容的 bounds，其在屏幕上的 position 以及该 layer 是否已以任何方式 rotated、scaled 或 transformed 的信息。就像 view 一样，layer 具有 frame 和 bounds 矩形，你可以使用它们来放置 layer 及其内容。layer 还具有 view 不具备的其他属性，例如：锚点（anchor point），它定义了在其周围进行操作的点。指定 layer geometry 某些方面的方式也不同于为 view 指定该信息的方式。
#### Layers Use Two Types of Coordinate Systems（layers 使用两种类型的坐标系）
&emsp;layer 同时使用基于点（point-based）的坐标系和单位坐标系来指定内容的放置。使用哪种坐标系取决于所传递信息的类型。指定直接映射到屏幕坐标的值或必须相对于另一个 layer 指定的值（例如图层的 position 属性）时，将使用基于点的坐标。当值不应绑定到屏幕坐标时使用单位坐标，因为它是相对于其他值的。例如，layer 的 anchorPoint 属性指定相对于 layer 本身 bounds 的点，该点可以更改。

&emsp;基于点的坐标最常见的用途是指定 layer 的 size 和 position，你可以使用 layer 的 bounds 和 position 属性来进行指定。bounds 定义了 layer 本身的坐标系，并包含了屏幕上 layer 的大小。 position 属性定义了 layer 相对于其父级坐标系的位置。尽管 layer 具有 frame 属性，但该属性实际上是从 bounds 和 position 属性中的值派生的，因此使用频率较低。

&emsp;layer bounds 和 frame 矩形的方向始终与基础平台的默认方向匹配。图1-3 显示了 iOS 和 OS X 上 bounds 矩形的默认方向。在 iOS 中，bounds 矩形的原点默认位于 layer 的左上角，而在 OS X 中则位于 底部-左边（bottom-left）角。如果你在 iOS 和 OS X 版本的应用程序之间共享 Core Animation 代码，则必须考虑这些差异。

&emsp;Figure 1-3  The default layer geometries for iOS and OS X（iOS 和 OS X 的默认 layer geometries）

![layer_coords_bounds](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/ad2d402d312d45e8a8ff6f41f6707fad~tplv-k3u1fbpfcp-watermark.image)

&emsp;图1-3 中要注意的一件事是 position 属性位于 layer 的中间。该属性是其定义根据 layer 的 anchorPoint 属性中的值而更改的几个属性之一。锚点表示某些坐标的起源点，并且在  Anchor Points Affect Geometric Manipulations 中有更详细的描述。

&emsp;锚点是你使用单位坐标系指定的多个属性之一。 Core Animation 使用单位坐标来表示属性，这些属性的值在 layer 大小更改时可能会更改。你可以将单位坐标视为指定总可能值的百分比。单位坐标空间中的每个坐标的范围为 0.0 到 1.0。例如，沿 x 轴，左边缘位于坐标 0.0，右边缘位于坐标 1.0。沿 y 轴，单位坐标值的方向根据平台而变化，如图 1-4 所示。

&emsp;Figure 1-4  The default unit coordinate systems for iOS and OS X（iOS 和 OS X 的默认单位坐标系）

![layer_coords_unit](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/cd09ff0fa3f247c9b104bbb85b4ffdf7~tplv-k3u1fbpfcp-watermark.image)

> &emsp;Note: 在 OS X 10.8 之前，geometryFlipped 属性是在需要时更改 layer y 轴默认方向的一种方法。涉及翻转变换（flip transforms）时，有时必须使用此属性来校正 layer 的方向。例如，如果父 view 使用了翻转变换，则其子 view（及其对应 layer）的内容通常会被反转。在这种情况下，将子 layer 的 geometryFlipped 属性设置为 YES 是纠正问题的简便方法。在 OS X 10.8 及更高版本中，AppKit 会为你管理此属性，并且你不应对其进行修改。对于 iOS 应用程序，建议你完全不要使用 geometryFlipped 属性。

&emsp;所有坐标值（无论是点坐标还是单位坐标）都指定为浮点数。使用浮点数可让你指定可能位于法线坐标值之间的精确位置。使用浮点值很方便，尤其是在打印过程中或绘制到可能由多个像素表示一个点的 Retina 显示器时。浮点值使你可以忽略基础设备分辨率，而仅以所需的精度指定值。
#### Anchor Points Affect Geometric Manipulations（锚点影响几何操纵）
&emsp;layer 的几何相关操作是相对于该 layer 的锚点进行的，你可以使用该 layer 的 anchorPoint 属性进行访问。在操纵 layer 的 position 或 transform 属性时，锚点的影响最为明显。始终相对于 layer 的锚点指定 position 属性，并且你应用于 layer 的任何变换也都相对于锚点进行。

&emsp;图1-5 演示了如何将锚点从其默认值更改为其他值如何影响 layer 的 position 属性。即使 layer 未在其父级 bounds 内移动，将锚点从 layer 的中心移动到 layer 的 bounds 原点也会更改 position 属性中的值。

&emsp;Figure 1-5  How the anchor point affects the layer’s position property（锚点如何影响 layer 的 position 属性）

![layer_coords_anchorpoint_position](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/b613b3a352c546d29bba35ddf9e7b64e~tplv-k3u1fbpfcp-watermark.image)

&emsp;图1-6 显示了更改锚点如何影响应用于 layer 的变换。将旋转变换应用于 layer 时，旋转会围绕锚点发生。由于锚点默认情况下设置为图层的中间位置，因此通常会创建你期望的旋转行为。但是，如果更改锚点，则旋转的结果将不同。

&emsp;Figure 1-6  How the anchor point affects layer transformations（锚点如何影响 layer 转换（transformations））

![layer_coords_anchorpoint_transform](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/8668f933f653469ca4ca545b85b4af96~tplv-k3u1fbpfcp-watermark.image)

#### Layers Can Be Manipulated in Three Dimensions（可以在三个维度上操纵 layers）
&emsp;每个 layer 都有两个变换矩阵（transform matrices），可用于操纵该 layer 及其内容。 CALayer 的 transform 属性指定要同时应用于该 layer 及其嵌入式子 layer 的转换。通常，当你要修改 layer 本身时，可以使用此属性。例如，你可以使用该属性缩放或旋转 layer 或临时更改其位置。 sublayerTransform 属性定义仅适用于子 layer 的其他转换，并且最常用于向场景内容添加透视视觉效果。

&emsp;transforms 的工作方式是将坐标值乘以一个数字矩阵，以获得表示原始点的变换版本的新坐标。因为可以在三个维度上指定 Core Animation 值，所以每个坐标点都有四个值，必须将这些值乘以四乘四矩阵，如图 1-7 所示。在 Core Animation 中，图中的变换由 CATransform3D 类型表示。幸运的是，你不必直接修改此结构的字段即可执行标准转换。 Core Animation 提供了一组全面的功能，用于创建比例尺，平移和旋转矩阵以及进行矩阵比较。除了使用函数来操纵变换之外，Core Animation 还扩展了键值编码支持，使你可以使用 key path 修改变换。有关可以修改的 key path 的列表，请参阅 CATransform3D Key Paths。

&emsp;Figure 1-7  Converting a coordinate using matrix math（使用矩阵数学转换坐标）

![transform_basic_math](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/f57daa03742649c8a161e8bd6fd6a971~tplv-k3u1fbpfcp-watermark.image)

+ x' = x * m11 + y * m12 + z * m13 + 1 * m14
+ y' = x * m21 + y * m22 + z * m23 + 1 * m24
+ z' = x * m31 + y * m32 + z * m33 + 1 * m34

&emsp;图1-8 显示了你可以进行的一些更常见转换的矩阵配置。将任何坐标乘以恒等（identity 矩阵）变换将返回完全相同的坐标。对于其他转换，如何修改坐标完全取决于你更改的矩阵成分。例如，仅沿 x 轴平移，你将为平移矩阵的 tx 分量提供一个非零值，并将 ty 和 tz 值保留为 0。对于旋转，你将提供适当的正弦和余弦值目标旋转角度。

&emsp;Figure 1-8  Matrix configurations for common transformations（常见转换（transformations）的矩阵配置）
![transform_manipulations](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/6ea3e5fd03fe436e85b606d08f34439d~tplv-k3u1fbpfcp-watermark.image)

&emsp;有关用于创建和操纵变换（transforms）的函数的信息，请参见 Core Animation Function Reference。
### Layer Trees Reflect Different Aspects of the Animation State（layer 树反映了动画状态的不同方面）
&emsp;使用 Core Animation 的应用程序具有三组 layer 对象。在使你的应用程序内容显示在屏幕上时，每组 layer 对象都有不同的作用：

+ 模型层树（model layer tree）（或简称 "layer tree"）中的对象是应用程序与之交互最多的对象。此树中的对象是存储任何动画的 target 值的模型对象。无论何时更改 layer 的属性，都会使用其中一个对象。
+ 表示树（presentation tree）中的对象包含任何正在运行的动画的动态值。尽管层树对象包含动画的目标值，但表示树中的对象在显示在屏幕上时会反映当前值。永远不要修改此树中的对象。相反，你可以使用这些对象读取当前动画值，或者从这些值开始创建新动画。
+ 渲染树（render tree）中的对象执行实际动画，并且是 Core Animation 专有。

&emsp;每一组 layer 对象都组织成一个层次结构，例如你的应用程序中的 views。实际上，对于为所有 view 启用 layer 的应用程序，每棵树的初始结构都与 view 层次结构完全匹配。但是，应用程序可以根据需要将其他 layer 对象（即，与 view 无关的 layer）添加到 layer 层次结构中。你可以在某些情况下执行此操作，以优化不需要 view 所有开销的内容的应用性能。图 1-9 显示了在简单的 iOS 应用中发现的 layer 的分解。示例中的 window 包含一个内容 view，该 view 本身包含一个按钮 view 和两个独立的 layer 对象。每个 view 都有一个对应的 layer 对象，该对象形成 layer 层次结构的一部分。

&emsp;Figure 1-9  Layers associated with a window（与窗口关联的 layer）

![sublayer_hierarchy](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/4f45ae85c10a4b4a813e76cafd2d6c74~tplv-k3u1fbpfcp-watermark.image)

&emsp;对于 layer 树中的每个对象，表示和渲染树（presentation and render trees）中都有一个匹配的对象，如图 1-10 所示。如前所述，应用程序主要与图层树（layer tree）中的对象一起使用，但有时可能会访问表示树（presentation tree）中的对象。具体来说，访问层树（layer tree）中对象的 presentationLayer 属性将返回表示树（presentation tree）中的相应对象。你可能要访问该对象以读取动画中间属性的当前值。

&emsp;Figure 1-10  The layer trees for a window（窗口的层树）

![sublayer_hierarchies](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/866c033240e447d89966ca1ba5a7c3f7~tplv-k3u1fbpfcp-watermark.image)

> &emsp;Important: 仅当动画正在运行时，才应访问表示树（presentation tree）中的对象。当动画正在进行时，表示树（presentation tree）包含在屏幕上显示的层值。此行为与层树（layer tree）不同，层树（layer tree）始终反映代码设置的最后一个值，并等效于动画的最终状态。
### The Relationship Between Layers and Views（图层与视图之间的关系）
&emsp;layer 不能替代应用程序的 view，也就是说，你不能仅基于 layer 对象创建可视界面。layer 为你的 view 提供基础结构。具体而言，layer 使绘制和动画化 view 内容并使其保持动画状态并保持较高的帧速率更加轻松有效。但是，很多事情是 layer 无法做到的。layer 不处理事件、绘制内容、参与响应者链或执行许多其他操作。因此，每个应用程序仍必须具有一个或多个 view 来处理这些类型的交互。

&emsp;在 iOS 中，每个 view 都有相应的 layer 对象支持，但在 OS X 中，你必须确定哪些 view 应具有 layer。在 OS X v10.8 及更高版本中，将 layer 添加到所有 view 可能是有意义的。但是，你不需要这样做，并且在不需要和不需要开销的情况下仍可以禁用 layer。分层（layers do increase）确实会增加你的应用程序的内存开销，但是它们的好处通常大于缺点，因此，最好在禁用 layer 支持之前测试应用程序的性能。

&emsp;为 view 启用 layer 支持（layer support）时，将创建称为 layer 支持（layer-backed ）的 view。在层支持的 view 中，系统负责创建基础 layer 对象，并使该 layer 与 view 保持同步。所有 iOS 视图都是基于 layer 的，并且 OS X 中的大多数 view 也是。但是，在 OS X 中，你还可以创建一个图层托管视图（layer-hosting view），该 view 是你自己提供 layer 对象的 view。对于图层托管视图（layer-hosting view），AppKit 在管理 layer 时采取了一种不干涉的方法，并且不会根据 view 的更改对其进行修改。

> &emsp;Note: 对于支持 layer 的 view，建议你尽可能操作 view 而不是其 layer。在 iOS 中，view 只是 layer 对象的薄薄包装，因此你对 layer 进行的任何操作通常都可以正常工作。但是在 iOS 和 OS X 中都存在这样的情况，即操纵 layer 而不是  view 可能无法产生所需的结果。本文档将尽可能指出这些陷阱并尝试提供方法来帮助你解决它们。

&emsp;除了与 view 关联的 layer 之外，你还可以创建没有对应 view 的 layer 对象。你可以将这些独立的 layer 对象嵌入到应用程序中的任何其他 layer 对象中，包括与 view 关联的对象。通常，你将独立的 layer 对象用作特定优化路径的一部分。例如，如果要在多个位置使用同一图像，则可以加载一次图像并将其与多个独立的 layer 对象关联，然后将这些对象添加到 layer tree 中。然后，每个 layer 都引用源映像，而不是尝试在内存中创建该映像的自己的副本。

&emsp;有关如何为应用程序 view 启用 layer 支持的信息，请参阅 Enabling Core Animation Support in Your App。有关如何创建 layer 对象层次结构的信息，以及有关何时进行创建的提示，请参阅 Building a Layer Hierarchy。

## Setting Up Layer Objects（设置图层对象）
&emsp;layer 对象是你使用 Core Animation 所做的一切的核心。layer 管理你应用的视觉内容，并提供用于修改该内容的样式和视觉外观的选项。尽管 iOS 应用已自动启用了 layer 支持，但 OS X 应用的开发人员必须先显式启用它，然后才能利用性能优势。启用后，你需要了解如何配置和操作应用程序的 layer 以获得所需的效果。
### Enabling Core Animation Support in Your App（在你的应用中启用 Core Animation 支持）
&emsp;在 iOS 应用中，始终启用 Core Animation，并且每个 view 都有一个 layer 支持。在 OS X 中，应用程序必须通过执行以下操作显式启用 Core Animation 支持：
+ Link against the QuartzCore framework。（iOS 应用程序必须在明确使用 Core Animation 接口的情况下，才引用此框架。）
+ 通过执行以下操作之一，为一个或多个 NSView 对象启用 layer 支持：
  + 在你的 nib 文件中，使用 View Effects inspector 为 view 启用 layer 支持。检查器显示所选 view 及其子  view 的复选框。建议你尽可能在 window 的内容 view 中启用 layer 支持。
  + 对于你以代码的方式创建的 view，请调用 view 的 `setWantsLayer:` 方法并传递 YES 值，以指示该 view 应使用 layer。

&emsp;以前述方式之一启用 layer 支持会创建一个 layer 支持的 view。使用 layer 支持的 view，系统负责创建基础 layer 对象并保持该 layer 的更新。在 OS X 中，还可以创建一个图层托管视图（layer-hosting view），从而使你的应用程序实际创建和管理底层 layer 对象。 （你不能在 iOS 中创建图层托管视图。）有关如何创建图层托管视图的更多信息，请参见 Layer Hosting Lets You Change the Layer Object in OS X。
### Changing the Layer Object Associated with a View（更改与视图关联的图层对象）
&emsp;默认情况下，layer 支持 view 创建 CALayer 类的实例，在大多数情况下，你可能不需要不同类型的 layer 对象。但是，Core Animation 提供了不同的 layer 类，每个 layer 类都提供了你可能会发现有用的专门功能。选择不同的 layer 类可以使你以简单的方式提高性能或支持特定类型的内容。例如，CATiledLayer 类经过优化，可以高效地显示大型图像。
#### Changing the Layer Class Used by UIView（更改 UIView 使用的图层类）
&emsp;你可以通过重写 iOS 的 `layerClass` 方法并返回其他类对象来更改 iOS view 使用的 layer 类型。大多数 iOS view 都会创建 CALayer 对象，并将该 layer 用作其内容的备份存储。对于你自己的大多数 view，此默认选择是一个不错的选择，你无需更改它。但是你可能会发现在某些情况下使用不同的 layer 类更为合适。例如，在以下情况下，你可能想要更改 layer 类：

+ 你的 view 使用 Metal 或 OpenGL ES 绘制内容，在这种情况下，你将使用 CAMetalLayer 或 CAEAGLLayer 对象。
+ 有一个专门的 layer 类，可以提供更好的性能。
+ 你想利用某些特殊的 Core Animation layer 类，例如粒子发射器（particle emitters）或复制器。

&emsp;更改 view 的 layer 类非常简单；清单2-1 中显示了一个示例。你所要做的就是重写 `layerClass` 方法，并返回要使用的类对象。在显示之前，view 将调用 `layerClass` 方法并使用返回的类为其自身创建一个新的 layer 对象。创建后，view 的 layer 对象无法更改。

&emsp;Listing 2-1  Specifying the layer class of an iOS view（指定 iOS 视图的图层类）
```c++
+ (Class) layerClass {
   return [CAMetalLayer class];
}
```
&emsp;有关 layer 类及其使用方式的列表，请参见 Different Layer Classes Provide Specialized Behaviors。
#### Layer Hosting Lets You Change the Layer Object in OS X（图层托管使你可以在 OS X 中更改图层对象）
&emsp;图层托管视图（layer-hosting view）是你自己创建和管理基础 layer 对象的 NSView 对象。在想要控制与 view 关联的 layer 对象的类型的情况下，可以使用图层托管。例如，你可以创建一个图层托管视图，以便可以分配默认 CALayer 类以外的其他 layer 类。如果要使用单个 view 来管理独立 layer 的层次结构，也可以使用它。

&emsp;当你调用 view 的 `setLayer:` 方法并提供一个 layer 对象时，AppKit 会采用一种不使用该 layer 的方法。通常，AppKit 会更新 view 的 layer 对象，但是在图层托管情况下，它并不能用于大多数属性。

&emsp;要创建图层托管视图，请创建 layer 对象并将其与 view 关联，然后在屏幕上显示该 view，如清单 2-2 所示。除了设置 layer 对象之外，你还必须调用 `setWantsLayer:` 方法以使 view 知道它应该使用 layer。

&emsp;Listing 2-2  Creating a layer-hosting view（创建图层托管视图）
```c++
// Create myView...
 
[myView setWantsLayer:YES];
CATiledLayer* hostedLayer = [CATiledLayer layer];
[myView setLayer:hostedLayer];
 
// Add myView to a view hierarchy.
```
&emsp;如果选择自己托管图层，则必须自行设置 contentsScale 属性，并在适当的时候 provide high-resolution content。有关 high-resolution content 和比例因子（scale factors）的更多信息，请参见 Working with High-Resolution Images。
#### Different Layer Classes Provide Specialized Behaviors（不同的层类别提供特殊的行为）
&emsp;Core Animation 定义了许多标准 layer 类，每种标准 layer 类都是针对特定用例设计的。 CALayer 类是所有 layer 对象的根类。它定义了所有 layer 对象必须支持的行为，并且是 layer 支持的 view 使用的默认类型。但是，你也可以在表 2-1 中指定一个 layer 类。

&emsp;Table 2-1  CALayer subclasses and their uses（CALayer 子类及其用途）

| Class | Usage |
| --- | --- |
| CAEmitterLayer | 用于实现基于 Core Animation 的粒子发射器系统。发射器层对象控制粒子的生成及其来源。 |
| CAGradientLayer | 用于绘制填充图层形状的颜色渐变（在任何圆角的范围内）。 |
| CAMetalLayer | 用于设置和 vend 可绘制纹理，以使用 Metal 渲染图层内容。 |
| CAEAGLLayer/CAOpenGLLayer | 用于设置备份存储和上下文，以使用 OpenGL ES（iOS）或 OpenGL（OS X）渲染图层内容。 |
| CAReplicatorLayer | 当你要自动制作一个或多个子层的副本时使用。复制器为你制作副本，并使用你指定的属性来更改副本的 appearance 或 attributes。 |
| CAScrollLayer | 用于管理由多个子层组成的较大的可滚动区域。 |
| CAShapeLayer | 用于绘制三次贝塞尔曲线样条曲线。Shape 图层对于绘制基于路径的形状非常有利，因为它们始终会产生清晰的路径，而与你绘制到图层的备份存储中的路径相反，后者在缩放时看起来并不好。但是，清晰的结果确实涉及在主线程上渲染 Shape 并缓存结果。 |
| CATextLayer | 用于呈现纯文本字符串或属性字符串。 |
| CATiledLayer | 用于管理可分为较小图块的大图像，并支持放大和缩小内容，分别渲染。 |
| CATransformLayer | 用于渲染真实的 3D 图层层次结构，而不是由其他图层类实现的平坦的图层层次结构。 |
| QCCompositionLayer | 用于渲染 Quartz Composer 合成。（仅支持 OS X） |

### Providing a Layer’s Contents（提供图层的内容）
&emsp;layer 是管理你应用程序提供的内容的数据对象。layer 的内容由包含要显示的视觉数据的位图组成。你可以通过以下三种方式之一提供该位图的内容：

+ 将图像对象直接指定给 layer 对象的 contents 属性。（此技术最适用于从不更改或很少更改的 layer 内容。）
+ 将 delegate 对象指定给 layer，并让 delegate 绘制 layer 的内容。（此技术最适合于可能定期更改并且可以由外部对象（如 view）提供的 layer 内容。）
+ 定义一个 layer 子类并重写它的一个绘图方法来自己提供 layer 内容。（如果必须创建自定义 layer 子类，或者要更改 layer 的基本绘图行为，则此技术是合适的。）

&emsp;唯一需要担心为 layer 提供内容的时间是你自己创建 layer 对象时。如果你的应用仅包含支持 layer 的 view，则无需担心使用任何前述技术来提供 layer 内容。支持 layer 的 view 会自动以最有效的方式为其关联 layer 提供内容。
#### Using an Image for the Layer’s Content（使用图像作为图层的内容）
&emsp;由于 layer 只是用于管理位图图像的容器，因此你可以将图像直接分配给 layer 的 contents 属性。将图像分配给 layer 很容易，并且可以指定要在屏幕上显示的确切图像。layer 使用你直接提供的图像对象，并且不尝试创建该图像的自己的副本。如果你的应用在多个地方使用相同的图像，此行为可以节省内存。

&emsp;你分配给 layer 的图像必须是 CGImageRef 类型。 （在 OS X v10.6 及更高版本中，你还可以分配一个 NSImage 对象。）分配图像时，请记住提供一个分辨率与本机设备的分辨率匹配的图像。对于具有 Retina 显示屏的设备，这可能还需要你调整图像的 contentsScale 属性。有关在图层上使用高分辨率内容的信息，请参阅 Working with High-Resolution Images。
#### Using a Delegate to Provide the Layer’s Content（使用委托提供图层内容）
&emsp;如果 layer 的内容动态变化，则可以使用 delegate 对象在需要时提供和更新该内容。在显示时，该 layer 调用 delegate 的方法以提供所需的内容：
+ 如果 delegate 实现 `displayLayer:` 方法，则该实现负责创建位图并将其分配给 layer 的 contents 属性。
+ 如果你的 delegate 执行 `drawLayer:inContext:` 方法，Core Animation 创建位图，创建要绘制到该位图中的图形上下文，然后调用 delegate 方法来填充位图。你的 delegate 方法所要做的就是绘制到提供的图形上下文中。

&emsp;delegate 对象必须实现 `displayLayer:` 或 `drawLayer:inContext:` 方法。如果 delegate 同时实现 `displayLayer:` 和 `drawLayer:inContext:` 方法，则 layer 仅调用 `displayLayer:` 方法。

&emsp;重写 `displayLayer:` 方法最适合你的应用喜欢加载或创建要显示的位图的情况。清单2-3 显示了 `displayLayer:` 委托方法的示例实现。在此示例中，委托使用帮助器对象加载并显示所需的图像。委托方法根据其自身的内部状态选择要显示的图像，在示例中，该状态是名为 `displayYesImage` 的自定义属性。

&emsp;Listing 2-3  Setting the layer contents directly（直接设置图层内容）
```c++
- (void)displayLayer:(CALayer *)theLayer {
    // Check the value of some state property
    if (self.displayYesImage) {
        // Display the Yes image
        theLayer.contents = [someHelperObject loadStateYesImage];
    } else {
        // Display the No image
        theLayer.contents = [someHelperObject loadStateNoImage];
    }
}
```
&emsp;如果你没有预渲染的图像或帮助对象来为你创建位图，则 delegate 可以使用 `drawLayer:inContext:` 方法动态绘制内容。清单2-4 显示了 `drawLayer:inContext:` 方法的示例实现。在此示例中，delegate 使用固定宽度和当前渲染颜色绘制一条简单的弯曲路径。

&emsp;Listing 2-4  Drawing the contents of a layer（绘制图层内容）
```c++
- (void)drawLayer:(CALayer *)theLayer inContext:(CGContextRef)theContext {
    // Create path
    CGMutablePathRef thePath = CGPathCreateMutable();
 
    CGPathMoveToPoint(thePath,NULL,15.0f,15.f);
    CGPathAddCurveToPoint(thePath,
                          NULL,
                          15.f,250.0f,
                          295.0f,250.0f,
                          295.0f,15.0f);
 
    CGContextBeginPath(theContext);
    CGContextAddPath(theContext, thePath);
 
    CGContextSetLineWidth(theContext, 5);
    CGContextStrokePath(theContext);
 
    // Release the path
    CFRelease(thePath);
}
```
&emsp;对于具有自定义内容的支持 layer 的 view，你应该继续重写该 view 的方法来进行绘制。layer 支持的 view 自动使自己成为其 layer 的 delegate 并实现所需的委托方法，并且你不应更改该配置。相反，你应该实现 view 的 `drawRect:` 方法来绘制内容。

&emsp;在 OS X v10.8 及更高版本中，绘图的另一种方法是通过重写 view 的 `wantUpdateLayer` 和 `updateLayer` 方法来提供位图。重写 `wantUpdateLayer` 并返回 YES 会使 NSView 类遵循替代的呈现路径。view 不调用 `drawRect:`，而是调用你的 `updateLayer` 方法，该方法的实现必须直接将位图分配给 layer 的 `content` 属性。这是 AppKit 希望你直接设置 view layer 对象的内容的一种情况。
#### Providing Layer Content Through Subclassing（通过子类提供层内容）
&emsp;如果仍然要实现自定义 layer 类，则可以重写 layer 类的绘制方法以进行任何绘制。layer 对象本身生成自定义内容并不常见，但是 layer 当然可以管理内容的显示。例如，CATiledLayer 类通过将大图像分成较小的图块来管理大图像，这些小图块可以单独管理和呈现。因为只有 layer 具有在任何给定时间需要渲染哪些图块的信息，所以它直接管理绘图行为。

&emsp;子类别化时，可以使用以下两种技术之一来绘制 layer 的内容：

+ 重写 layer 的 `display` 方法，并使用它直接设置 layer 的 contents 属性。
+ 重写 layer 的 `drawInContext:` 方法，并使用它绘制到提供的图形上下文中。

&emsp;重写哪种方法取决于对绘图过程所需的控制程度。`display` 方法是更新 layer 内容的主要入口点，因此重写该方法将使你完全控制该过程。重写 `display` 方法还意味着你要负责创建要分配给 contents 属性的 CGImageRef。如果你只想绘制内容（或让 layer 管理绘制操作），可以重写 `drawInContext:` 方法，让 layer 为你创建备份存储。
#### Tweaking the Content You Provide（调整你提供的内容）
&emsp;当你将图像分配给 layer 的 content 属性时，该 layer 的 contentsGravity 属性确定如何操作该图像以适合当前 bounds。默认情况下，如果图像大于或小于当前 bounds，则 layer 对象将缩放图像以适合可用空间。如果layer  bounds 的宽高比与图像的高宽比不同，则可能导致图像变形。你可以使用 contentsGravity 属性来确保以最佳方式呈现你的内容。

&emsp;可以分配给 contentsGravity 属性的值分为两类：

+ 基于位置（position-based）的重力常数允许你将图像固定到 layer bounds 矩形的特定边或角，而无需缩放图像。
+ 基于缩放（scaling-based）的重力常数允许你使用几个选项中的一个来拉伸图像，其中一些选项保留纵横比，而另一些则不保留纵横比。

&emsp;图2-1 显示了基于位置的重力设置如何影响图像。除了 kCAGravityCenter 常数外，每个常数都将图像固定在 layer bounds 矩形的特定边缘或角落。 kCAGravityCenter 常数使图像在 layer 中居中。这些常数都不会使图像以任何方式缩放，因此图像始终以其原始大小进行渲染。如果图像大于 layer bounds，则可能会导致图像的一部分被裁剪；如果图像较小，则该 layer 中未被图像覆盖的部分将显示该 layer 的背景色（如果设置）。

&emsp;Figure 2-1  Position-based gravity constants for layers（图层的基于位置的重力常数）

![layer_contentsgravity1](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/2e8413a42dd84fae9c4764a9a84e247c~tplv-k3u1fbpfcp-watermark.image)

&emsp;图2-2 显示了基于缩放的重力常数如何影响图像。如果所有这些常数都不完全适合 layer 的 bounds 矩形，则将缩放图像。两种模式之间的区别在于它们如何处理图像的原始长宽比。有些模式保留它，而另一些则不保留。默认情况下，图层的 contentsGravity 属性设置为 kCAGravityResize 常数，这是唯一不保留图像长宽比的模式。

&emsp;Figure 2-2  Scaling-based gravity constants for layers（基于缩放的图层重力常数）

![positioningmask](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/c91bcb27b3c34f238f6b282e47db89f7~tplv-k3u1fbpfcp-watermark.image)

#### Working with High-Resolution Images（使用高分辨率图像）
&emsp;layer 不具备基础设备屏幕分辨率的任何固有知识。layer 仅存储指向位图的指针，并在给定可用像素的情况下以最佳方式显示它。如果将图像分配给 layer 的 content 属性，则必须通过将 layer 的 contentsScale 属性设置为适当的值来告知 Core Animation 该图像的分辨率。该属性的默认值为 1.0，适用于打算在标准分辨率屏幕上显示的图像。如果你的图像打算用于 Retina 显示屏，请将此属性的值设置为 2.0。

&emsp;仅当你直接向 layer 分配位图时，才需要更改 contentsScale 属性的值。 UIKit 和 AppKit 中基于 layer 的 view 会根据屏幕分辨率和该 view 管理的内容，自动将其 layer 的比例因子设置为适当的值。例如，如果你将 NSImage 对象分配给 OS X 中 layer 的 contents 属性，则 AppKit 会查看该图像是否同时具有标准和高分辨率版本。如果存在，则 AppKit 会为当前分辨率使用正确的变体，并将 contentsScale 属性的值设置为匹配。

&emsp;在 OS X 中，基于位置的重力常数会影响从分配给 layer 的 NSImage 对象中选择图像表示的方式。由于这些常数不会导致图像缩放，因此 Core Animation 依靠 contentsScale 属性来选择具有最合适像素密度的图像表示形式。

&emsp;在 OS X 中，layer 的委托可以实现 `layer:shouldInheritContentsScale:fromWindow` 方法并使用它来响应比例因子的变化。每当给定 window 的分辨率发生变化时，AppKit 都会自动调用该方法，这可能是因为 window 在标准分辨率和高分辨率屏幕之间移动。如果委托支持更改 layer 图像的分辨率，则此方法的实现应返回 YES。然后，该方法应根据需要更新 layer 的内容，以反映新的分辨率。
### Adjusting a Layer’s Visual Style and Appearance（调整图层的视觉样式和外观）
&emsp;layer 对象具有内置的视觉装饰（visual adornments），例如边框（border）和背景色，可以用来补充 layer 的主要内容。因为这些视觉装饰不需要任何渲染，所以在某些情况下可以将 layer 用作独立实体。你所要做的就是在 layer 上设置一个属性，layer 处理必要的绘图，包括任何动画。有关这些视觉装饰如何影响 layer 外观的其他说明，请参见 Layer Style Property Animations。
#### Layers Have Their Own Background and Border（图层具有自己的背景和边界）
&emsp;除基于图像的内容外，layer 还可以显示填充的背景和描边边框。如图 2-3 所示，背景色呈现在 layer 内容图像的后面，边框呈现在该图像的顶部。如果 layer 包含子 layer，它们也将显示在边框下方。因为背景色位于图像的后面，所以该颜色会穿透图像的任何透明部分。

&emsp;Figure 2-3  Adding a border and background to a layer（在图层上添加边框和背景）

![layer_border_background](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/51f9cac58e564c35b2fa4e2590aa93c0~tplv-k3u1fbpfcp-watermark.image)

&emsp;清单2-5 显示了为 layer 设置背景色和边框所需的代码。所有这些属性都可以设置动画。

&emsp;Listing 2-5  Setting the background color and border of a layer（设置图层的背景色和边框）
```c++
myLayer.backgroundColor = [NSColor greenColor].CGColor;
myLayer.borderColor = [NSColor blackColor].CGColor;
myLayer.borderWidth = 3.0;
```
> &emsp;Note: 可以为 layer 的背景使用任何类型的颜色，包括具有透明度或使用图案图像的颜色。不过，在使用模式图像时，请注意 Core Graphics 处理模式图像的渲染，并使用其标准坐标系进行处理，这与 iOS 中的默认坐标系不同。因此，在 iOS 上渲染的图像在默认情况下会显示为上下颠倒，除非你翻转坐标。

&emsp;如果将 layer 的背景色设置为不透明颜色，请考虑将 layer 的 opaque 属性设置为 YES。这样做可以提高在屏幕上合成 layer 时的性能，并且不需要 layer 的备份存储来管理 alpha 通道。但是，如果 layer 的 corner radius 不为零，则不能将其标记为不透明。
#### Layers Support a Corner Radius（图层支持圆角半径）
&emsp;你可以通过为 layer 添加圆角半径来为其创建圆角矩形效果。圆角半径是一种视觉装饰，可遮盖 layer bounds 矩形的部分拐角，以使基础内容得以显示，如图 2-4 所示。由于涉及到应用透明蒙版，因此除非将 masksToBounds 属性设置为 YES，否则圆角半径不会影响图层的 content 属性中的图像。但是，圆角半径始终会影响 layer 背景颜色和边框的绘制方式。

&emsp;Figure 2-4  A corner radius on a layer（图层的圆角半径）

![layer_corner_radius](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/6b56cbf027bb4b2983edc495c3e2d3fe~tplv-k3u1fbpfcp-watermark.image)

&emsp;若要将圆角半径应用于 layer，请为 layer 的 cornerRadius 属性指定一个值。指定的半径值以点为单位测量，并在显示之前应用于 layer 的所有四个角。
#### Layers Support Built-In Shadows（图层支持内置阴影）
&emsp;CALayer 类包含几个用于配置阴影效果的属性。阴影使 layer 看起来像是漂浮在其基础内容之上，从而增加了 layer 的深度。这是另一种视觉装饰，你可能会发现它在特定情况下对你的应用有用。使用 layer，你可以控制阴影的颜色，相对于 layer 内容的位置、不透明度和形状。

&emsp;默认情况下，layer 阴影的不透明度（opacity）值设置为 0，可有效隐藏阴影。将不透明度更改为非零值会导致 Core Animation 绘制阴影。由于阴影默认情况下位于 layer 的正下方，因此你可能还需要更改阴影的偏移量才能看到它。不过请务必记住，为阴影指定的偏移量是使用 layer 的本地坐标系应用的，这在 iOS 和 OS X 上是不同的。图 2-5 显示了一个阴影向下延伸到 layer 的右边。在 iOS 中，这要求为 y 轴指定一个正值，但在 OS X 中，该值必须为负。

&emsp;Figure 2-5  Applying a shadow to a layer（将阴影应用于图层）

![layer_shadows](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/4c8c23288a134dc0905e1228d65195e4~tplv-k3u1fbpfcp-watermark.image)

&emsp;将阴影添加到 layer 时，阴影是 layer 内容的一部分，但实际上延伸到 layer 的 bounds 矩形之外。因此，如果为 layer 启用 masksToBounds 属性，则阴影效果将被修剪到边缘周围。如果你的 layer 包含任何透明内容，这可能会导致奇怪的效果，即阴影的直接在 layer 下面的部分仍然可见，而超出 layer 的部分则不可见。如果要阴影但也要使用 bounds masking，请使用两层而不是一层。将蒙版应用于包含你的内容的 layer，然后将该 layer 嵌入与启用阴影效果的大小完全相同的第二图层中。

&emsp;有关如何将阴影应用于 layer 的示例，请参见 Shadow Properties。
#### Filters Add Visual Effects to OS X Views（过滤器将视觉效果添加到 OS X 视图）
&emsp;在 OS X 应用中，你可以将 Core Image 滤镜直接应用于 layer 的内容。你可以这样做来模糊或锐化 layer 的内容，更改颜色，扭曲内容或执行许多其他类型的操作。例如，图像处理程序可能会使用这些滤镜无损地修改图像，而视频编辑程序可能会使用它们来实现不同类型的视频过渡效果。而且，由于过滤器是通过硬件应用到 layer 内容的，因此渲染既快速又流畅。

> &emsp;Note: 您无法将过滤器添加到 iOS 中的 layer 对象。

&emsp;对于给定的 layer，你可以将过滤器应用于该 layer 的前景和背景内容。前景内容包括 layer 本身包含的所有内容，包括其 contents 属性中的图像，其背景颜色，其边框以及其子 layer 的内容。背景内容是直接在 layer 下面但实际上不是 layer 本身一部分的内容。大多数 layer 的背景内容是其直接上层的内容，该内容可能被该层完全或部分遮盖。例如，当你希望用户专注于 layer 的前景内容时，可以对背景内容应用模糊滤镜。

&emsp;你可以通过将 CIFilter 对象添加到 layer 的以下属性来指定过滤器：

+ filters 属性包含仅影响 layer 前景内容的过滤器数组。
+ backgroundFilters 属性包含一组过滤器，这些过滤器仅影响 layer 的背景内容。
+ compositingFilter 属性定义 layer 的前景和背景内容如何组合在一起。

&emsp;要将过滤器添加到 layer ，必须先找到并创建 CIFilter 对象，然后对其进行配置，然后再将其添加到 layer。 CIFilter 类包括几个用于查找可用的 Core Image 过滤器的类方法，例如 `filterWithName:` 方法。不过，创建过滤器只是第一步。许多滤镜具有定义滤镜如何修改图像的参数。例如，盒子模糊滤镜的输入半径参数会影响所应用的模糊量。在过滤器配置过程中，应始终为这些参数提供值。但是，不需要指定的一个常见参数就是输入图像，它由 layer 本身提供。

&emsp;将过滤器添加到 layer 时，最好在将过滤器添加到 layer 之前配置过滤器参数。这样做的主要原因是，一旦添加到 layer，就无法修改 CIFilter 对象本身。但是，你可以使用图层的 `setValue:forKeyPath:` 方法在事后更改过滤器值。

&emsp;清单2-6 显示了如何创建捏变形过滤器（滤镜）（a pinch distortion filter）并将其应用于 layer 对象。此滤镜向内捏住图层的源像素，使最接近指定中心点的那些像素失真。请注意，在该示例中，你无需指定滤镜的输入图像，因为该 layer 的图像会自动使用。

&emsp;Listing 2-6  Applying a filter to a layer（将滤镜应用于图层）
```c++
CIFilter* aFilter = [CIFilter filterWithName:@"CIPinchDistortion"];
[aFilter setValue:[NSNumber numberWithFloat:500.0] forKey:@"inputRadius"];
[aFilter setValue:[NSNumber numberWithFloat:1.25] forKey:@"inputScale"];
[aFilter setValue:[CIVector vectorWithX:250.0 Y:150.0] forKey:@"inputCenter"];
 
myLayer.filters = [NSArray arrayWithObject:aFilter];
```
&emsp;有关可用 Core Image 过滤器的信息，请参见 [Core Image Filter Reference](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Reference/CoreImageFilterReference/index.html#//apple_ref/doc/uid/TP40004346)。
### The Layer Redraw Policy for OS X Views Affects Performance（OS X 视图的层重绘策略影响性能）
&emsp;在 OS X 中，layer 支持的 view 支持几种不同的策略来确定何时更新底层的内容。由于 native AppKit 绘图模型与 Core Animation 引入的模型之间存在差异，因此这些策略使将旧代码迁移到 Core Animation 变得更加容易。你可以逐个视图地配置这些策略，以确保每个视图的最佳性能。






























## 参考链接
**参考链接:🔗**
+ [CALayer](https://developer.apple.com/documentation/quartzcore/calayer?language=objc)
+ [Core Animation Programming Guide](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/CoreAnimation_guide/Introduction/Introduction.html#//apple_ref/doc/uid/TP40004514-CH1-SW1)
