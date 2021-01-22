# iOS CALayer 官方文档
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
&emsp;Layers 通常用于为 view 提供 backing store，但也可以在没有 view 的情况下使用以显示内容。layer 的主要工作是管理你提供的视觉内容（visual content），但 layer 本身也具有可以设置的视觉属性（visual attributes），例如背景色（background color）、边框（border）和阴影（shadow）。除了管理视觉内容外，layer 还维护有关其内容的几何（geometry）（例如其位置（position）、大小（size）和变换（transform））的信息，这些信息用于在屏幕上显示该内容。修改 layer 的属性是在 layer 的内容或几何（geometry）上启动动画的方式。layer 对象通过 CAMediaTiming 协议封装 layer 及其动画的持续时间（duration）和步调（pacing），该协议定义了 layer 的时间信息（timing information）。

&emsp;如果 layer 对象是由 view 创建的，则 view 通常会自动将自身指定为 layer 的 delegate，并且不应更改该关系。对于你自己创建的 layers，可以为其指定一个 delegate 对象，并使用该对象动态提供 layer 的内容并执行其他任务。layer 可能还具有布局管理器（layout manager）对象（指定给 layoutManager 属性），以分别管理子图层（sublayers）的布局。
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
&emsp;这是不在 presentation layer（表示层）中的 layer 对象的指定初始化程序。
#### - initWithLayer:
&emsp;重写以复制或初始化指定 layer 的自定义字段。

&emsp;Core Animation 使用此初始值设定项来创建 layers 的 shadow 副本，例如用作 presentation layers。子类可以重写此方法，以将其实例变量复制到 presentation layer 中（子类随后应调用超类）。在任何其他情况下调用此方法都将导致未定义的行为。
```c++
- (instancetype)initWithLayer:(id)layer;
```
&emsp;`layer`: 应从其复制自定义字段的 layer。

&emsp;Return Value:从 `layer` 复制的任何自定义实例变量的 layer 实例。

&emsp;此初始化程序用于创建 layer 的 shadow 副本，例如，用于 `presentationLayer` 方法。在任何其他情况下使用此方法都会产生不确定的行为。例如，请勿使用此方法用现有 layer 的内容初始化新 layer。

&emsp;如果要实现自定义 layer 子类，则可以重写此方法并将其用于将实例变量的值复制到新对象中。子类应始终调用超类实现。

&emsp;此方法是 presentation layer（表示层）中各 layer 对象的指定初始化器。
### Accessing Related Layer Objects（访问相关 layer 对象）
#### - presentationLayer
&emsp;返回 presentation layer 对象的副本，该对象表示当前在屏幕上显示的 layer 的状态。
```c++
- (nullable instancetype)presentationLayer;
```
&emsp;Return Value: 当前 presentation layer 对象的副本。

&emsp;通过此方法返回的 layer 对象提供了当前在屏幕上显示的 layer 的近似值。在动画制作过程中，你可以检索该对象并使用它来获取那些动画的当前值。

&emsp;返回 layer 的 sublayers、mask 和 superlayer 属性从表示树（presentation tree）（而不是模型树）返回相应的对象。此模式也适用于任何只读 layer 方法。例如，返回对象的 hitTest: 方法查询 presentation tree 中的 layer 对象。

Returns a copy of the layer containing all properties as they were at the start of the current transaction, with any active animations applied. This gives a close approximation to the version of the layer that is currently displayed. Returns nil if the layer has not yet been committed.
返回包含所有属性的层的副本，这些属性与当前事务开始时的属性相同，并应用了所有活动动画。这非常接近当前显示的图层版本。如果尚未提交该层，则返回 nil。

The effect of attempting to modify the returned layer in any way is undefined.
尝试以任何方式修改返回的图层的效果是不确定的。

The sublayers, mask and superlayer properties of the returned layer return the presentation versions of these properties. This carries through to read-only layer methods. E.g., calling -hitTest: on the result of the -presentationLayer will query the presentation values of the layer tree.
返回层的“ sublayers”，“ mask”和“ superlayer”属性返回这些属性的表示形式。这将执行只读层方法。例如，在-presentationLayer的结果上调用-hitTest：将查询层树的表示值。

#### - modelLayer
&emsp;返回与 receiver 关联的模型层对象（如果有）。
```c++
- (instancetype)modelLayer;
```
&emsp;Return Value: 表示基础模型层的层实例。

&emsp;在表示树中的图层上调用此方法将返回模型树中的相应图层对象。仅当涉及表示层更改的事务正在进行时，此方法才返回值。如果没有正在进行的事务，则调用此方法的结果是不确定的。

/* When called on the result of the -presentationLayer method, returns the underlying layer with the current model values. When called on a non-presentation layer, returns the receiver. The result of calling this method after the transaction that produced the presentation layer has completed is undefined. */
在 -presentationLayer 方法的结果上调用时，返回具有当前模型值的基础层。在非表示层上调用时，返回接收者。产生表示层的事务完成后调用此方法的结果是不确定的。
### Accessing the Delegate

#### delegate
&emsp;layer 的委托对象。
```c++
@property(nullable, weak) id <CALayerDelegate> delegate;
```
&emsp;你可以使用委托对象来提供图层的内容，处理任何子图层的布局以及提供自定义操作以响应与图层相关的更改。您分配给此属性的对象应实现 CALayerDelegate 非正式协议的一种或多种方法。关于协议的更多信息，请参见 CALayerDelegate。

&emsp;在 iOS 中，如果图层与 UIView 对象关联，则必须将此属性设置为拥有该图层的视图。

An object that will receive the CALayer delegate methods defined below (for those that it implements). The value of this property is not retained. Default value is nil.
一个对象，它将接收下面定义的 CALayer 委托方法（针对其实现的方法）。不保留此属性的值。默认值为 nil。

### Providing the Layer’s Content
#### contents
&emsp;提供图层内容的对象。可动画的。
```c++
@property(nullable, strong) id contents;
```
&emsp;此属性的默认值为 nil。

&emsp;如果使用图层显示静态图像，则可以将此属性设置为 CGImageRef，其中包含要显示的图像。 （在macOS 10.6及更高版本中，你也可以将属性设置为 NSImage 对象。）为该属性分配值会导致图层使用你的图像，而不是创建单独的后备存储（backing store）。

&emsp;如果图层对象绑定到视图对象，则应避免直接设置此属性的内容。视图和图层之间的相互作用通常会导致视图在后续更新期间替换此属性的内容。

/** Layer content properties and methods. **/
图层内容的属性和方法。

