# iOS《View Programming Guide for iOS》官方文档

## About Windows and Views（关于 Windows 和 Views）
&emsp;在 iOS 中，你可以使用 Windows 和 views 在屏幕上显示应用程序的内容。 Windows 本身没有任何可见的内容，但是它为应用程序的 views 提供了基本的容器。Views 定义要在 window 的哪一部分填充什么内容。例如，你可能具有显示 images、text、shapes 或其某种组合的 views。你还可以使用 views 来组织和管理其他 views。
### At a Glance
&emsp;每个应用程序至少都有一个 window 和一个 view 来呈现其内容。 UIKit 和其他系统框架提供了可用于呈现内容的预定义 views。这些 views 的范围从简单的按钮和文本标签到更复杂的视图，例如表视图（table views）、选择器视图（picker views）和滚动视图（scroll views）。在预定义 views 无法满足你需求的地方，你还可以定义 custom views 并自己管理绘图（drawing）和事件处理（event handling）。
#### Views Manage Your Application’s Visual Content（Views 管理你的应用程序的视觉内容）
&emsp;view 是 UIView 类（或其子类之一）的实例，并管理应用程序 window 中的矩形区域。Views 负责绘制内容、处理多点触控事件以及管理任何 subviews 的布局。绘图涉及使用诸如 Core Graphics、OpenGL ES 或 UIKit 之类的图形技术在 view 的矩形区域内绘制形状、图像和文本。view 通过使用手势识别器或直接处理触摸事件来响应其矩形区域中的触摸事件。在 view 层次结构中，父视图负责定位和调整其子视图的大小，并且可以动态地这样做。动态修改子视图的功能使你的 views 可以适应不断变化的条件，例如界面旋转和动画。

&emsp;你可以将 views 视为用于构建用户界面的构建块（building blocks，或者构建基础）。你通常不使用一个 view 来呈现所有内容，而是使用多个 views 来构建 view 层次结构。层次结构中的每个 view 都呈现用户界面的特定部分，并且通常针对特定类型的内容进行了优化。例如，UIKit 具有专门用于呈现图像、文本和其他类型的内容的视图。

> Relevant Chapters: View and Window Architecture, Views

#### Windows Coordinate the Display of Your Views（Windows 协调 Views 的显示）
&emsp;window 是 UIWindow 类的实例，用于处理应用程序用户界面的整体外观。 Windows 使用 views（和 views 自己的视图控制器（view controllers））来管理与可见 view 层次结构的交互以及对可见 view 层次结构的更改。在大多数情况下，你的应用程序 window 永远不会改变。创建 window 后，它保持不变，只有它显示的 views 改变。每个应用程序都有至少一个 window，在设备的主屏幕上显示该应用程序的用户界面。如果将外接显示器连接到设备，则应用程序可以创建第二个 window 以在该屏幕上显示内容。

> Relevant Chapters: Windows

#### Animations Provide the User with Visible Feedback for Interface Changes（动画为用户提供界面更改的可见反馈）
&emsp;动画为用户提供了有关 view 层次结构更改的可见反馈。系统定义了标准动画，用于呈现模式视图（modal views）并在不同 views 组之间进行转换（transitioning）。但是，view 的许多属性也可以直接设置动画。例如，通过动画，你可以更改 view 的透明度、它在屏幕上的位置、它的大小、它的背景颜色或其他属性。而且，如果你直接使用 view 的基础 Core Animation 图层对象（CALayer），则还可以执行许多其他动画。

> Relevant Chapters: Animations

#### The Role of Interface Builder（Interface Builder 的作用）
&emsp;Interface Builder 是一个应用程序，可用于以图形方式构造和配置应用程序的 windows 和 views。使用 Interface Builder，你可以组装 views 并将其放置在 nib 文件中，该文件是存储 views 和其他对象的 freeze-dried version 的资源文件。当你在运行时加载 nib 文件时，其中的对象将重新构建为实际对象，你的代码随后可以通过程序对其进行操作。

&emsp;Interface Builder 大大简化了创建应用程序的用户界面时需要做的工作。由于整个 iOS 都集成了对 Interface Builder 和 nib 文件的支持，因此只需很少的努力即可将 nib 文件整合到应用程序的设计中。

&emsp;有关如何使用 Interface Builder 的更多信息，请参见 Interface Builder User Guide。有关 view controllers 如何管理包含其 views 的 nib 文件的信息，请参见 View Controller Programming Guide for iOS 中的 Creating Custom Content View Controllers。

### See Also
&emsp;由于 views 是非常复杂和灵活的对象，因此不可能将所有行为都包含在一个文档中。但是，还有其他文档可帮助你了解管理 views 和整个用户界面的其他方面。
+ View controllers 是管理应用程序 views 的重要部分。view controller 管理单个 views 层次结构中的所有 views，并有助于在屏幕上呈现这些 views。有关 view controllers 及其角色的更多信息，请参见 View Controller Programming Guide for iOS。
+ Views 是应用程序中手势和触摸事件的主要接收者。有关使用手势识别器和直接处理触摸事件的更多信息，请参见 Event Handling Guide for iOS。
+ 自定义 views 必须使用可用的绘图技术来呈现其内容。有关使用这些技术在 views 中进行绘制的信息，请参见 Drawing and Printing Guide for iOS。
+ 在标准 view 动画不足的地方，可以使用 Core Animation。有关使用 Core Animation 实现动画的信息，请参见 Core Animation Programming Guide。

## View and Window Architecture（View 和 Window 架构）
&emsp;Views 和 windows 显示应用程序的用户界面，并处理与该界面的交互。 UIKit 和其他系统框架提供了许多 views，你几乎可以不加修改就可以使用它们。你还可以为需要不同于标准 views 显示内容的地方定义自定义 views。

&emsp;无论是使用系统 views 还是创建自己的自定义 views，都需要了解 UIView 和 UIWindow 类提供的基础结构。这些类为管理 views 的布局和表示提供了复杂的工具。了解这些工具是如何工作的对于确保在应用程序中发生更改时 views 的行为是非常重要的。

### View Architecture Fundamentals（View 架构基础）
&emsp;你可能希望在视觉上做的大多数事情都是使用 view 对象（UIView 类的实例）完成的。view 对象在屏幕上定义一个矩形区域，并处理该区域中的绘图和触摸事件。view 还可以充当其他 views 的父级，并协调这些 views 的位置和大小调整。 UIView 类完成了管理 views 之间的这些关系的大部分工作，但是你也可以根据需要自定义默认行为。

&emsp;Views 与 Core Animation layers 结合使用，以处理 view 内容的渲染和动画处理。 UIKit 中的每个 view 都由一个 layer 对象（通常是 CALayer 类的实例）支持，该对象管理该 view 的 backing store 并处理与 view 相关的动画。大多数操作都应该通过 UIView 接口执行。但是，在需要更好地控制 views 的渲染或动画行为的情况下，可以改为通过其 layer 执行操作。

&emsp;为了理解 views 和 layers 之间的关系，请看一个示例。图 1-1 显示了 ViewTransitions 示例应用程序的 view 体系结构以及与底层 Core Animation layers 的关系。应用程序中的 views 包括一个 window（也是一个 view）、一个充当容器 view 的通用 UIView 对象、一个图像 view、一个用于显示控件的工具栏和一个条形按钮项（它不是 view 本身，而是在内部管理 view）（UIBarButtonItem -> UIBarItem -> NSObject）。（实际的 ViewTransitions 示例应用程序包括用于实现 transitions 的附加图像 view。为了简单起见，并且由于该 view 通常是隐藏的，因此不包括在图 1-1 中。）每个 view 都有一个相应的 layer 对象，可以从该 view 的 layer 属性访问该对象。（由于条形按钮项不是 view，因此无法直接访问其 layer。）这些 layer 对象后面是 Core Animation 渲染对象，最终是用于管理屏幕上实际位的硬件缓冲区。

&emsp;Figure 1-1  Architecture of the views in a sample application

![view-layer-store](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/2f630851553947f8afc9f025be6dedef~tplv-k3u1fbpfcp-watermark.image)

&emsp;Core Animation layer 对象的使用对性能有重要影响。view 对象的实际绘图代码被尽可能少地调用，当调用代码时，结果被 Core Animation 缓存，并在以后尽可能多地重用。重用已经呈现的内容消除了更新 views 通常需要的昂贵的绘图周期。在可以操纵现有内容的动画中，重用此内容尤其重要。这种重用比创建新内容要节省的多。
#### View Hierarchies and Subview Management（View 层次结构和 Subview 管理）
&emsp;除了提供自己的内容外，view 还可以充当其他 views 的容器。当一个 view 包含另一个 view  时，将在两个 views 之间创建父子关系。关系中的 child view 称为 subview，parent  view 称为 superview。建立这种类型的关系对应用程序的外观和应用程序的行为都有影响。