/* An object providing the contents of the layer, typically a CGImageRef, but may be something else. (For example, NSImage objects are supported on Mac OS X 10.6 and later.) Default value is nil. Animatable. */
提供该层内容的对象，通常为 CGImageRef，但也可以为其他对象。 （例如，Mac OS X 10.6 和更高版本支持 NSImage 对象。）默认值为 nil。可动画的。
#### contentsRect
&emsp;单位坐标空间中的矩形，用于定义应使用的图层内容部分。可动画的。
```c++
@property CGRect contentsRect;
```
&emsp;默认为单位矩形（0.0、0.0、1.0、1.0）。

&emsp;如果请求单位矩形之外的像素，则内容图像的边缘像素将向外扩展。

&emsp;如果提供了一个空矩形，则结果是不确定的。

/* A rectangle in normalized image coordinates defining the subrectangle of the `contents' property that will be drawn into the layer. If pixels outside the unit rectangles are requested, the edge pixels of the contents image will be extended outwards. If an empty rectangle is provided, the results are undefined. Defaults to the unit rectangle [0 0 1 1]. Animatable. */
标准化图像坐标中的矩形，定义了将被绘制到图层中的“ contents”属性的子矩形。如果请求单位矩形之外的像素，则内容图像的边缘像素将向外扩展。如果提供了一个空矩形，则结果是不确定的。默认为单位矩形[0 0 1 1]。可动画的。

#### contentsCenter
&emsp;矩形，用于定义在调整图层内容大小时如何缩放图层内容。可动画的。
```c++
@property CGRect contentsCenter;
```
&emsp;可以使用此属性将图层内容细分为 3x3 网格。此属性中的值指定网格中中心矩形的位置和大小。如果层的 contentsGravity 属性设置为某个调整大小模式，则调整层的大小会导致网格的每个矩形中发生不同的缩放。中心矩形在两个维度上都拉伸，上中心和下中心矩形仅水平拉伸，左中心和右中心矩形仅垂直拉伸，四角矩形完全不拉伸。因此，你可以使用此技术来实现可拉伸的背景或使用三部分或九部分图像的图像。

&emsp;默认情况下，此属性中的值设置为单位矩形（0.0,0.0）（1.0,1.0），这将导致整个图像在两个维度上缩放。如果指定的矩形超出单位矩形，则结果未定义。只有在将 contentsRect 属性应用于图像之后，才应用指定的矩形。

> &emsp;Note: 如果此属性中矩形的宽度或高度很小或为 0，则该值将隐式更改为以指定位置为中心的单个源像素的宽度或高度。

/* A rectangle in normalized image coordinates defining the scaled center part of the `contents' image.
标准化图像坐标中的矩形定义了“内容”图像的缩放中心部分。

* When an image is resized due to its `contentsGravity' property its center part implicitly defines the 3x3 grid that controls how the image is scaled to its drawn size. The center part is stretched in both dimensions; the top and bottom parts are only stretched horizontally; the left and right parts are only stretched vertically; the four corner parts are not stretched at all. (This is often called "9-slice scaling".)
当图像由于其“ contentsGravity”属性而调整大小时，其中心部分隐式定义了 3x3 网格，该网格控制如何将图像缩放到其绘制的大小。中心部分在两个方向上都拉伸。顶部和底部仅水平拉伸；左右部分仅垂直拉伸；四个角部分根本没有拉伸。 （这通常称为“ 9切片缩放”。）

* The rectangle is interpreted after the effects of the `contentsRect' property have been applied. It defaults to the unit rectangle [0 0 1 1] meaning that the entire image is scaled. As a special case, if the width or height is zero, it is implicitly adjusted to the width or height of a single source pixel centered at that position. If the rectangle extends outside the [0 0 1 1] unit rectangle the result is undefined. Animatable. */
矩形在应用了 contentsRect 属性的效果后被解释。默认为单位矩形[0 0 1 1]，表示整个图像都会缩放。作为特殊情况，如果宽度或高度为零，则将其隐式调整为以该位置为中心的单个源像素的宽度或高度。如果矩形延伸到[0 0 1 1]单元矩形的外部，则结果不确定。可动画的。
#### - display
&emsp;重新加载该层的内容。
```c++
- (void)display;
```
&emsp;不要直接调用此方法。图层会在适当的时候调用此方法以更新图层的内容。如果图层具有委托对象，则此方法尝试调用委托的 displayLayer：方法，委托可使用该方法来更新图层的内容。如果委托未实现 displayLayer：方法，则此方法将创建后备存储并调用图层的 drawInContext：方法以将内容填充到该后备存储中。新的后备存储将替换该层的先前内容。

&emsp;子类可以重写此方法，并使用它直接设置图层的 contents 属性。如果你的自定义图层子类对图层更新的处理方式不同，则可以执行此操作。

/* Reload the content of this layer. Calls the -drawInContext: method then updates the `contents' property of the layer. Typically this is not called directly. */
重新加载该层的内容。调用 -drawInContext：方法，然后更新图层的 “ contents” 属性。通常，不直接调用它。
#### - drawInContext:
&emsp;使用指定的图形上下文绘制图层的内容。
```c++
- (void)drawInContext:(CGContextRef)ctx;
```
&emsp;`ctx`: 在其中绘制内容的图形上下文。上下文可以被裁剪以保护有效的层内容。希望找到要绘制的实际区域的子类可以调用 CGContextGetClipBoundingBox。

&emsp;此方法的默认实现本身不会进行任何绘制。如果图层的委托实现了 drawLayer：inContext：方法，则会调用该方法进行实际绘制。

&emsp;子类可以重写此方法，并使用它来绘制图层的内容。绘制时，应在逻辑坐标空间中的点中指定所有坐标。

/* Called via the -display method when the `contents' property is being updated. Default implementation does nothing. The context may be clipped to protect valid layer content. Subclasses that wish to find the actual region to draw can call CGContextGetClipBoundingBox(). */
当 contents 属性被更新时，通过-display方法调用。默认实现不执行任何操作。上下文可以被裁剪以保护有效的层内容。希望找到要绘制的实际区域的子类可以调用 CGContextGetClipBoundingBox（）。
### Modifying the Layer’s Appearance
#### contentsGravity
&emsp;一个常数，指定图层内容如何在其边界内定位或缩放。
```c++
@property(copy) CALayerContentsGravity contentsGravity;
```
&emsp;Contents Gravity Values 中列出了此属性的可能值。

&emsp;此属性的默认值为 kCAGravityResize。

> &emsp;Important: 内容重力常数的命名基于垂直轴的方向。如果将重力常数与垂直分量（例如 kCAGravityTop）一起使用，则还应检查层的内容是否重叠。如果该选项为“是”，kCAGravityTop将内容与层的底部对齐，kCAGravityBottom将内容与层的顶部对齐。
> 
> &emsp;macOS 和 iOS 中视图的默认坐标系在垂直轴的方向上不同：在 macOS 中，默认坐标系的原点位于绘图区域的左下角，正值从中向上延伸，在 iOS 中，默认坐标系的原点位于绘图区域的左上角，正值从该坐标系向下延伸。

&emsp;图1显示了四个示例，这些示例为图层的contentsGravity属性设置不同的值。

&emsp;Figure 1 Different effects of setting a layer's contents gravity

![]()

1. Contents gravity is kCAGravityResize - the default

2. Contents gravity is kCAGravityCenter

3. Contents gravity is contentsAreFlipped ? kCAGravityTop : kCAGravityBottom

4. Contents gravity is contentsAreFlipped ? kCAGravityBottomLeft : kCAGravityTopLeft


/* A string defining how the contents of the layer is mapped into its bounds rect. Options are 'center', 'top', 'bottom', 'left', 'right', 'topLeft', 'topRight', 'bottomLeft', 'bottomRight', 'resize', 'resizeAspect', 'resizeAspectFill'. The default value is `resize'. Note that "bottom" always means "Minimum Y" and "top" always means "Maximum Y". */

一个字符串，定义了如何将图层的内容映射到其边界 rect。选项为'center'，'top'，'bottom'，'left'，'right'，'topLeft'，'topRight'，'bottomLeft'，'bottomRight'，'resize'，'resizeAspect'，'resizeAspectFill'。默认值为`resize'。注意，“底部”始终表示“最小Y”，“顶部”始终表示“最大Y”。
#### Contents Gravity Values
&emsp;当层边界大于内容对象的边界时，内容重力常量指定内容对象的位置。它们由 contentsGravity 属性使用。
```c++
CA_EXTERN CALayerContentsGravity const kCAGravityCenter API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
CA_EXTERN CALayerContentsGravity const kCAGravityTop API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
CA_EXTERN CALayerContentsGravity const kCAGravityBottom API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
CA_EXTERN CALayerContentsGravity const kCAGravityLeft API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
CA_EXTERN CALayerContentsGravity const kCAGravityRight API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
CA_EXTERN CALayerContentsGravity const kCAGravityTopLeft API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
CA_EXTERN CALayerContentsGravity const kCAGravityTopRight API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
CA_EXTERN CALayerContentsGravity const kCAGravityBottomLeft API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
CA_EXTERN CALayerContentsGravity const kCAGravityBottomRight API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
CA_EXTERN CALayerContentsGravity const kCAGravityResize API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
CA_EXTERN CALayerContentsGravity const kCAGravityResizeAspect API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
CA_EXTERN CALayerContentsGravity const kCAGravityResizeAspectFill API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
```
+ kCAGravityCenter: 内容在边界矩形中水平和垂直居中。
+ kCAGravityTop: 内容在边界矩形的上边缘水平居中。
+ kCAGravityBottom: 内容在边界矩形的下边缘水平居中。
+ kCAGravityLeft: 内容在边界矩形的左边缘垂直居中。
+ kCAGravityRight: 内容在边界矩形的右边缘垂直居中。
+ kCAGravityTopLeft: 内容位于边界矩形的左上角。
+ kCAGravityTopRight: 内容位于边界矩形的右上角。
+ kCAGravityBottomLeft: 内容位于边界矩形的左下角。
+ kCAGravityBottomRight: 内容位于边界矩形的右下角。
+ kCAGravityResize: 调整内容大小以适合整个边界矩形。
+ kCAGravityResizeAspect: 调整内容大小以适合边界矩形，从而保留内容的外观。如果内容没有完全填充边界矩形，则内容将以部分轴为中心。
+ kCAGravityResizeAspectFill:调整内容大小以完全填充边界矩形，同时仍保留内容的外观。内容以其超过的轴为中心。

#### opacity
&emsp;receiver 的不透明度。可动画的。
```c++
@property float opacity;
```
&emsp;此属性的值必须在0.0（透明）到1.0（不透明）之间。超出该范围的值将被限制为最小值或最大值。此属性的默认值为1.0。

/* The opacity of the layer, as a value between zero and one. Defaults to one. Specifying a value outside the [0,1] range will give undefined results. Animatable. */
图层的不透明度，介于0和1之间的值。默认为1。指定超出[0,1]范围的值将产生不确定的结果。可动画的。

#### hidden
&emsp;指示是否显示图层的布尔值。可动画的。
```c++
@property(getter=isHidden) BOOL hidden;
```
&emsp;此属性的默认值为 NO。

/* When true the layer and its sublayers are not displayed. Defaults to NO. Animatable. */
如果为true，则不显示该图层及其子图层。默认为NO。可动画的。
#### masksToBounds
&emsp;一个布尔值，指示是否将子层裁剪到该层的边界。可动画的。
```c++
@property BOOL masksToBounds;
```
&emsp;当此属性的值为YES时，Core Animation将创建一个隐含的剪贴蒙版，该蒙版与图层的边界匹配并包括任何角半径效果。如果还指定了mask属性的值，则将两个掩码相乘以获得最终的掩码值。

&emsp;此属性的默认值为NO。

/* When true an implicit mask matching the layer bounds is applied to the layer (including the effects of the 'cornerRadius' property). If both 'mask' and 'masksToBounds' are non-nil the two masks are multiplied to get the actual mask values. Defaults to NO. Animatable. */
如果为 true，则将与图层范围匹配的隐式蒙版应用于该图层（包括“ cornerRadius”属性的效果）。如果“ mask”和“ masksToBounds”都不为零，则将两个掩码相乘以获得实际的掩码值。默认为 NO。可动画的。
#### mask
&emsp;可选图层，其 Alpha 通道用于掩盖图层的内容。
```c++
@property(nullable, strong) __kindof CALayer *mask;
```
&emsp;图层的 Alpha 通道决定了该图层的内容和背景可以显示多少。完全或部分不透明的像素允许基础内容显示出来，但是完全透明的像素会阻止该内容。

&emsp;此属性的默认值为 nil。配置遮罩时，请记住设置遮罩层的大小和位置，以确保其与遮罩的层正确对齐。

&emsp;你分配给此属性的图层不得具有 superlayer。如果是这样，则行为是不确定的。

/* A layer whose alpha channel is used as a mask to select between the layer's background and the result of compositing the layer's contents with its filtered background. Defaults to nil. When used as a mask the layer's 'compositingFilter' and `backgroundFilters' properties are ignored. When setting the mask to a new layer, the new layer must have a nil superlayer, otherwise the behavior is undefined. Nested masks (mask layers with their own masks) are unsupported. */
图层，其 alpha 通道用作遮罩，以在图层的背景和将其内容与其过滤的背景合成的结果之间进行选择。默认为零。当用作遮罩时，图层的'compositingFilter'和'backgroundFilters'属性将被忽略。将蒙版设置为新层时，新层必须具有 nil 超级层，否则行为是不确定的。不支持嵌套蒙版（具有自己的蒙版的蒙版层）。
#### doubleSided
&emsp;一个布尔值，指示当背离查看器时，图层是否显示其内容。可动画的。
```c++
@property(getter=isDoubleSided) BOOL doubleSided;
```
&emsp;当此属性的值为 NO 时，该层将背对查看器时隐藏其内容。此属性的默认值为 YES。