&emsp;在视觉上，subview 的内容会掩盖其 superview 的全部或部分内容。如果 subview 是完全不透明的，则 subview 所占据的区域将完全遮挡父视图的相应区域。如果 subview 是部分透明的，则将两个 views 中的内容混合在一起，然后再显示在屏幕上。每个 superview 都将其 subviews 存储在一个有序数组中，并且该数组中的顺序也会影响每个 subview 的可见性。如果两个同级 subview 彼此重叠，则最后添加的（或已移至 subview 数组末尾的一个）出现在另一个 subview 的顶部。

&emsp;superview 与 subview 的关系也会影响几种 view 行为。更改 superview 的大小会产生连锁反应，这也会导致任何 subviews 的大小和位置也发生变化。更改 superview 的大小时，可以通过适当配置 view 来控制每个 subview 的调整大小的行为。影响 subview 的其他更改包括隐藏 superview、更改 superview 的 alpha（透明度）或对 superview 的坐标系进行数学变换。

&emsp;view 层次结构中 views 的排列也决定了应用程序如何响应事件。当触摸发生在特定 view 中时，系统会将包含触摸信息的事件对象直接发送到该 view 进行处理。但是，如果 view 不处理特定的触摸事件，它可以将事件对象传递给它的 superview。如果 superview 不处理事件，它会将事件对象传递给它的 superview，依此类推响应者链（and so on up the responder chain）。特定 view 还可以将事件对象传递给中间的响应者对象，例如视图控制器。如果没有对象处理事件，它最终会到达应用程序对象，而应用程序对象通常会丢弃它。

&emsp;有关如何创建视图层次结构的更多信息，请参见 Creating and Managing a View Hierarchy。

#### The View Drawing Cycle（View 绘图周期）
&emsp;UIView 类使用按需绘制模型（an on-demand drawing model）来呈现内容。当 view 首次出现在屏幕上时，系统会要求它绘制其内容。系统捕获该内容的 snapshot，并将该 snapshot 用作 view 的视觉表示。如果你从不更改 view 的内容，则可能永远不会再次调用 view 的绘图代码。snapshot 图像可用于涉及 view 的大多数操作。如果确实更改了内容，则会通知系统 view 已更改。然后，views 重复绘制 view 的过程并捕获绘制结果的 snapshot。

&emsp;当 view 的 contents 更改时，你不会直接重绘这些更改。而是使用 `- setNeedsDisplay` 或 `- setNeedsDisplayInRect:` 方法使 view 被标记为无效。这些方法告诉系统， view 的内容已更改，需要在下一个时机重新绘制。在启动任何重新绘制操作之前，系统将一直等到当前 run loop 结束。此延迟使你有机会一次使多个 view 无效，从层次结构中添加或删除 views、隐藏 views、调整 views 大小以及重新放置 views。你所做的所有更改都将同时在下一次绘制结果中反映出来。（即我们可以把一组对 views 的不同操作放在一起，然后在下次绘制时统一进行绘制，而不是说对 views 进行一步操作就绘制一次）

> Note: 更改 view 的几何形状不会自动导致系统重新绘制 view 的内容。view 的 contentMode 属性确定如何解释对 view 几何的更改。大多数 content modes 会在 view 范围内拉伸或重新定位现有 snapshot，而不创建新 snapshot。有关 content modes 如何影响 view 的绘制周期的更多信息，请参见 Content Modes。

&emsp;当需要渲染 view 内容时，实际的绘制过程会根据 view 及其配置而有所不同。系统 view 通常实现私有绘制方法来呈现其内容。这些相同的系统 views 经常公开可用于配置 view 实际外观的接口。对于自定义 UIView 子类，通常会重写 view 的 `- drawRect:` 方法，并使用该方法绘制 view 的内容。还有其他提供 view 内容的方法，例如直接设置 underlying layer 的 contents，但是重写 `- drawRect:` 方法是最常见的技术。

&emsp;有关如何为自定义 views 绘制内容的详细信息，请参见 Implementing Your Drawing Code。

#### Content Modes（内容模式）

&emsp;每个 view 都有一个 content mode，用于控制 view 如何响应 view geometry 中的更改而回收其内容，以及是否完全回收其内容。首次显示 view 时，它会照常渲染其内容，并将结果捕获在基础位图（underlying bitmap）中。之后，对 view 几何形状的更改并不总是导致重新创建位图。而是，contentMode 属性中的值确定是应缩放位图以适合新的 bounds 还是应将其简单固定到 view 的一个角或边缘。

&emsp;只要执行以下操作，便会应用视图的 content mode：（而不是进行重绘，那么会存在根据 content mode 进行调整位图并进行重绘吗？是存在的， 设置为 UIViewContentModeRedraw 模式的情况）

+ 更改 view frame 或 bounds 矩形的宽度或高度。
+ 将包含缩放因子（scaling factor）的变换（transform）指定给视图的 transform 属性。

&emsp;默认情况下，大多数 views 的 contentMode 属性设置为 UIViewContentModeScaleToFill，这将导致 view 内容被缩放以适应新的 frame 大小。图 1-2 显示了某些可用内容模式下发生的结果。从图中可以看出，并不是所有的内容模式都会导致 view bounds 被完全填充，而那些内容模式可能会扭曲 view 的内容。

&emsp;Figure 1-2  Content mode comparisons