/* When false layers facing away from the viewer are hidden from view. Defaults to YES. Animatable. */
当假面背对观察者时，将其隐藏起来。默认为 YES。可动画的。
#### cornerRadius
&emsp;为图层的背景绘制圆角时要使用的半径。可动画的。
```c++
@property CGFloat cornerRadius;
```
&emsp;将“半径”设置为大于0.0的值将导致图层开始在其背景上绘制圆角。默认情况下，角半径不应用于图层内容属性中的图像；它仅应用于图层的背景色和边框。但是，将masksToBounds属性设置为YES会将内容剪裁到圆角。

&emsp;此属性的默认值为 0.0。

/* When positive, the background of the layer will be drawn with rounded corners. Also effects the mask generated by the 'masksToBounds' property. Defaults to zero. Animatable. */
当为正时，将以圆角绘制图层的背景。还影响“ masksToBounds”属性生成的蒙版。默认为 zero。可动画的。
#### maskedCorners
&emsp;定义使用“ cornerRadius”属性时四个角中的哪个角接受遮罩。默认为所有四个角。
```c++
@property CACornerMask maskedCorners API_AVAILABLE(macos(10.13), ios(11.0), watchos(4.0), tvos(11.0));
```
/* Defines which of the four corners receives the masking when using 'cornerRadius' property. Defaults to all four corners. */
#### CACornerMask
&emsp;maskedCorners属性的位定义。
/* Bit definitions for `maskedCorners' property. */
```c++
typedef NS_OPTIONS (NSUInteger, CACornerMask)
{
  kCALayerMinXMinYCorner = 1U << 0,
  kCALayerMaxXMinYCorner = 1U << 1,
  kCALayerMinXMaxYCorner = 1U << 2,
  kCALayerMaxXMaxYCorner = 1U << 3,
};
```
#### borderWidth
&emsp;图层 border 的宽度。可动画的。
```c++
@property CGFloat borderWidth;
```
&emsp;当此值大于0.0时，图层将使用当前的borderColor值绘制边框。边框是根据此属性中指定的值从接收者的边界绘制的。它在接收者的内容和子层之上进行了合成，并包含cornerRadius属性的效果。

&emsp;此属性的默认值为 0.0。

/* The width of the layer's border, inset from the layer bounds. The border is composited above the layer's content and sublayers and includes the effects of the 'cornerRadius' property. Defaults to zero. Animatable. */
图层边界的宽度，从图层边界插入。边框在图层内容和子图层上方合成，并包含“ cornerRadius”属性的效果。默认为零。可动画的。
#### borderColor
&emsp;图层边框的颜色。可动画的。
```c++
@property(nullable) CGColorRef borderColor;
```
&emsp;此属性的默认值为不透明的黑色。

&emsp;使用Core Foundation保留/释放语义保留此属性的值。尽管该属性声明似乎使用默认的assign语义进行对象保留，但仍会发生此行为。

/* The color of the layer's border. Defaults to opaque black. Colors created from tiled patterns are supported. Animatable. */
图层边框的颜色。默认为不透明黑色。支持从平铺模式创建的颜色。可动画的。

#### backgroundColor
&emsp;接收器的背景色。可动画的。
```c++
@property(nullable) CGColorRef backgroundColor;
```
&emsp;此属性的默认值为nil。

&emsp;使用Core Foundation保留/释放语义保留此属性的值。尽管该属性声明似乎使用默认的assign语义进行对象保留，但仍会发生此行为。

/* The background color of the layer. Default value is nil. Colors created from tiled patterns are supported. Animatable. */
图层的背景色。默认值为零。支持从平铺模式创建的颜色。可动画的。
#### shadowOpacity
&emsp;图层阴影的不透明度。可动画的。
```c++
@property float shadowOpacity;
```
&emsp;此属性中的值必须在0.0（透明）到1.0（不透明）之间。此属性的默认值为0.0。

/* The opacity of the shadow. Defaults to 0. Specifying a value outside the [0,1] range will give undefined results. Animatable. */
阴影的不透明度。默认值为 0。指定[0,1]范围以外的值将得到不确定的结果。可动画的。
#### shadowRadius
&emsp;用于渲染图层阴影的模糊半径（以磅为单位）。可动画的。
```c++
@property CGFloat shadowRadius;
```
&emsp;指定半径此属性的默认值为3.0。

/* The blur radius used to create the shadow. Defaults to 3. Animatable. */
用于创建阴影的模糊半径。默认值为 3。可设置动画。
#### shadowOffset
&emsp;图层阴影的偏移量（以磅为单位）。可动画的。
```c++
@property CGSize shadowOffset;
```
&emsp;此属性的默认值为（0.0，-3.0）。

/* The shadow offset. Defaults to (0, -3). Animatable. */
阴影偏移。默认为（0，-3）可动画的。
#### shadowColor
&emsp;图层阴影的颜色。可动画的。
```c++
@property(nullable) CGColorRef shadowColor;
```
&emsp;图层阴影的颜色。可动画的。

&emsp;使用 Core Foundation 保留/释放语义保留此属性的值。尽管该属性声明似乎使用默认的assign语义进行对象保留，但仍会发生此行为。

/* The color of the shadow. Defaults to opaque black. Colors created from patterns are currently NOT supported. Animatable. */
阴影的颜色。默认为不透明黑色。当前不支持从图案创建的颜色。可动画的。
#### shadowPath
&emsp;图层阴影的形状。可动画的。
```c++
@property(nullable) CGPathRef shadowPath;
```
&emsp;此属性的默认值为 nil，这会导致层使用标准阴影形状。如果为此属性指定值，则层将使用指定的路径而不是层的合成 alpha 通道创建其阴影。你提供的路径定义了阴影的轮廓。使用非零缠绕规则和当前阴影颜色、不透明度和模糊半径填充。

&emsp;与大多数可设置动画的属性不同，此属性（与所有CGPathRef可设置动画的属性一样）不支持隐式动画。但是，可以使用CAPropertyAnimation的任何具体子类为路径对象设置动画。路径将插值为“在线”点的线性混合；“离线”点可以非线性插值（以保持曲线导数的连续性）。如果两条路径具有不同数量的控制点或段，则结果未定义。如果路径延伸到层边界之外，它将不会自动剪裁到层，只有在正常的层掩蔽规则导致这种情况时。

&emsp;指定显式路径通常可以提高渲染性能。

&emsp;使用 Core Foundation 保留/释放语义保留此属性的值。尽管该属性声明似乎使用默认的assign语义进行对象保留，但仍会发生此行为。

/* When non-null this path defines the outline used to construct the layer's shadow instead of using the layer's composited alpha channel. The path is rendered using the non-zero winding rule. Specifying the path explicitly using this property will usually improve rendering performance, as will sharing the same path reference across multiple layers. Upon assignment the path is copied. Defaults to null. Animatable. */
当为非null时，此路径定义用于构造图层阴影的轮廓，而不是使用图层的合成Alpha通道。使用非零缠绕规则渲染路径。使用此属性显式指定路径通常可以提高渲染性能，因为可以在多个图层之间共享相同的路径引用。分配后，路径将被复制。默认为空。可动画的。

##### Using Shadow Path for Special Effects（使用阴影路径进行特殊效果）
&emsp;你可以使用图层的阴影路径来创建特殊效果，例如模拟 Pages 中可用的阴影。

&emsp;清单1 显示了将椭圆阴影添加到图层底部以模拟Pages Contact Shadow 效果所需的代码。

&emsp;Listing 1 Creating a contact shadow path
```c++
let layer = CALayer()
     
layer.frame = CGRect(x: 75, y: 75, width: 150, height: 150)
layer.backgroundColor = NSColor.darkGray.cgColor
layer.shadowColor = NSColor.gray.cgColor
layer.shadowRadius = 5
layer.shadowOpacity = 1
     
let contactShadowSize: CGFloat = 20
let shadowPath = CGPath(ellipseIn: CGRect(x: -contactShadowSize,
                                          y: -contactShadowSize * 0.5,
                                          width: layer.bounds.width + contactShadowSize * 2,
                                          height: contactShadowSize),
                        transform: nil)
     
layer.shadowPath = shadowPath
```
&emsp;Figure 1 Layer with contact shadow effect

![]()

&emsp;清单 2 显示了如何创建路径来模拟Pages Curved Shadow。路径的左侧，顶部和右侧是直线，而底部是凹曲线，如图2所示。

&emsp;Figure 2 Shadow path for curved shadow effect

![]()

&emsp;Listing 2 Creating a curved shadow path
```c++
let layer = CALayer()
layer.frame = CGRect(x: 75, y: 75, width: 150, height: 150)
layer.backgroundColor = NSColor.darkGray.cgColor
     
layer.shadowColor = NSColor.black.cgColor
layer.shadowRadius = 5
layer.shadowOpacity = 1
     
let shadowHeight: CGFloat = 10
let shadowPath = CGMutablePath()
shadowPath.move(to: CGPoint(x: layer.shadowRadius,
                            y: -shadowHeight))
shadowPath.addLine(to: CGPoint(x: layer.shadowRadius,
                               y: shadowHeight))
shadowPath.addLine(to: CGPoint(x: layer.bounds.width - layer.shadowRadius,
                               y: shadowHeight))
shadowPath.addLine(to: CGPoint(x: layer.bounds.width - layer.shadowRadius,
                               y: -shadowHeight))
     
shadowPath.addQuadCurve(to: CGPoint(x: layer.shadowRadius,
                                    y: -shadowHeight),
                        control: CGPoint(x: layer.bounds.width / 2,
                                         y: shadowHeight))
     
layer.shadowPath = shadowPath
```
&emsp;Figure 3 Layer with curved shadow effect

![]()

#### style
&emsp;可选字典，用于存储未由图层明确定义的属性值
```c++
@property(nullable, copy) NSDictionary *style;
```
&emsp;该字典又可以具有样式键，从而形成默认值的层次结构。如果是分层样式字典，则使用属性的最浅值。例如，“ style.someValue”的值优先于“ style.style.someValue”。

&emsp;如果样式字典未为属性定义值，则调用接收者的defaultValueForKey：方法。此属性的默认值为nil。

&emsp;下列关键字不参考样式词典：bounds，frame。

> &emsp;Warning: 如果修改了 style 字典或其任何祖先，则在重置样式属性之前，图层属性的值是不确定的。

/* When non-nil, a dictionary dereferenced to find property values that aren't explicitly defined by the layer. (This dictionary may in turn have a `style' property, forming a hierarchy of default values.) If the style dictionary doesn't define a value for an attribute, the +defaultValueForKey: method is called. Defaults to nil.
不为nil时，将取消引用字典以查找该层未明确定义的属性值。 （此字典可能又具有“样式”属性，形成默认值的层次结构。）如果样式字典未为属性定义值，则调用+ defaultValueForKey：方法。默认为零。

* Note that if the dictionary or any of its ancestors are modified, the values of the layer's properties are undefined until the 'style' property is reset. */
请注意，如果修改了字典或其任何祖先，则在重置“样式”属性之前，图层属性的值是不确定的。
#### allowsEdgeAntialiasing
&emsp;一个布尔值，指示是否允许该图层执行边缘抗锯齿。
```c++
@property BOOL allowsEdgeAntialiasing API_AVAILABLE(macos(10.10), ios(2.0), watchos(2.0), tvos(9.0));
```
&emsp;值为YES时，允许图层按照其edgeAntialiasingMask属性中的值要求对其边缘进行抗锯齿。默认值是从主捆绑包的Info.plist文件中的boolean UIViewEdgeAntialiasing属性读取的。如果未找到任何值，则默认值为NO。