![scale_aspect](https://p1-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/baefe73213d042b1b56865c824855e3b~tplv-k3u1fbpfcp-watermark.image)

&emsp;内容模式有助于回收 view 的内容，但是当你特别希望 custom views 在缩放和调整大小操作期间重新绘制时，也可以将内容模式设置为 UIViewContentModeRedraw 值。将 view 的内容模式设置为该值将强制系统调用 view 的 `- drawRect:` 方法以响应几何体更改。一般来说，你应该尽可能避免使用这个值，当然也不应该在标准系统 views 中使用它。

&emsp;有关可用内容模式的详细信息，请参见 UIView Class Reference。

#### Stretchable Views（可伸缩的 Views）
&emsp;你可以将 view 的一部分指定为可拉伸的，这样，当 view 的大小更改时，仅可拉伸部分中的内容会受到影响。通常，将可拉伸区域用于按钮或其他 views，其中 view 的一部分定义了可重复的模式（repeatable pattern）。你指定的可拉伸区域可以允许沿 view 的一个或两个轴拉伸。当然，当沿两个轴拉伸 view 时，view 的边缘还必须定义可重复的模式，以避免任何变形。图 1-3 显示了这种变形如何在 view 中显现出来。每个 view 原始像素的颜色被复制以填充较大 view 中的相应区域。

&emsp;Figure 1-3  Stretching the background of a button

![button_scale](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/28aa264aafbc4ebfbd15f7e543571de8~tplv-k3u1fbpfcp-watermark.image)

&emsp;你可以使用 contentStretch 属性指定 view 的可拉伸区域。此属性接受一个矩形，其值规格化为 0.0 到 1.0 。拉伸 view 时，系统将这些归一化值乘以 view 的当前 bounds 和比例因子，以确定需要拉伸的像素。标准化值的使用减轻了你每次 view bounds 更改时都更新 contentStretch 属性的需求。

&emsp;view 的内容模式在确定如何使用 view 的可拉伸区域方面也发挥着作用。仅当内容模式会导致 view 内容缩放时，才使用可拉伸区域。这意味着只有 UIViewContentModeScaleToFill、UIViewContentModeScaleScaleAspectFit 和 UIViewContentModeScaleScaleAspectFill 内容模式才支持可拉伸 view。如果你指定一种将内容固定到边缘或角落的内容模式（因此实际上不会缩放内容），则 view 将忽略可拉伸区域。

> Note: 在为 view 指定背景时，建议在创建可拉伸的 UIImage 对象之前使用 contentStretch 属性。可拉伸 view 完全在 Core Animation layer 中处理，这通常提供更好的性能。

#### Built-In Animation Support（内置动画支持）
&emsp;在每个 view 后面都有一个 layer 对象的好处之一是，你可以轻松地对许多与 view 相关的更改进行动画处理。动画是一种向用户传达信息的有用方法，在设计应用程序时应始终考虑动画。 UIView 类的许多属性都是可设置动画的，也就是说，存在从一个值到另一个值进行动画制作的半自动支持。要为这些可设置动画的属性之一执行动画，你要做的就是：

1. 告诉 UIKit 你要执行动画。
2. 更改属性的值。

&emsp;你可以在 UIView 对象上设置动画的属性包括：

frame - 使用此动画为 view 设置位置和大小变化。（CALayer 类的 frame 不支持动画，可使用 bounds 和 position 属性来达到同样的效果）
bounds - 使用此动画可对 view 大小进行动画处理。
center - 使用它可以动画化 view 的位置。
transform - 使用它旋转或缩放 view。
alpha - 使用它可以更改 view 的透明度。
backgroundColor - 使用它可以更改 view 的背景色。
contentStretch - 使用它来更改 view 内容的拉伸方式。

&emsp;从一组 views 转换到另一组 views 时，动画非常重要。通常，使用 view controller 来管理与用户界面各部分之间的主要更改相关联的动画。例如，对于涉及从较高级别信息 navigating 到较低级别信息的界面，通常使用 navigation controller 来管理显示每个连续级别数据的 view 之间的转换。但是，也可以使用动画而不是 view controller 在两组 views 之间创建 transitions。在标准视图控制器动画（standard view-controller animations）不能产生所需结果的地方，可以这样做。

&emsp;除了使用 UIKit 类创建的动画外，还可以使用 Core Animation layers 创建动画。降到 layer 级别可以让你对动画的计时和属性有更多的控制。

&emsp;有关如何执行基于 view 的动画的详细信息，请参见 Animations。有关使用 Core Animation 创建动画的更多信息，请参见 Core Animation Programming Guide 和 Core Animation Cookbook。

### View Geometry and Coordinate Systems（查看几何和坐标系）
&emsp;UIKit 中的默认坐标系的原点在左上角，并且轴从原点向下延伸到右侧。坐标值使用浮点数表示，无论基础屏幕分辨率如何，都可以精确地布局和定位内容。图 1-4 显示了相对于屏幕的此坐标系。除了屏幕坐标系外，windows 和 views 还定义了自己的局部坐标系，使你可以指定相对于 view 或 window 原点而不是相对于屏幕的坐标。

&emsp;Figure 1-4  Coordinate system orientation in UIKit

![native_coordinate_system](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/49aaf614797946f9bc2e7466521a8ca2~tplv-k3u1fbpfcp-watermark.image)

&emsp;因为每个 view 和 window 都定义了自己的局部坐标系，所以你需要知道在任何给定时间有效的坐标系。每次绘制 view 或更改其几何形状时，都是相对于某个坐标系进行的。对于绘图，你可以指定相对于 view 自身坐标系的坐标。如果几何形状发生变化，则可以指定相对于 superview 坐标系的坐标。 UIWindow 和 UIView 类都包含帮助你从一个坐标系转换为另一个坐标系的方法。

> Important: 一些 iOS 技术定义的默认坐标系的原点和方向与 UIKit 使用的不同。例如，Core Graphics 和 OpenGL ES 使用一个坐标系，其原点位于 view 或 window 的左下角，其 y 轴相对于屏幕向上。在绘制或创建内容时，代码必须考虑这些差异，并根据需要调整坐标值（或坐标系的默认方向）。

#### The Relationship of the Frame, Bounds, and Center Properties（Frame、Bounds 和 Center 属性的关系）
&emsp;view 对象使用其 frame、bounds 和 center 属性跟踪其大小和位置：

+ frame 属性包含框 frame 矩形，用于指定 view 在其 superview 坐标系中的大小和位置。
+ bounds 属性包含 bounds 矩形，它指定 view 在自己的局部坐标系中的大小（及其内容原点）。
+ center 属性包含 superview 坐标系中 view 的已知中心点。

&emsp;你主要使用 center 和 frame 属性来操纵当前 view 的几何形状。例如，在构建 view 层次结构或在运行时更改 view 的位置或大小时，可以使用这些属性。如果仅更改 view 的位置（而不是其大小），则首选 center 属性。即使将缩放或旋转因子添加到 view 的变换中，center 属性中的值也始终有效。对于 frame 属性中的值，情况并非如此，如果 view 的 transform 不等于 identity transform，则该属性将被视为无效。

&emsp;你主要在绘制过程中使用 bounds 属性。bounds 矩形以 view 自身的局部坐标系表示。此矩形的默认原点为（0，0），其大小与 frame 矩形的大小匹配。你在此矩形内绘制的所有内容都是 view 可见内容的一部分。如果更改 bounds 矩形的原点，则在新矩形内绘制的所有内容都会成为 view 可见内容的一部分。

&emsp;图 1-5 显示了图像视图的 frame 和 bounds 矩形之间的关系。在该图中，图像 view 的左上角位于其 superview 坐标系中的点（40、40），矩形的大小为 240 x 380 点。对于 bounds 矩形，原点为（0，0），并且矩形的大小类似地为 240 x 380 点。

&emsp;Figure 1-5  Relationship between a view's frame and bounds

![frame_bounds_rects](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/3ccd1e9953d44905909b8c5cf40c973b~tplv-k3u1fbpfcp-watermark.image)

&emsp;尽管可以独立于其他属性更改 frame、bounds 和 center 属性，但是对一个属性的更改会通过以下方式影响其他属性：

+ 设置 frame 属性时，bounds 属性中的 size 值将更改，以匹配 frame 矩形的新大小。center 属性中的值也会发生类似的更改，以匹配 frame 矩形的新中心点。
+ 设置 center 属性时，frame 中的原点值会相应更改。
+ 设置 bounds 属性的大小时，frame 属性中的 size 值将更改，以匹配 bounds 矩形的新大小。

&emsp;默认情况下，view  的 frame 不会裁剪到其 superview 的 frame。因此，位于其 superview frame 之外的所有 subviews 均会完整呈现。不过，你可以通过将 superview 的 clipsToBounds 属性设置为 YES 来更改此行为。无论 subviews 是否在视觉上被剪切，触摸事件始终会遵循目标 view 的 superview 的 bounds 矩形。换言之，发生在 view 的某个部分（位于其 superview 的 bounds 矩形之外）中的触摸事件不会传递到该 view。

#### Coordinate System Transformations（坐标系转换）
&emsp;坐标系转换提供了一种快速，轻松地更改 view（或其 contents）的方法。仿射变换（affine transform）是一种数学矩阵，它指定一个坐标系中的点如何映射到另一个坐标系中的点。你可以将仿射变换应用于整个 view，以更改 view 相对于其 superview 的大小、位置或方向。你还可以在绘图代码中使用仿射变换来对单个渲染内容执行相同类型的操作。因此，如何应用仿射变换取决于上下文：

+ 要修改整个 view，请在 view 的 transform 属性中修改仿射变换。
+ 要修改 view 的 `- drawRect:` 方法中的特定内容片段，请修改与活动图形上下文关联的仿射变换。

&emsp;当你要实现动画时，通常可以修改 view 的 transform 属性。例如，你可以使用此属性来创建围绕其中心点旋转的 view 动画。你不会使用此属性对 view 进行永久更改，例如修改 view 的位置或在 view 的坐标空间内调整 view 的大小。对于这种类型的更改，你应该改为修改 view 的 frame 矩形。

> Note: 修改 view 的 transform 属性时，所有变换都相对于 view 的中心点执行。

&emsp;在 view 的 `- drawRect:` 方法中，使用仿射变换（affine transforms）来定位和定向要绘制的项。与其在 view 中的某个位置固定对象的位置，不如相对于固定点（通常为（0，0））创建每个对象，并在绘制之前使用 transform 来定位对象。这样，如果对象在 view 中的位置发生更改，则只需修改 transform 即可，这比在新位置重新创建对象要快得多，成本也要低得多。你可以使用 CGContextGetCTM 函数检索与图形上下文关联的仿射变换，并且可以使用相关的 Core Graphics 函数在绘图期间设置或修改此 transform。

&emsp;当前变换矩阵（transformation matrix）（CTM）是在任何给定时间使用的仿射变换。操纵整个 view  的几何图形时，CTM 是存储在 view  的 transform 属性中的仿射变换。在 `- drawRect:` 方法内部，CTM 是与活动图形上下文关联的仿射变换。

&emsp;每个 subview 的坐标系都基于其 superview 的坐标系。因此，当你修改 view 的 transform 属性时，该更改会影响该视图及其所有 subviews。但是，这些更改仅影响屏幕上 view 的最终呈现。由于每个 view 都绘制其内容并相对于其自己的 bounds 布置其 subviews，因此在绘制和布局期间，它可以忽略其 superview 的变换。

&emsp;图 1-6 演示了渲染时如何在视觉上结合两个不同的旋转因子。在视图的 `- drawRect:` 方法内部，对 shape 应用 45 度旋转因子会使该 shape 看起来旋转 45 度。对 view 应用单独的 45 度旋转因子，然后使 shape 看起来像旋转了 90 度。 shape 仍然相对于绘制它的 view 旋转了 45 度，但是 view 旋转使它看起来旋转了更多。

&emsp;Figure 1-6  Rotating a view and its content

![xform_rotations](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/6985a037f33d43989327b451d38abb45~tplv-k3u1fbpfcp-watermark.image)

> Important: 如果 view 的 transform 属性不是 identity 转换，则该 view 的 frame 属性的值未定义，必须忽略。将 transforms 应用于 view 时，必须使用 view 的 bounds 和 center 属性来获取 view 的大小和位置。任何子视图的 frame 矩形仍然有效，因为它们是相对于视图的 bounds。

&emsp;有关在运行时修改视图的 transform 属性的信息，请参见 Translating、Scaling and Rotating Views。有关如何在绘图过程中使用 transforms 来定位内容的信息，请参见 Drawing and Printing Guide for iOS。

#### Points Versus Pixels（Points 与像素）

&emsp;在 iOS 中，所有坐标值和距离都是使用浮点值指定的，这些浮点值的单位称为点（points）。一个点的可测量大小因设备而异，基本上不相关。了解点的主要内容是：它们为绘图提供了一个固定的参照系。

&emsp;表 1-1 列出了纵向不同类型的基于 iOS 的设备的屏幕尺寸（以磅为单位）。首先列出宽度尺寸，然后是屏幕的高度尺寸。只要你针对这些屏幕尺寸设计界面，你的 views 就会在相应类型的设备上正确显示。

&emsp;Table 1-1  Screen dimensions for iOS-based devices

| Device | Screen dimensions(in points) |
| --- | --- |
| iPhone and iPod touch devices with 4-inch Retina display | 320 x 568 |
| Other iPhone and iPod touch devices | 320 x 480 |
| iPad | 768 x 1024 |

&emsp;用于每种类型的设备的基于点的测量系统定义了所谓的用户坐标空间。这是几乎所有代码都使用的标准坐标空间。例如，在操纵 view 的几何体或调用 Core Graphics 函数绘制 view 的内容时，可以使用点和用户坐标空间。尽管有时用户坐标空间中的坐标会直接映射到设备屏幕上的像素，但你永远不要以为是这种情况。相反，你应该始终记住以下几点：

**一个点不一定对应于屏幕上的一个像素。（One point does not necessarily correspond to one pixel on the screen.）**

&emsp;在设备级别，view 中指定的所有坐标必须在某个点转换为像素。但是，用户坐标空间中的点到设备坐标空间中的像素的映射通常由系统处理。 UIKit 和 Core Graphics 都使用主要基于矢量（vector-based）的绘图模型，其中所有坐标值都是使用点指定的。因此，如果使用 Core Graphics 绘制曲线，则无论基础屏幕的分辨率如何，都可以使用相同的值指定曲线。

&emsp;当你需要使用图像或其他基于像素的技术（如 OpenGL ES）时，iOS 提供了管理这些像素的帮助。对于作为资源存储在应用程序包中的静态图像文件，iOS 定义了用于指定不同像素密度的图像以及加载与当前屏幕分辨率最匹配的图像的约定。Views 还提供有关当前比例因子的信息，以便可以手动调整任何基于像素的图形代码以适应更高分辨率的屏幕。有关在不同屏幕分辨率下处理基于像素的内容的技术，请参见 Supporting High-Resolution Screens In Views in Drawing and Printing Guide for iOS。

### The Runtime Interaction Model for Views（Views 的运行时交互模型）
&emsp;每当用户与你的用户界面进行交互，或者你自己的代码以编程方式更改某些内容时，UIKit 内都会发生一系列复杂的事件来处理该交互。在该序列中的特定时间点，UIKit 调出你的 view 类，并为其提供机会代表你的应用程序进行响应。了解这些标注点对于了解 views 在系统中的位置非常重要。图 1-7 显示了事件的基本顺序，该顺序从用户触摸屏幕开始，到图形系统作为响应更新屏幕内容，然后结束。对于任何以编程方式启动的操作，也将发生相同的事件序列。

&emsp;Figure 1-7  UIKit interactions with your view objects

![drawing_model](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/800911ec192b464c845cc288131c575f~tplv-k3u1fbpfcp-watermark.image)

&emsp;以下步骤进一步分解了图 1-7 中的事件序列，并解释了每个阶段发生的情况以及你可能希望应用程序如何响应。

1. 用户触摸屏幕。
2. 硬件将触摸事件报告给 UIKit 框架。
3. UIKit 框架将触摸打包到一个 UIEvent 对象中并将其分派到适当的 view。 （有关 UIKit 如何将事件传递到 view 的详细说明，请参见 Event Handling Guide for iOS。）
4. view 的事件处理代码响应事件。例如，你的代码可能：

  + 更改 view 或其 subviews 的属性（frame、bounds、alpha 等）。
  + 调用 `- setNeedsLayout` 方法将 view（或其 subviews）标记为需要布局更新。
  + 调用 `- setNeedsDisplay` 或 `- setNeedsDisplayInRect:` 方法将 view（或其 subviews）标记为需要重新绘制。
  + 通知 controller 某些数据的更改。
  当然，由你决定 view 应该做哪些事情以及应该调用哪些方法。
  
5. 如果 view 的几何图形因任何原因发生更改，UIKit 将根据以下规则更新其 subviews： 
  + 如果已经为 views 配置了自动调整大小规则（autoresizing rules），UIKit 会根据这些规则调整每个 view。有关自动调整大小规则如何工作的详细信息，请参见 Handling Layout Changes Automatically Using Autoresizing Rules。
  + 如果视图实现了 `- layoutSubviews` 方法，UIKit 将调用它。
    你可以在 custom views 中重写此方法，并使用它来调整任何 subviews 的位置和大小。例如，提供较大可滚动区域的 view 将需要使用多个 subviews 作为 "tiles"，而不是创建一个大 view，该大 view 无论如何都不太适合内存。在这个方法的实现中，view 将隐藏现在在屏幕外的任何 subviews，或者重新定位它们，并使用它们来绘制新公开的内容。作为此过程的一部分，view 的布局代码还可以使任何需要重新绘制的 view 无效。
    
6. 如果任何 view 的任何部分被标记为需要重绘，UIKit 会要求 view 重绘它自己。
  对于明确定义了 `- drawRect:` 方法的 custom views，UIKit 会调用该方法。此方法的实现应尽快重绘 view 的指定区域，而别无其他。此时，请勿进行其他布局更改，也不要对应用程序的数据模型进行其他更改。此方法的目的是更新 view 的视觉内容。
  标准系统 view 通常不实现 `- drawRect:` 方法，而是在此时管理其绘制。
  
7. 任何更新的 views 都将与应用程序的其余可见内容混合在一起，并发送到图形硬件进行显示。 
8. 图形硬件将渲染的内容传输到屏幕。

> Note: 先前的更新模型主要适用于使用标准系统 view 和绘图技术的应用程序。使用 OpenGL ES 进行绘制的应用程序通常会配置一个全屏 view 并直接绘制到关联的 OpenGL ES 图形上下文。在这种情况下，该 view 仍可以处理触摸事件，但是由于它是全屏的，因此无需布置 subviews。有关使用 OpenGL ES 的更多信息，请参见 OpenGL ES Programming Guide。

&emsp;在前面的步骤中，你自己的 custom views 的主要集成点是：

+ 事件处理方法：
  `touchesBegan:withEvent:`
  `touchesMoved:withEvent:`
  `touchesEnded:withEvent:`
  `touchesCancelled:withEvent:`

+ `layoutSubviews` 方法
+ `drawRect:` 方法

&emsp;这些是 views 最常用的重写方法，但你可能不需要重写所有这些方法。如果使用手势识别器来处理事件，则不需要重写任何事件处理方法。类似地，如果 view 不包含 subviews 或其大小没有更改，则没有理由重写 `layoutSubviews` 方法。最后，只有当 view 的内容在运行时可以更改，并且你正在使用 UIKit 或 Core Graphics 等 native 技术进行绘制时，才需要 `drawRect:` 方法。

&emsp;同样重要的是要记住，这些是主要的集成点，但不是唯一的集成点。UIView 类的几个方法被设计成子类的重写点。你应该查看  UIView Class Reference 中的方法描述，以查看哪些方法可能适合你在自定义实现中重写。

### Tips for Using Views Effectively（有效使用 Views 的提示）
&emsp;对于需要绘制标准系统 view 未提供的内容的情况，custom views 很有用，但是你有责任确保 views 的性能足够好。 UIKit 尽其所能来优化与 view 相关的行为，并帮助你在 custom views 中获得良好的性能。但是，你可以通过考虑以下提示在这方面帮助 UIKit。

> Important: 优化绘图代码之前，应始终收集有关 view 当前性能的数据。测量当前性能可以让你确认是否确实存在问题，如果存在，还可以为你提供基准测量结果，你可以将其与未来的优化进行比较。

#### Views Do Not Always Have a Corresponding View Controller（视图并不总是具有对应的视图控制器）
&emsp;应用程序中的各个 views 和 view controllers 之间很少存在一对一的关系。view controller 的工作是管理 view 层次结构，该 view 层次结构通常由多个 view 组成，这些 view 用于实现某些自包含功能。对于 iPhone 应用程序，每个 view 层次结构通常填充整个屏幕，尽管对于 iPad 应用程序，视图层次结构可能仅填充屏幕的一部分。

&emsp;在设计应用程序的用户界面时，必须考虑 view controllers 将扮演的角色。view controllers 提供了许多重要的行为，例如协调 views 在屏幕上的显示、协调从屏幕上删除这些 views、释放内存以响应内存不足警告以及旋转 views 以响应界面方向的更改。规避这些行为可能会导致应用程序行为不正确或出现意外情况。

&emsp;有关更多信息，请参见 view controllers 及其在应用程序中的角色，请参见 View Controller Programming Guide for iOS。

#### Minimize Custom Drawing（最小化自定义绘图）
&emsp;虽然自定义绘图有时是必要的，但也应尽可能避免。只有当现有的系统 view 类不提供所需的外观或功能时，才应该真正执行任何 custom drawing。每当你的内容可以与现有 view 的组合进行组合时，你最好将这些 view 对象组合到自定义 view 层次结构中。

#### Take Advantage of Content Modes（利用内容模式）
&emsp;内容模式可以最大限度地减少重新绘制 view 所花费的时间。默认情况下，view 使用 UIViewContentModeScaleToFill 内容模式，该模式缩放 view 的现有内容以适合 view 的 frame 矩形。你可以根据需要更改此模式以不同方式调整内容，但如果可以，应避免使用 UIViewContentModeRedraw 内容模式。无论使用哪种内容模式，都可以通过调用 `- setNeedsDisplay` 或 `- setNeedsDisplayInRect:`强制 view 重绘其内容。

#### Declare Views as Opaque Whenever Possible（尽可能将视图声明为不透明）
&emsp;UIKit 使用每个 view 的 opaque 属性来确定该 view 是否可以优化合成操作。对于 custom view，将此属性的值设置为 YES 可以告诉 UIKit，它不需要在 view 背后呈现任何内容。较少的渲染可以提高绘图代码的性能，因此通常会鼓励这样做。当然，如果将 opaque 属性设置为 YES，则 view 必须使用完全不透明的内容完全填充其 bounds 矩形。（如尽量使用 alpha 为 NO 的图片）

#### Adjust Your View’s Drawing Behavior When Scrolling（滚动时调整视图的绘图行为）
&emsp;滚动可以在短时间内产生大量 view 更新。如果 view 的绘图代码没有得到适当的调整，view 的滚动性能可能会很差。与其试图确保 view 的内容始终是原始的，不如考虑在滚动操作开始时更改 view 的行为。例如，可以临时降低呈现内容的质量，或者在滚动过程中更改内容模式。当滚动停止时，你可以将 view 返回到以前的状态，并根据需要更新内容。

#### Do Not Customize Controls by Embedding Subviews（不要通过嵌入子视图来自定义控件）
&emsp;尽管从技术上来说可以将 subviews 添加到标准系统控件（从 UIControl 继承的对象）中，但是你永远不应以这种方式对其进行自定义。支持自定义的控件通过控件类本身中的显式且文档齐全的接口来实现。例如，UIButton 类包含用于设置按钮标题和背景图像的方法。使用定义的定制点意味着你的代码将始终正常工作。通过在按钮内嵌入自定义图像视图或标签来规避这些方法，可能会导致你的应用程序现在或将来某个按钮的实现发生变化时，表现不正确。

## Windows
&emsp;每个 iOS 应用程序都至少需要一个 window（UIWindow 类的一个实例），有些可能包含多个 window。window 对象具有以下职责：

+ 它包含你应用程序的可见内容。
+ 它在将触摸事件传递给 view 和其他应用程序对象中扮演着关键角色。
+ 它与你的应用程序的 view controllers 一起使用，以方便方向更改。

&emsp;在 iOS 中，windows 没有 title bars、close boxes 或任何其他视觉装饰。window 始终只是一个或多个 view 的空白容器。此外，应用程序不会通过显示新 windows 来更改其内容。当你想要更改显示的内容时，可以更改 window 的最前面的 view。

&emsp;大多数 iOS 应用程序在其生命周期内仅创建和使用一个 window。该 window 跨越设备的整个主屏幕，并在应用程序生命周期的早期从应用程序的主 nib 文件加载（或通过程序创建）。但是，如果应用程序支持使用外部显示器进行视频输出，则它可以创建一个附加 window 以在该外部显示器上显示内容。所有其他 windows 通常由系统创建，并且通常是响应于特定事件（例如来电）而创建的。

### Tasks That Involve Windows（Windows 涉及的任务）
&emsp;对于许多应用程序，应用程序与其 window 交互的唯一时间是在启动时创建 window。但是，你可以使用应用程序的 window 对象执行一些与应用程序相关的任务：
+ **使用 window 对象将点和矩形与 window 的本地坐标系相互转换。** 例如，如果在 window 坐标中提供了一个值，则在尝试使用它之前，可能需要将其转换为特定 view 的坐标系。有关如何转换坐标的信息，请参见 Converting Coordinates in the View Hierarchy。
+ **使用 window 通知来跟踪与 window 相关的更改。** Windows 在显示或隐藏它们或接受或放弃 key 状态时会生成通知。你可以使用这些通知在应用程序的其他部分中执行操作。有关更多信息，请参见 Monitoring Window Changes。

### Creating and Configuring a Window（创建和配置窗口）
&emsp;你可以通过编程或使用 Interface Builder 创建和配置应用程序的 main window。在任何一种情况下，你都应该在启动时创建 window，并且应该保留它，并在应用程序委托对象中存储对它的引用。如果应用程序创建了其他 windows，请让应用程序在需要时延迟创建它们。例如，如果你的应用程序支持在外部显示器上显示内容，则应等到显示器连接后，再创建相应的 window。

&emsp;无论应用程序是在前台还是后台启动，都应该在启动时创建应用程序的 main window。创建和配置 window 本身并不昂贵。但是，如果应用程序直接在后台启动，则应避免在应用程序进入前台之前使 window 可见。

#### Creating Windows in Interface Builder（在 Interface Builder 中创建 Windows）
&emsp;使用 Interface Builder 创建应用程序的 main window 很简单，因为 Xcode 项目模板可以帮你完成。每个新的 Xcode 应用程序项目都包含一个主 nib 文件（通常使用 MainWindow.xib 或其他名称），其中包含该应用程序的 main window。此外，这些模板还在应用程序委托对象中为该 window 定义了 outlet。你可以使用此 outlet 来访问代码中的 window 对象。

> Important: 在 Interface Builder 中创建 window 时，建议启用 attributes inspector 中的 Full Screen at Launch 选项。如果未启用此选项，并且 window 小于目标设备的屏幕，则某些 view 将不会接收触摸事件。这是因为 window（和所有 view 一样）不接收超出其 bounds 的触摸事件。由于 view 在默认情况下不会剪裁到 window 的 bounds，因此 view 仍然显示为可见，但事件不会到达它们。（这里类似当 subviews 的 bounds 超出其 superview 的 bounds 时不会主动对其裁剪，但是触摸超出 superview 的 bounds 之外的区域时也不会响应此事件。）启用 Full Screen at Launch 选项可确保 window 大小适合当前屏幕。

&emsp;如果要改造项目以使用 Interface Builder，则使用 Interface Builder 创建 window 是将 UIWindow 对象拖到 nib 文件中的简单问题。当然，你还应该执行以下操作：

+ 要在运行时访问 window，你应该 connect the window to an outlet，outlet 通常是在应用程序委托或 nib 文件的文件所有者（File’s Owne）中定义的。
+ 如果改造计划包括使新的 nib 文件成为应用程序的 main nib 文件，则还必须将应用程序的 Info.plist 文件中的 NSMainNibFile 键设置为 nib 文件的名称。 更改此键的值可确保在调用应用程序委托的 `- application:didFinishLaunchingWithOptions:` 方法之前，已加载 nib 文件并可供使用。

&emsp;有关创建和配置 nib 文件的详细信息，请参见 Interface Builder User Guide。有关如何在运行时将 nib 文件加载到应用程序中的信息，请参见  Resource Programming Guide 中的 Nib Files。

#### Creating a Window Programmatically（以编程方式创建窗口）
&emsp;如果你希望以编程方式创建应用程序的 main window，则应在 `- application:didFinishLaunchingWithOptions:` 应用程序委托的方法中包含类似于以下代码：
```c++
self.window = [[[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]] autorelease];
```
&emsp;在前面的例子中，self.window 假定为应用程序委托的声明属性，该属性配置为保留窗口对象（strong 修饰）。如果要为外部显示器创建 window，则将其分配给其他变量，并且需要指定表示该显示器的非主 UIScreen 对象的 bounds。

&emsp;创建 windows 时，应始终将 window 大小设置为屏幕的最大范围。不应缩小 window 的大小以容纳状态栏或任何其他项。不管怎样，状态栏总是浮在 window 的顶部，所以只有将 view 放到 window 中，才能缩小状态栏。如果你使用的是 view controllers，那么视图控制器应该自动处理 view 的大小调整。

#### Adding Content to Your Window（向窗口添加内容）
&emsp;每个 window 通常都有一个根视图对象（root view object）（由相应的 view controller 管理），该对象包含代表你的内容的所有其他 views。使用单个 root view 简化了更改界面的过程。要显示新内容，你要做的就是替换 root view。要在 window 中安装 view，请使用 `- addSubview:`方法。例如，要安装由 view controller 管理的 view，你将使用类似于以下代码：
```c++
[window addSubview:viewController.view];
```
&emsp;除了上述代码，你还可以在 nib 文件中配置 window 的 rootViewController 属性。该属性提供了一种使用 nib 文件而非编程方式配置 window root view 的便捷方法。如果从其 nib 文件加载 window 时设置了此属性，则 UIKit 会自动从关联的 view controller 安装 view 作为 window 的 root view。此属性仅用于安装 root view，window 不使用它与 view controller 进行通信。

&emsp;你可以将任何所需的 view 用作 window 的 root view。根据你的界面设计，root view 可以是充当一个或多个 subviews 的容器的通用 UIView 对象，root view 可以是标准系统 view，或者 root view 可以是你定义的 custom view。通常用作 root view 的一些标准系统 views 包括 scroll views、table views 和 image views。

&emsp;在配置 window 的 root view 时，你需要设置其初始大小和在 window 中的位置。对于不包含状态栏或显示半透明状态栏的应用程序，请设置 view 大小以匹配 window 的大小。对于显示不透明状态栏的应用程序，将 view 置于状态栏下方，并相应地减小其大小。从 view 的高度减去状态栏的高度可防止遮挡 view 的顶部。

> Note: 如果 window 的 root view 由容器视图控制器（例如 tab bar controller、navigation controller 或 split-view controller）提供，则无需自己设置 view 的初始大小。容器视图控制器会根据状态栏是否可见来自动调整其 view 的大小。

#### Changing the Window Level（更改 Window 级别）
&emsp;每个 UIWindow 对象都有一个可配置的 windowLevel 属性，该属性确定该 window 相对于其他 windows 的放置方式。在大多数情况下，你无需更改应用程序 windows 的级别。在创建时，新 windows 会自动分配给 normal 窗口级别。normal 窗口级别指示该 window 显示与应用程序相关的内容。较高的 window 级别保留用于需要浮动在应用程序内容上方的信息，例如系统状态栏或警报消息。而且，尽管你可以自己将 window 分配给这些级别，但是当你使用特定的界面时，系统通常会为你执行此操作。例如，当你显示或隐藏状态栏或显示警报 view 时，系统会自动创建所需的 window 以显示这些项目。

### Monitoring Window Changes（监视 Window 更改）
&emsp;如果要跟踪应用程序内部 window 的出现或消失，可以使用以下与 window 相关的通知来进行跟踪：

+ UIWindowDidBecomeVisibleNotification
+ UIWindowDidBecomeHiddenNotification
+ UIWindowDidBecomeKeyNotification
+ UIWindowDidResignKeyNotification

&emsp;这些通知是根据你应用程序 window 中的程序更改而传递的。因此，当你的应用程序显示或隐藏 window 时，将相应地传递 UIWindowDidBecomeVisibleNotification 和 UIWindowDidBecomeHiddenNotification 通知。当你的应用程序进入后台执行状态时，不会发送这些通知。即使你的应用程序在后台时 window 未显示在屏幕上，它仍被认为在你的应用程序上下文中可见。

&emsp;UIWindowDidBecomeKeyNotification 和 UIWindowDidResignKeyNotification 通知可帮助你的应用程序跟踪哪个 window 是 key window，即哪个 window 当前正在接收键盘事件和其他与触摸无关的事件。触摸事件传递到发生触摸的 window，而没有关联坐标值的事件传递到应用程序的 key window。一次仅一个 window 可能是 key window。

### Displaying Content on an External Display（在外接显示器上显示内容）
&emsp;要在外部显示器上显示内容，必须为你的应用程序创建一个附加 window，并将其与代表外部显示器的屏幕对象相关联。默认情况下，新 window 通常与主屏幕关联。更改 window 的关联屏幕对象会导致该 window 的内容重新路由到相应的显示器。window 与正确的屏幕相关联后，你可以向其添加 view 并像在应用程序的主屏幕上一样显示它。

&emsp;UIScreen 类维护代表可用硬件显示的屏幕对象列表。通常，对于任何基于 iOS 的设备，只有一个屏幕对象代表主显示屏，但是支持连接到外部显示器的设备可以具有一个附加的屏幕对象。支持外部显示器的设备包括具有 Retina 显示器的 iPhone 和 iPod touch 设备以及 iPad。较旧的设备（例如 iPhone 3GS）不支持外部显示器。

> Note: 因为外部显示器本质上是视频输出连接，所以不应期望与外部显示器相关联的 window 中的 views 和控件发生触摸事件。此外，你的应用程序有责任根据需要更新 window 的内容。因此，要镜像 main window 的内容，你的应用程序将需要为外部显示器的 window 创建一组重复的视图，并与 main window 中的 views 一起进行更新。

&emsp;以下介绍了在外部显示器上显示内容的过程。但是，以下步骤总结了基本过程：

1. 在应用程序启动时，注册屏幕连接和断开连接通知。
2. 当需要在外部显示器上显示内容时，创建并配置一个 window。
  + 使用 UIScreen 的 screens 属性来获取外部显示器的屏幕对象。
  + 创建一个 UIWindow 对象，并根据屏幕（或你的内容）调整其大小。
  + 将用于外部显示的 UIScreen 对象分配给 window 的 screen 属性。
  + 根据需要调整屏幕对象的分辨率以支持你的内容。
  + 在 window 中添加任何适当的 views。
3. 显示 window 并正常更新。

#### Handling Screen Connection and Disconnection Notifications（处理屏幕连接和断开连接通知）
&emsp;屏幕连接和断开连接通知对于妥善处理外部显示器的更改至关重要。当用户连接或断开显示器连接时，系统会向你的应用程序发送适当的通知。你应该使用这些通知来更新应用程序状态并创建或释放与外部显示器关联的 window。

&emsp;关于连接和断开连接通知，要记住的重要一点是，即使你的应用程序在后台挂起，它们也可以随时出现。因此，最好观察在应用程序运行时持续存在的来自对象的通知，例如你的应用程序委托。如果你的应用程序已暂停，则通知将排队，直到你的应用程序退出暂停状态并开始在前台或后台运行。

&emsp;清单 2-1 显示了用于注册连接和断开连接通知的代码。应用程序委托在初始化时调用此方法，但是你也可以从应用程序中其他位置注册这些通知。清单 2-2 显示了处理程序方法的实现。

&emsp;Listing 2-1  Registering for screen connect and disconnect notifications
```c++
- (void)setupScreenConnectionNotificationHandlers {
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
 
    [center addObserver:self selector:@selector(handleScreenConnectNotification:) name:UIScreenDidConnectNotification object:nil];
    [center addObserver:self selector:@selector(handleScreenDisconnectNotification:) name:UIScreenDidDisconnectNotification object:nil];
}
```

&emsp;如果将外部显示器连接到设备后你的应用程序处于活动状态，则应为该显示器创建第二个 window，并在其中填充一些内容。内容不必是你要呈现的最终内容。例如，如果你的应用程序尚未准备好使用额外的屏幕，则可以使用第二个 window 显示一些占位符内容。如果你没有为屏幕创建 window，或者创建但未显示 window，则外部显示屏上会显示黑场。

&emsp;清单 2-2 显示了如何创建辅助 window 并将其填充一些内容。在此示例中，应用程序在用于接收屏幕连接通知的处理程序方法中创建 window。 （有关注册连接和断开通知的信息，请参见清单 2-1。）用于连接通知的处理程序方法将创建一个辅助 window，将其与新连接的屏幕相关联，并调用应用程序主视图控制器的方法以添加一些内容。内容并显示在 window 中。断开连接通知的处理程序方法释放 window 并通知主视图控制器，以便它可以相应地调整其表示形式。

&emsp;Listing 2-2  Handling connect and disconnect notifications
```c++
- (void)handleScreenConnectNotification:(NSNotification*)aNotification {
    UIScreen* newScreen = [aNotification object];
    CGRect screenBounds = newScreen.bounds;
 
    if (!_secondWindow) {
        _secondWindow = [[UIWindow alloc] initWithFrame:screenBounds];
        _secondWindow.screen = newScreen;
 
        // Set the initial UI for the window.
        [viewController displaySelectionInSecondaryWindow:_secondWindow];
    }
}
 
- (void)handleScreenDisconnectNotification:(NSNotification*)aNotification {
    if (_secondWindow) {
        // Hide and then delete the window.
        _secondWindow.hidden = YES;
        [_secondWindow release];
        _secondWindow = nil;
 
        // Update the main screen based on what is showing here.
        [viewController displaySelectionOnMainScreen];
    }
}
```

#### Configuring a Window for an External Display（为外部显示器配置窗口）
&emsp;要在外部屏幕上显示 window，必须将其与正确的屏幕对象相关联。这个过程包括定位适当的 UIScreen 对象并将其分配给窗口的 screen 属性。你可以从 UIScreen 的 screens 类方法获得 screen 对象的列表。此方法返回的数组始终至少包含一个表示主屏幕的对象。如果存在第二个对象，则该对象表示连接的外部显示器。

&emsp;清单 2-3 显示了一个在应用程序启动时调用的方法，以查看是否已经附加了外部显示器。如果是，该方法将创建一个 window，将其与外部显示器相关联，并在显示 window 之前添加一些占位符内容。在本例中，占位符内容是白色背景和表示没有要显示的内容的标签。若要显示 window，此方法将更改其隐藏属性的值，而不是调用 makeKeyAndVisible。它这样做是因为 window 只包含静态内容，不用于处理事件。

&emsp;Listing 2-3  Configuring a window for an external display
```c++
- (void)checkForExistingScreenAndInitializeIfPresent {
    if ([[UIScreen screens] count] > 1) {
        // Associate the window with the second screen.
        // The main screen is always at index 0.
        UIScreen* secondScreen = [[UIScreen screens] objectAtIndex:1];
        CGRect screenBounds = secondScreen.bounds;
 
        _secondWindow = [[UIWindow alloc] initWithFrame:screenBounds];
        _secondWindow.screen = secondScreen;
 
        // Add a white background to the window
        UIView* whiteField = [[UIView alloc] initWithFrame:screenBounds];
        whiteField.backgroundColor = [UIColor whiteColor];
 
        [_secondWindow addSubview:whiteField];
        [whiteField release];
 
        // Center a label in the view.
        NSString* noContentString = [NSString stringWithFormat:@"<no content>"];
        CGSize stringSize = [noContentString sizeWithFont:[UIFont systemFontOfSize:18]];
 
        CGRect labelSize = CGRectMake((screenBounds.size.width - stringSize.width) / 2.0,
                                    (screenBounds.size.height - stringSize.height) / 2.0,
                                    stringSize.width, stringSize.height);
 
        UILabel* noContentLabel = [[UILabel alloc] initWithFrame:labelSize];
        noContentLabel.text = noContentString;
        noContentLabel.font = [UIFont systemFontOfSize:18];
        [whiteField addSubview:noContentLabel];
 
        // Go ahead and show the window.
        _secondWindow.hidden = NO;
    }
}
```

> Important: 在显示 window 之前，应始终将屏幕与 window 关联。尽管可以更改当前可见 window 的屏幕，但这是一项昂贵的操作，应避免。

&emsp;一旦显示外部屏幕的 window，应用程序就可以像其他 window 一样开始更新它。你可以根据需要添加和删除 subviews，更改 subviews 的内容，设置 view 更改的动画，并根据需要使其内容无效。

#### Configuring the Screen Mode of an External Display（配置外接显示器的屏幕模式）
&emsp;根据你的内容，你可能需要在将 window 与其关联之前更改屏幕模式。许多屏幕支持多种分辨率，其中一些分辨率使用不同的像素长宽比。屏幕对象默认情况下使用最常见的屏幕模式，但是你可以将该模式更改为更适合你的内容的模式。例如，如果你要使用 OpenGL ES 来实现游戏，并且纹理是为 640 x 480 像素屏幕设计的，则可以更改具有更高默认分辨率的屏幕的屏幕模式。

&emsp;如果计划使用默认模式以外的其他屏幕模式，则应在将屏幕与 window 关联之前，将该模式应用于 UIScreen 对象。 UIScreenMode 类定义单个屏幕模式的属性。你可以从屏幕的 availableModes 属性获取屏幕支持的模式的列表，并在列表中进行迭代以找到符合你需求的模式。

&emsp;有关屏幕模式的更多信息，请参见 UIScreenMode Class Reference。

## Animations
&emsp;动画在用户界面的不同状态之间提供流畅的视觉过渡。在 iOS 中，动画广泛用于重新定位 view、更改其大小、从 view 层次结构中删除 view 以及隐藏 view。你可以使用动画向用户传达反馈或实现有趣的视觉效果。

&emsp;在 iOS 中，创建复杂的动画不需要你编写任何绘图代码。本章中描述的所有动画技术都使用 Core Animation 提供的内置支持。你要做的就是触发动画并让 Core Animation 处理单个帧的渲染。这使得仅需几行代码即可轻松创建复杂的动画。

### What Can Be Animated?
&emsp;UIKit 和 Core Animation 都提供对动画的支持，但是每种技术提供的支持级别不同。在 UIKit 中，使用 UIView 对象执行动画。Views 支持包含许多常见任务的基本动画集。例如，可以设置 views 属性更改的动画，或使用 transition 动画将一组 views 替换为另一组 views。

&emsp;表 4-1 列出了 UIView 类的可设置动画的属性（具有内置动画支持的属性）。具有动画效果并不意味着动画会自动发生。更改这些属性的值通常只需要立即更新属性（和 view），而无需动画。要对此类更改进行动画处理，必须从动画块内部更改属性的值，这在 Animating Property Changes in a View 中进行介绍。

&emsp;Table 4-1  Animatable UIView properties

| Property | Changes you can make |
| --- | --- |
| frame | 修改此属性，以更改 view 相对于其 superview 坐标系的大小和位置。 （如果 transform 属性不包含 identity 变换，请改为修改 bounds 或 center 属性。） |
| bounds | 修改此属性以更改 view 的大小。 |
| center | 修改此属性以更改 view 相对于其 superview 坐标系的位置。 |
| transform | 修改此属性以相对于其中心点缩放、旋转或平移 view。使用此属性的变换始终在 2D 空间中执行。 （要执行 3D 转换，你必须使用 Core Animation 为 view 的 layer 对象设置动画。） |
| alpha | 修改此属性以逐渐更改 view 的透明度。 |
| backgroundColor | 修改此属性以更改 view 的背景色。 |
| contentStretch | 修改此属性可更改 view 内容被拉伸以填充可用空间的方式。 |

&emsp;Animated view transitions 是你对 view 层次进行更改的一种方法，可以超越 view controller 提供的那些方法。尽管你应该使用视图控制器来管理简洁的视图层次结构，但是有时你可能希望替换全部或部分视图层次结构。在这种情况下，你可以使用基于视图的 transitions 来为视图的添加和删除添加动画效果。

&emsp;在你想要执行更复杂的动画或 UIView 类不支持的动画的地方，可以使用 Core Animation 和视图的基础 layer 来创建动画。由于视图和图层对象错综复杂地链接在一起，因此对视图图层的更改会影响视图本身。使用 Core Animation，可以为视图的图层设置以下类型的动画：

+ 图层的大小和位置
+ 执行 transformations 时使用的中心点
+ Transformations 为 3D 空间中的 layer 或其 sublayers
+ 在 layer 层次结构中添加或删除 layer 
+ layer 相对于其他同级图层（sibling layers）的 Z 顺序（Z-order ）
+ layer 的阴影（shadow）
+ layer 的 border（包括 layer 的 corners 是否圆角）
+ 在调整大小操作期间拉伸的 layer 部分
+ layer 的不透明度（opacity）
+ 超出 layer bounds 的 sublayers 的裁剪行为
+ layer 的当前内容
+ layer 的栅格化行为（rasterization behavior）

> Note: 如果 view 承载自定义 layer 对象（即没有关联 view 的 layer 对象），则必须使用 Core Animation 来设置对其所做任何更改的动画。

&emsp;尽管本章介绍了一些 Core Animation 行为，但它是从 view  代码启动它们相关的。有关如何使用 Core Animation 为 layers 设置动画的详细信息，请参见 Core Animation Programming Guide 和 Core Animation Cookbook。

### Animating Property Changes in a View（在 View 中动画化属性更改）
&emsp;



















## UIView

### Laying out Subviews（布局子视图）
&emsp;如果你的应用未使用 Auto Layout，则手动设置视图的布局。
#### - layoutSubviews
&emsp;布局子视图。
```c++
// override point. called by layoutIfNeeded automatically. 
// As of iOS 6.0, when constraints-based layout is used the base implementation applies the constraints-based layout, otherwise it does nothing.
- (void)layoutSubviews;
```
&emsp;由 `- layoutIfNeeded` 自动调用。从 iOS 6.0 开始，使用基于约束（constraints-based）的布局时，基本实现将应用基于约束的布局，否则不执行任何操作。

&emsp;此方法的默认实现在 iOS 5.1 及更低版本上不执行任何操作。否则，默认实现将使用你设置的任何约束来确定任何子视图的大小（size）和位置（position）。

&emsp;子类可以根据需要重写此方法，以更精确地布局其子视图。仅当子视图的自动调整大小（autoresizing）和基于约束（constraint-based）的行为没有提供所需的行为时，才应重写此方法。你可以使用实现直接设置子视图的 frame rectangles。

&emsp;你不应该直接调用此方法。如果要强制更新布局，请在下次 drawing update 之前调用 `- setNeedsLayout` 方法。如果要立即更新视图的布局，请调用 `- layoutIfNeeded` 方法。
#### - setNeedsLayout
&emsp;使 UIView 的当前布局无效，并在下一个更新周期内触发布局更新。
```c++
// Allows you to perform layout before the drawing cycle happens.
// -layoutIfNeeded forces layout early.
- (void)setNeedsLayout;
```
&emsp;允许你在绘图周期发生之前执行布局。

&emsp;如果要调整视图子视图的布局，请在应用程序的主线程上调用此方法。此方法记录请求并立即返回。因为此方法不强制立即更新，而是等待下一个更新周期，所以可以使用它在更新任何视图之前使多个视图的布局无效。此行为允许你将所有布局更新合并到一个更新周期，这通常会提高性能。
#### - layoutIfNeeded
&emsp;如果布局更新正在等待中，请立即布置子视图。
```c++
- (void)layoutIfNeeded;
```
&emsp;使用此方法可强制视图立即更新其布局。使用 Auto Layout 时，布局引擎会根据需要更新视图的位置，以满足约束的更改。使用接收消息的视图作为根视图，此方法从根视图开始布局子视图。如果根视图没有被标记为需要更新布局，则此方法将退出，而不修改布局或调用任何与布局相关的回调。
#### requiresConstraintBasedLayout
&emsp;一个布尔值，指示 UIView 是否依赖于基于约束的布局系统。
```c++
/* constraint-based layout engages lazily when someone tries to use it (e.g., adds a constraint to a view).  
 * If you do all of your constraint set up in -updateConstraints, you might never even receive updateConstraints if no one makes a constraint.  
 * To fix this chicken and egg problem, override this method to return YES if your view needs the window to use constraint-based layout.  
 */
@property(class, nonatomic, readonly) BOOL requiresConstraintBasedLayout API_AVAILABLE(ios(6.0));
```
&emsp;Return Value: 如果视图必须位于使用基于约束的布局的 window 中才能正常运行，则为 YES，否则为 NO。

&emsp;如果自定义视图无法使用自动调整大小正确地布局，则应覆盖它以返回 YES。
#### translatesAutoresizingMaskIntoConstraints
&emsp;一个布尔值，它确定视图的自动调整大小蒙版是否转换为 Auto Layout 约束。
```c++
/* By default, the autoresizing mask on a view gives rise to constraints that fully determine the view's position. 
 * This allows the auto layout system to track the frames of views whose layout is controlled manually (through -setFrame:, for example).
 * When you elect to position the view using auto layout by adding your own constraints, 
 * you must set this property to NO. IB will do this for you.
 */
@property(nonatomic) BOOL translatesAutoresizingMaskIntoConstraints API_AVAILABLE(ios(6.0)); // Default YES
```
&emsp;如果此属性的值为 YES，则系统将创建一组约束，这些约束将复制视图的自动调整大小蒙版所指定的行为。这也使你可以使用视图的 frame，bounds 或 center 属性来修改视图的大小和位置，从而可以在 Auto Layout 中创建基于 frame 的静态布局。

&emsp;请注意，自动调整大小遮罩约束完全指定视图的大小和位置；因此，不能添加其他约束来修改此大小或位置，而不会引入冲突。如果要使用 Auto Layout 动态计算视图的大小和位置，则必须将此属性设置为 NO，然后为视图提供一组无歧义、无冲突的约束。

&emsp;默认情况下，以编程方式创建的任何视图的属性均设置为 YES。如果在 Interface Builder 中添加视图，则系统会自动将此属性设置为 NO。
### Drawing and Updating the View（绘制和更新视图）
#### - drawRect:
&emsp;在传入的矩形内绘制 UIView 的图像。
```c++
- (void)drawRect:(CGRect)rect;
```
&emsp;`rect`: 视图范围中需要更新的部分。第一次绘制视图时，此矩形通常是视图的整个可见范围。但是，在后续的绘图操作中，矩形可能仅指定视图的一部分。

&emsp;此方法的默认实现不执行任何操作。使用 Core Graphics 和 UIKit 等技术绘制视图内容的子类应重写此方法并在那里实现其绘制代码。如果视图以其他方式设置其内容，则不需要重写此方法。例如，如果视图仅显示背景色，或者视图直接使用底层对象（CALayer）设置其内容，则不需要重写此方法。

&emsp;调用此方法时，UIKit 已经为你的视图适当地配置了绘图环境，你可以简单地调用渲染内容所需的任何绘图方法和函数。具体来说，UIKit 创建和配置用于绘制的图形上下文，并调整该上下文的转换，使其原点与视图边框的原点匹配。可以使用 UIGraphicsGetCurrentContext 函数获取对图形上下文的引用，但不要建立对图形上下文的强引用，因为它可以在调用 `- drawRect:` 方法之间更改。

&emsp;类似地，如果使用 OpenGL ES 和 GLKView 类进行绘制，那么 GLKit 在调用此方法（或 `glkView:drawInRect:` GLKView 委托的方法），因此你只需发出渲染内容所需的任何 OpenGL ES 命令。有关如何使用 OpenGL ES 绘图的更多信息，请参见 OpenGL ES Programming Guide。

&emsp;应将任何图形限制为 rect 参数中指定的矩形。此外，如果视图的 opaque 属性设置为 YES，则 `drawRect:` 方法必须用不透明内容完全填充指定的矩形。

&emsp;如果直接将 UIView 子类化，则此方法的实现不需要调用 super。但是，如果你正在子类化一个不同的视图类，那么应该在实现的某个时候调用 super。

&emsp;当第一次显示视图或发生使视图的可见部分无效的事件时，将调用此方法。你不应该自己直接调用这个方法。若要使视图的一部分无效，从而导致该部分被重新绘制，请改为调用 `setNeedsDisplay` 或 `setNeedsDisplayInRect:` 方法。
#### - setNeedsDisplay
&emsp;将 UIView 的整个 bounds 矩形标记为需要重绘。
```c++
- (void)setNeedsDisplay;
```
&emsp;你可以使用此方法或 `- setNeedsDisplayInRect:` 通知系统你的视图内容需要重绘。此方法记录请求并立即返回。该视图实际上直到下一个绘图周期才被重绘，此时所有无效的视图都将更新。

&emsp;Note: 如果视图由 CAEAGLLayer 对象支持，则此方法无效。它仅适用于使用本机绘图技术（例如 UIKit 和 Core Graphics）来呈现其内容的视图。

&emsp;应该使用此方法请求仅当视图的内容或外观更改时才重新绘制视图。如果仅更改视图的几何图形，则通常不会重新绘制视图。而是根据视图的 contentMode 属性中的值调整其现有内容。重新显示现有内容可以避免重新绘制未更改的内容，从而提高性能。
#### - setNeedsDisplayInRect:
&emsp;将 UIView 的指定矩形标记为需要重绘。
```c++
- (void)setNeedsDisplayInRect:(CGRect)rect;
```
&emsp;`rect`: 接收器的矩形区域标记为无效；它应该在接收器的坐标系中指定。

&emsp;可以使用此方法或 `- setNeedsDisplay` 通知系统需要重画视图的内容。此方法将指定的矩形添加到视图的当前无效矩形列表中，并立即返回。在下一个绘图周期之前，视图实际上不会重新绘制，此时所有无效的视图都会更新。

&emsp;Note: 如果视图由 CAEAGLLayer 对象支持，则此方法无效。它仅适用于使用本机绘图技术（例如 UIKit 和 Core Graphics）来呈现其内容的视图。

&emsp;应该使用此方法请求仅当视图的内容或外观更改时才重新绘制视图。如果仅更改视图的几何图形，则通常不会重新绘制视图。而是根据视图的 contentMode 属性中的值调整其现有内容。重新显示现有内容可以避免重新绘制未更改的内容，从而提高性能。
#### contentScaleFactor
&emsp;应用于视图的比例因子。
```c++
@property(nonatomic) CGFloat    contentScaleFactor API_AVAILABLE(ios(4.0));
```
&emsp;比例因子确定视图中的内容如何从逻辑坐标空间（以点为单位）映射到设备坐标空间（以像素为单位）。此值通常为 1.0 或 2.0。较高的比例因子表示视图中的每个点由底层中的多个像素表示。例如，如果比例因子为 2.0，图幅大小为 50 x 50 点，则用于显示该内容的位图大小为 100 x 100 像素。

&emsp;此属性的默认值是与当前显示视图的屏幕相关联的比例因子。如果你的自定义视图实现了一个自定义 `- drawRect:` 方法并与窗口相关联，或者如果你使用 GLKView 类来绘制 OpenGL ES 内容，那么你的视图将以屏幕的完整分辨率进行绘制。对于系统视图，即使在高分辨率屏幕上，此属性的值也可能为 1.0。

&emsp;通常，不需要修改此属性中的值。但是，如果应用程序使用 OpenGL ES 绘制，则可能需要更改比例因子，以牺牲图像质量来获得渲染性能。有关如何调整 OpenGL ES 渲染环境的详细信息，请参见 OpenGL ES Programming Guide 中的 Supporting High-Resolution Displays 部分。
#### - tintColorDidChange
&emsp;当 tintColor 属性更改时由系统调用。
```c++
/*
 * The -tintColorDidChange message is sent to appropriate subviews of a view when its tintColor
 * is changed by client code or to subviews in the view hierarchy of a view whose tintColor is
 * implicitly changed when its superview or tintAdjustmentMode changes.
 */
- (void)tintColorDidChange API_AVAILABLE(ios(7.0));
```
&emsp;当你的代码更改该视图上的 tintColor 属性的值时，系统将在该视图上调用此方法。此外，系统在继承了更改的交互色的子视图上调用此方法。

&emsp;在你的实现中，根据需要刷新视图渲染。












## 参考链接
**参考链接:🔗**
+ [UIView](https://developer.apple.com/documentation/uikit/uiview?language=objc)
+ [View Programming Guide for iOS](https://developer.apple.com/library/archive/documentation/WindowsViews/Conceptual/ViewPG_iPhoneOS/Introduction/Introduction.html#//apple_ref/doc/uid/TP40009503)