/* When true this layer is allowed to antialias its edges, as requested by the value of the edgeAntialiasingMask property.
设置为true时，允许该层按照edgeAntialiasingMask属性值的要求对边缘进行抗锯齿。

* The default value is read from the boolean UIViewEdgeAntialiasing property in the main bundle's Info.plist. If no value is found in the Info.plist the default value is NO. */
从主包的Info.plist中的布尔UIViewEdgeAntialiasing属性读取默认值。如果在Info.plist中找不到值，则默认值为NO。
#### allowsGroupOpacity
&emsp;一个布尔值，指示是否允许该图层将自身与其父级分开组合为一个组。
```c++
@property BOOL allowsGroupOpacity API_AVAILABLE(macos(10.10), ios(2.0), watchos(2.0), tvos(9.0));
```
&emsp;当值为YES且图层的不透明度属性值小于1.0时，允许图层将其自身组合为与其父级分开的组。当图层包含多个不透明组件时，这会给出正确的结果，但可能会降低性能。

&emsp;默认值是从主捆绑包的Info.plist文件中的boolean UIViewGroupOpacity属性读取的。如果未找到任何值，则对与iOS 7 SDK或更高版本链接的应用程序的默认值为“YES”，对于与较早的SDK链接的应用程序的默认值为“否”。

/* When true, and the layer's opacity property is less than one, the layer is allowed to composite itself as a group separate from its parent. This gives the correct results when the layer contains multiple opaque components, but may reduce performance.
如果为 true，并且图层的不透明度属性小于1，则允许图层将其自身组合为与其父级分开的组。当图层包含多个不透明组件时，这将给出正确的结果，但可能会降低性能。

* The default value of the property is read from the boolean UIViewGroupOpacity property in the main bundle's Info.plist. If no value is found in the Info.plist the default value is YES for applications linked against the iOS 7 SDK or later and NO for applications linked against an earlier SDK. */
该属性的默认值是从主捆绑包的Info.plist中的布尔UIViewGroupOpacity属性读取的。如果在Info.plist中找不到值，则对于与iOS 7 SDK或更高版本链接的应用程序，默认值为YES；对于与早期SDK链接的应用程序，默认值为NO。
### Layer Filters
#### filters
&emsp;一组 Core Image 过滤器，可应用于图层及其子图层的内容。可动画的。
```c++
@property(nullable, copy) NSArray *filters;
```
&emsp;你添加到此属性的过滤器会影响图层的内容，包括其边框，填充的背景和子图层。此属性的默认值为nil。

&emsp;在 CIFilter 对象附加到层之后直接更改其输入会导致未定义的行为。可以在将过滤器参数附着到图层后修改过滤器参数，但必须使用图层的设置值：forKeyPath：执行此操作的方法。此外，必须为筛选器指定一个名称，以便在数组中标识它。例如，要更改过滤器的inputRadius参数，可以使用类似以下代码：
```c++
CIFilter *filter = ...;
CALayer *layer = ...;
 
filter.name = @"myFilter";
layer.filters = [NSArray arrayWithObject:filter];
[layer setValue:[NSNumber numberWithInt:1] forKeyPath:@"filters.myFilter.inputRadius"];
```
&emsp;iOS 中的图层不支持此属性。

/* An array of filters that will be applied to the contents of the layer and its sublayers. Defaults to nil. Animatable. */
一系列过滤器，将应用于该图层及其子图层的内容。默认为 nil。可动画的。
#### compositingFilter
&emsp;一个CoreImage滤镜，用于合成图层及其背后的内容。可动画的。
```c++
@property(nullable, strong) id compositingFilter;
```
&emsp;此属性的默认值为 nil，这将导致图层使用源覆盖合成。尽管你可以将任何 Core Image滤镜用作图层的合成滤镜，但为获得最佳效果，请使用 CICategoryCompositeOperation 类别中的滤镜。

&emsp;在 macOS 中，可以在将过滤器的参数附加到图层后对其进行修改，但是您必须使用该图层的setValue：forKeyPath：方法。例如，要更改过滤器的inputRadius参数，可以使用类似于以下代码：
```c++
CIFilter *filter = ...;
CALayer *layer = ...;
 
layer.compositingFilter = filter;
[layer setValue:[NSNumber numberWithInt:1] forKeyPath:@"compositingFilter.inputRadius"];
```
&emsp;在 CIFilter 对象附加到层之后直接更改其输入会导致未定义的行为。

&emsp;iOS 中的图层不支持此属性。

/* A filter object used to composite the layer with its (possibly filtered) background. Default value is nil, which implies source-over compositing. Animatable.
一个过滤器对象，用于将其背景（可能已过滤）合成层。默认值为 nil，这意味着源于合成。可动画的。

Note that if the inputs of the filter are modified directly after the filter is attached to a layer, the behavior is undefined. The filter must either be reattached to the layer, or filter properties should be modified by calling -setValue:forKeyPath: on each layer that the filter is attached to. (This also applies to the 'filters' and 'backgroundFilters' properties.) */
请注意，如果将过滤器附加到图层后直接修改了过滤器的输入，则行为是不确定的。过滤器必须重新连接到该层，或者应该通过在连接过滤器的每一层上调用-setValue：forKeyPath：来修改过滤器属性。 （这也适用于“过滤器”和“背景过滤器”属性。
#### backgroundFilters
&emsp;一组核心图像过滤器，可应用于紧靠该图层后面的内容。可动画的。
```c++
@property(nullable, copy) NSArray *backgroundFilters;
```
&emsp;背景过滤器会影响显示在图层本身中的图层后面的内容。通常，此内容属于充当该层父级的超层。这些滤镜不会影响图层本身的内容，包括图层的背景颜色和边框。

&emsp;此属性的默认值为nil。

&emsp;在CIFilter对象附加到层之后直接更改其输入会导致未定义的行为。在macOS中，可以在将过滤器参数附加到图层后修改过滤器参数，但必须使用图层的设置值：forKeyPath：执行此操作的方法。此外，必须为筛选器指定一个名称，以便在数组中标识它。例如，要更改过滤器的inputRadius参数，可以使用类似以下代码：
```c++
CIFilter *filter = ...;
CALayer *layer = ...;
 
filter.name = @"myFilter";
layer.backgroundFilters = [NSArray arrayWithObject:filter];
[layer setValue:[NSNumber numberWithInt:1] forKeyPath:@"backgroundFilters.myFilter.inputRadius"];
```
&emsp;你可以使用图层的masksToBounds来控制其背景滤镜效果的程度。

/* An array of filters that are applied to the background of the layer. The root layer ignores this property. Animatable. */
应用于图层背景的滤镜数组。根层将忽略此属性。可动画的。
#### minificationFilter
&emsp;减小内容大小时使用的过滤器。
```c++
@property(copy) CALayerContentsFilter minificationFilter;
```
&emsp;缩放过滤器中列出了此属性的可能值。

&emsp;此属性的默认值为kCAFilterLinear。

/* The filter types to use when rendering the 'contents' property of the layer. The minification filter is used when to reduce the size of image data, the magnification filter to increase the size of image data. Currently the allowed values are 'nearest' and 'linear'. Both properties default to 'linear'. */
呈现图层的“内容”属性时要使用的过滤器类型。缩小滤镜用于减小图像数据的大小，放大滤镜用于增大图像数据的大小。当前允许的值为“最近”和“线性”。这两个属性默认为“线性”。

#### minificationFilterBias
&emsp;缩小过滤器用来确定详细程度的偏差因子。
```c++
@property float minificationFilterBias;
```
&emsp;当将此值设置为kCAFilterTrilinear时，minificationFilter将使用此值。

&emsp;此属性的默认值为0.0。

/* The bias factor added when determining which levels of detail to use when minifying using trilinear filtering. The default value is 0. Animatable. */
在确定使用三线性过滤最小化时使用的细节级别时添加的偏差因子。默认值为 0。可设置动画。

#### magnificationFilter
&emsp;增加内容大小时使用的过滤器。
```c++
@property(copy) CALayerContentsFilter magnificationFilter;
```
&emsp;缩放过滤器中列出了此属性的可能值。

&emsp;此属性的默认值为kCAFilterLinear。

&emsp;图1显示了当将一个10 x 10点的圆图像放大10倍时，线性滤波和最近滤波之间的差异。

&emsp;Figure 1 Circle with different magnification filters

![]()

&emsp;左侧的圆圈使用kCAFilterLinear，右侧的圆圈使用kCAFilterNearest。
### Configuring the Layer’s Rendering Behavior
#### opaque
&emsp;一个布尔值，指示该图层是否包含完全不透明的内容。
```c++
@property(getter=isOpaque) BOOL opaque;
```
&emsp;此属性的默认值为“否”。如果应用程序绘制的内容完全不透明，并且填充了层的边界，则将此属性设置为“是”可使系统优化层的呈现行为。具体来说，当层为绘图命令创建备份存储时，核心动画会忽略该备份存储的alpha通道。这样做可以提高合成操作的性能。如果将此属性的值设置为“是”，则必须用不透明内容填充图层边界。

&emsp;设置此属性仅影响由 Core Animation 管理的后备存储。如果你将具有Alpha通道的图像分配给图层的content属性，则该图像将保留其Alpha通道，而不管该属性的值如何。

/* A hint marking that the layer contents provided by -drawInContext: is completely opaque. Defaults to NO. Note that this does not affect the interpretation of the 'contents' property directly. */
提示 -drawInContext：提供的图层内容是完全不透明的。默认为 NO。请注意，这不会直接影响对“ contents”属性的解释。

#### edgeAntialiasingMask
&emsp;定义如何光栅化接收器边缘的位掩码。
```c++
@property CAEdgeAntialiasingMask edgeAntialiasingMask;
```
&emsp;此属性指定层的哪些边缘被消除锯齿，并且是CAEdgeAntialiasingMask中定义的常量的组合。您可以分别为每个边缘（顶部，左侧，底部，右侧）启用或禁用抗锯齿。默认情况下，所有边缘均启用抗锯齿。

&emsp;通常，你将使用此属性为与其他层的边缘邻接的边缘禁用抗锯齿，以消除否则会发生的接缝。

/* Defines how the edges of the layer are rasterized. For each of the four edges (left, right, bottom, top) if the corresponding bit is set the edge will be antialiased. Typically this property is used to disable antialiasing for edges that abut edges of other layers, to eliminate the seams that would otherwise occur. The default value is for all edges to be antialiased. */
定义如何对图层的边缘进行栅格化。对于四个边缘中的每个边缘（左，右，底部，顶部），如果设置了相应的位，则将对边缘进行抗锯齿。通常，此属性用于禁用与其他层的边缘相邻的边缘的抗锯齿，以消除否则会发生的接缝。默认值是对所有边缘进行抗锯齿。
#### - contentsAreFlipped
&emsp;返回一个布尔值，指示在渲染时是否隐式翻转图层内容。
```c++
- (BOOL)contentsAreFlipped;
```
&emsp;Return Value: 如果在渲染时隐式翻转了图层内容，则为；否则为否。默认情况下，此方法返回NO。

&emsp;此方法提供有关在绘制过程中是否翻转图层内容的信息。您不应尝试覆盖此方法并返回其他值。

&emsp;如果图层需要翻转其内容，则它会从此方法返回YES，并将y-flip转换应用于图形上下文，然后再将其传递给图层的 drawInContext：方法。同样，该图层会将传递给其 setNeedsDisplayInRect：的所有矩形转换为翻转的坐标空间。

/* Returns true if the contents of the contents property of the layer will be implicitly flipped when rendered in relation to the local coordinate space (e.g. if there are an odd number of layers with flippedGeometry=YES from the receiver up to and including the implicit container of the root layer). Subclasses should not attempt to redefine this method. When this method returns true the CGContextRef object passed to -drawInContext: by the default -display method will have been y- flipped (and rectangles passed to -setNeedsDisplayInRect: will be similarly flipped). */
如果相对于本地坐标空间进行渲染时，如果层的内容属性的内容将被隐式翻转，则返回true（例如，如果从接收器到包含（包括）的隐式容器的层数为奇数，且 flippedGeometry = YES）根层）。子类不应尝试重新定义此方法。当此方法返回 true 时，默认情况下，传递给 -drawInContext：的 CGContextRef对象将被y-翻转（传递给 -setNeedsDisplayInRect：的矩形也将被相似地翻转）。
#### geometryFlipped
&emsp;一个布尔值，指示该层及其子层的几何形状是否垂直翻转。
```c++
@property(getter=isGeometryFlipped) BOOL geometryFlipped;
```
&emsp;如果层为层支持的视图提供支持，则该视图负责管理此属性中的值。对于独立图层，此属性控制是使用标准坐标系还是翻转坐标系来解释图层的几何值。此属性的值不影响图层内容的呈现。

&emsp;此属性的默认值为 NO。

/* Whether or not the geometry of the layer (and its sublayers) is flipped vertically. Defaults to NO. Note that even when geometry is flipped, image orientation remains the same (i.e. a CGImageRef stored in the `contents' property will display the same with both flipped=NO and flipped=YES, assuming no transform on the layer). */
图层（及其子图层）的几何形状是否垂直翻转。默认为 NO。请注意，即使在翻转几何图形时，图像方向也保持不变（即存储在``contents''属性中的 CGImageRef 将显示相同的内容，并假设在层上未进行任何变换时，flipd = NO 和 flipped = YES）。
#### drawsAsynchronously
&emsp;一个布尔值，指示是否在后台线程中延迟和异步处理绘制命令。
```c++
@property BOOL drawsAsynchronously API_AVAILABLE(macos(10.8), ios(6.0), watchos(2.0), tvos(9.0));
```
&emsp;当此属性设置为“是”时，用于绘制图层内容的图形上下文将对绘制命令进行排队，并在后台线程上执行这些命令，而不是同步执行这些命令。异步执行这些命令可以提高某些应用程序的性能。但是，在启用此功能之前，你应该始终衡量实际的性能优势。

&emsp;此属性的默认值为 NO。

/* When true, the CGContext object passed to the -drawInContext: method may queue the drawing commands submitted to it, such that they will be executed later (i.e. asynchronously to the execution of the -drawInContext: method). This may allow the layer to complete its drawing operations sooner than when executing synchronously. The default value is NO. */
如果为 true，则传递给 -drawInContext：方法的 CGContext 对象可以将提交给它的绘图命令排入队列，以便稍后执行它们（即，与-drawInContext：方法的执行异步）。这可以允许该层比同步执行时更快地完成其绘制操作。默认值为“否”。
#### shouldRasterize
&emsp;一个布尔值，指示在合成之前是否将图层渲染为位图。可动画的
```c++
@property BOOL shouldRasterize;
```
&emsp;当此属性的值为“是”时，层将在其局部坐标空间中渲染为位图，然后与任何其他内容合成到目标。阴影效果和“过滤器”属性中的任何过滤器都将光栅化并包含在位图中。但是，层的当前不透明度未光栅化。如果光栅化位图在合成过程中需要缩放，则会根据需要应用“缩小过滤器”和“放大过滤器”属性中的过滤器。

&emsp;如果此属性的值为 NO，则在可能的情况下将图层直接复合到目标中。如果合成模型的某些功能（例如包含滤镜）需要，则在合成之前仍可以对图层进行栅格化。

&emsp;此属性的默认值为 NO。

/* When true, the layer is rendered as a bitmap in its local coordinate space ("rasterized"), then the bitmap is composited into the destination (with the minificationFilter and magnificationFilter properties of the layer applied if the bitmap needs scaling). Rasterization occurs after the layer's filters and shadow effects are applied, but before the opacity modulation. As an implementation detail the rendering engine may attempt to cache and reuse the bitmap from one frame to the next. (Whether it does or not will have no affect on the rendered output.)
如果为 true，则在其局部坐标空间中将图层渲染为位图（“栅格化”），然后将位图组合到目标中（如果位图需要缩放，则应用图层的 minificationFilter 和 magnificationFilter 属性）。栅格化发生在应用图层的滤镜和阴影效果之后，但在不透明度调制之前。作为实现细节，渲染引擎可以尝试从一帧到下一帧缓存和重用位图。 （无论是否影响渲染的输出。）

* When false the layer is composited directly into the destination whenever possible (however, certain features of the compositing model may force rasterization, e.g. adding filters).
如果为 false，则在可能的情况下将图层直接合成到目标中（但是，合成模型的某些功能可能会强制进行栅格化，例如添加滤镜）。

* Defaults to NO. Animatable. */
默认为NO。可动画的。

#### rasterizationScale
&emsp;相对于图层的坐标空间栅格化内容的比例。可动画的。
```c++
@property CGFloat rasterizationScale;
```
&emsp;当shouldRasterize属性中的值为YES时，图层将使用此属性来确定是否缩放栅格化的内容（以及缩放多少）。此属性的默认值为1.0，这表示应以当前大小对其进行栅格化。较大的值将放大内容，较小的值将缩小内容。

/* The scale at which the layer will be rasterized (when the shouldRasterize property has been set to YES) relative to the coordinate space of the layer. Defaults to one. Animatable. */
相对于图层的坐标空间进行图层栅格化的比例（将 shouldRasterize 属性设置为 YES时）。默认为 1。可动画的。
#### contentsFormat
&emsp;有关所需的图层内容存储格式的提示。
```c++
@property(copy) CALayerContentsFormat contentsFormat API_AVAILABLE(macos(10.12), ios(10.0), watchos(3.0), tvos(10.0));
```
&emsp;此属性的默认值为kCAContentsFormatRGBA8Uint。

&emsp;UIView 和层备份 NSView 对象可能会将值更改为适合当前设备的格式。

/* A hint for the desired storage format of the layer contents provided by -drawLayerInContext. Defaults to kCAContentsFormatRGBA8Uint. Note that this does not affect the interpretation of the `contents' property directly. */
-drawLayerInContext 提供的层内容的所需存储格式的提示。默认为 kCAContentsFormatRGBA8Uint。注意，这不会直接影响对“ contents”属性的解释。
#### - renderInContext:
&emsp;将图层及其子图层渲染​​到指定的上下文中。
```c++
- (void)renderInContext:(CGContextRef)ctx;
```
&emsp;`ctx`: 用于渲染图层的图形上下文。

&emsp;此方法直接从图层树进行渲染，而忽略添加到渲染树的所有动画。在图层的坐标空间中渲染。

> &emsp;Important: 此方法的 OS X v10.5实现不支持整个Core Animation合成模型。不呈现QCCompositionLayer，CAOpenGLLayer和QTMovieLayer图层。此外，不会渲染使用3D变换的图层，也不会渲染指定backgroundFilters，filters，compositingFilter或mask值的图层。未来的macOS版本可能会增加对渲染这些图层和属性的支持。

/* Renders the receiver and its sublayers into 'ctx'. This method renders directly from the layer tree. Renders in the coordinate space of the layer.

* WARNING: currently this method does not implement the full CoreAnimation composition model, use with caution. */
将接收器及其子层渲染为“ ctx”。此方法直接从图层树渲染。在图层的坐标空间中渲染。
警告：当前此方法未实现完整的CoreAnimation合成模型，请谨慎使用。

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




## 参考链接
**参考链接:🔗**
+ [CALayer](https://developer.apple.com/documentation/quartzcore/calayer?language=objc)
+ [Core Animation Programming Guide](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/CoreAnimation_guide/Introduction/Introduction.html#//apple_ref/doc/uid/TP40004514-CH1-SW1)
