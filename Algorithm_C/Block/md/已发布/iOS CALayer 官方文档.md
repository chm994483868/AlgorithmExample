# iOS CALayer 官方文档

## CAMediaTiming
&emsp;CAMediaTiming 协议由图层（CALayer）和动画（CAAnimation）实现，它为分层计时系统（hierarchical timing system）建模，每个对象描述了从其父对象的时间值到本地时间的映射。

&emsp;绝对时间被定义为 mach time 转换成秒。为了方便查询当前绝对时间，提供了 CACurrentMediaTime 函数。

&emsp;从父时间到本地时间的转换分为两个阶段：
1. 转换为 “活动的本地时间”（active local time）。这包括对象在父时间轴中出现的时间点，以及它相对于父级播放的速度。
2. 从活动时间转换为 “基本本地时间”（basic local time）。时序模型允许对象重复其基本持续时间多次，并可以选择在重复播放之前进行播放。

```c++
@protocol CAMediaTiming
...
@end
```
### CACurrentMediaTime
&emsp;返回当前的绝对时间，以秒为单位。
```c++
CFTimeInterval CACurrentMediaTime(void);
```
&emsp;Return Value: 通过调用 mach_absolute_time() 并将结果转换为秒而得出的 CFTimeInterval。

### Animation Start Time（动画开始时间）
#### beginTime
&emsp;指定 receiver 相对于其父对象的开始时间（如果适用），预设为 0。
```c++
/* The begin time of the object, in relation to its parent object, if applicable. Defaults to 0. */
@property CFTimeInterval beginTime;
```
&emsp;对象的开始时间（相对于其父对象）（如果适用）。预设为 0。
#### timeOffset
&emsp;指定活动的本地时间中的附加时间偏移，预设为 0。
```c++
/* Additional offset in active local time. 
 * i.e. to convert from parent time tp to active local time t: t = (tp - begin) * speed + offset.
 * One use of this is to "pause" a layer by setting 'speed' to zero and 'offset' to a suitable value. 
 * Defaults to 0. 
 */
@property CFTimeInterval timeOffset;
```
&emsp;活动的本地时间增加的偏移量。例如；从父时间 tp 转换为活动的本地时间 t：t = (tp - begin) * speed + offset。一种用法是通过将 `speed` 设置为零并将 offset 设置为合适的值来暂停（"pause"）layer。预设为 0。

### Repeating Animations（重复动画）
#### repeatCount
&emsp;确定动画将重复的次数。
```c++
/* The repeat count of the object. May be fractional. Defaults to 0. */
@property float repeatCount;
```
&emsp;可能是分数（类型是 float）。如果 repeatCount 为 0，则将其忽略。预设值为 0。如果同时指定了 repeatDuration 和 repeatCount，则行为未定义。

#### repeatDuration
&emsp;确定动画将重复多少秒。（对象的重复持续时间。预设为 0。）
```c++
/* The repeat duration of the object. Defaults to 0. */
@property CFTimeInterval repeatDuration;
```
&emsp;预设值为 0。如果 repeatDuration 为 0，则将其忽略。如果同时指定了 repeatDuration 和 repeatCount，则行为是不确定的。

### Duration and Speed（持续时间和速度）
#### duration
&emsp;指定动画的基本持续时间（以秒为单位），默认为 0。
```c++
/* The basic duration of the object. Defaults to 0. */
@property CFTimeInterval duration;
```
&emsp;对象的基本持续时间。预设为 0。

#### speed
&emsp;指定时间如何从父时间空间映射到 receiver 的时间空间。
```c++
/* The rate of the layer. Used to scale parent time to local time, 
 * e.g. if rate is 2, local time progresses twice as fast as parent time.
 * Defaults to 1. 
 */
@property float speed;
```
&emsp;例如，如果 speed 为 2.0，则本地时间的进度是父时间的两倍。默认为 1.0。

&emsp;layer 的速率。用于将父时间缩放为本地时间，例如如果比率为 2，则本地时间的进度是父时间的两倍。
### Playback Modes（播放模式）
#### autoreverses
&emsp;确定 receiver 在完成时是否反向播放。
```c++
/* When true, the object plays backwards after playing forwards. Defaults to NO. */
@property BOOL autoreverses;
```
&emsp;如果为 true，则对象在向前播放后向后播放。默认为 NO。
#### fillMode
&emsp;确定 receiver 的 presentation 在其有效期限完成后是否被冻结或删除。可能的值在 Fill Modes 中说明。默认值为 kCAFillModeRemoved。
```c++
/* Defines how the timed object behaves outside its active duration.
 * Local time may be clamped to either end of the active duration, 
 * or the element may be removed from the presentation. 
 * The legal values are 'backwards', 'forwards', 'both' and 'removed'. 
 * Defaults to 'removed'. 
 */
@property(copy) CAMediaTimingFillMode fillMode;
```
&emsp;定义 timed object 在其活动持续时间之外的行为。本地时间可以固定在活动持续时间的任一端，或者可以从 presentation 中删除该元素。合法值是 backwards、forwards、both、和 removed。默认为 removed。
#### Fill Modes
&emsp;这些常数确定了 timed object 的活动持续时间完成后的行为。它们与 fillMode 属性一起使用。
```c++
typedef NSString * CAMediaTimingFillMode NS_TYPED_ENUM;
/* `fillMode' options. */

CA_EXTERN CAMediaTimingFillMode const kCAFillModeForwards   API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
CA_EXTERN CAMediaTimingFillMode const kCAFillModeBackwards  API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
CA_EXTERN CAMediaTimingFillMode const kCAFillModeBoth       API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
CA_EXTERN CAMediaTimingFillMode const kCAFillModeRemoved    API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));
```

+ kCAFillModeForwards: 动画完成后，receiver 在其最终状态下仍然可见。
+ kCAFillModeBackwards: The receiver clamps values before zero to zero when the animation is completed.
+ kCAFillModeBoth: receiver 将值固定在对象时间空间的两端。
+ kCAFillModeRemoved: 动画完成后，receiver 将从 presentation 中删除。

## CALayerDelegate
&emsp;CALayer 的 delegate 对象需要遵循此协议，以响应与 CALayer 相关的事件。
```c++
@protocol CALayerDelegate <NSObject>
...
@end
```
&emsp;你可以实施此协议的方法来提供 CALayer 的内容，处理 sublayers 的布局以及提供要执行的自定义动画动作（custom animation actions）。必须将实现此协议的对象分配给图层对象的委托属性。

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
#### frame
&emsp;图层的 frame 矩形。
```c++
@property CGRect frame;
```
&emsp;frame 矩形是在上层坐标空间中指定的层的位置和大小。对于图层，框架矩形是从边界，anchorPoint和position属性中的值派生的计算属性。为该属性分配新值时，图层将更改其位置和边界属性以匹配你指定的矩形。矩形中每个坐标的值以点为单位。

&emsp;如果 transform 属性应用的旋转变换不是90度的倍数，则不要设置 frame。

&emsp;有关框架，边界，anchorPoint和位置属性之间的关系的更多信息，请参见 Core Animation Programming Guide。

> &emsp;Note: frame 属性不能直接设置动画。相反，你应该为边界，anchorPoint和position属性的适当组合设置动画，以实现所需的结果。

/* Unlike NSView, each Layer in the hierarchy has an implicit frame rectangle, a function of the 'position', 'bounds', 'anchorPoint', and 'transform' properties. When setting the frame the 'position' and 'bounds.size' are changed to match the given frame. */
与NSView不同，层次结构中的每个图层都有一个隐式 frame 矩形，它是“位置”，“边界”，“ anchorPoint”和“ transform”属性的函数。设置框架时，将更改“ position”和“ bounds.size”以匹配给定 frame。
#### bounds
&emsp;图层的边界矩形。可动画的。
```c++
@property CGRect bounds;
```
&emsp;边界矩形是图层在其自身坐标空间中的原点和大小。创建新的独立图层时，此属性的默认值为一个空矩形，你必须在使用该图层之前对其进行更改。矩形中每个坐标的值以点为单位。

&emsp;有关框架，边界，anchorPoint和位置属性之间的关系的更多信息，请参见 Core Animation Programming Guide。

/* The bounds of the layer. Defaults to CGRectZero. Animatable. */
图层的边界。默认为CGRectZero。可动画的。
#### position
&emsp;图层在其上层坐标空间中的位置。可动画的。
```c++
@property CGPoint position;
```
&emsp;此属性的值以磅为单位指定，并且始终相对于anchorPoint属性中的值指定。对于新的独立图层，默认位置设置为（0.0，0.0）。更改frame属性也会更新此属性中的值。

&emsp;有关框架，边界，anchorPoint和位置属性之间的关系的更多信息，请参见 Core Animation Programming Guide。

/* The position in the superlayer that the anchor point of the layer's bounds rect is aligned to. Defaults to the zero point. Animatable. */
层边界的定位点rect对准的上层位置。默认为零。可动画的。
#### zPosition
&emsp;图层在 z 轴上的位置。可动画的。
```c++
@property CGFloat zPosition;
```
&emsp;该属性的默认值为 0。更改此属性的值将更改屏幕上各图层的前后顺序。较高的值比较低的值在视觉上更靠近该图层。这会影响框架矩形重叠的图层的可见性。

&emsp;此属性的值以点为单位。此属性的范围是单精度浮点-greatestFiniteMagnitude 到 greatFiniteMagnitude。

/* The Z component of the layer's position in its superlayer. Defaults to zero. Animatable. */
图层在其上层中位置的Z分量。默认为零。可动画的。
#### anchorPointZ
&emsp;图层沿 z 轴位置的锚点。可动画的。
```c++
@property CGFloat anchorPointZ;
```
&emsp;此属性指定围绕 z 轴进行几何操作的锚点。该点表示为沿z轴的距离（以点为单位）。此属性的默认值为0。

/* The Z component of the layer's anchor point (i.e. reference point for position and transform). Defaults to zero. Animatable. */
图层锚点（即位置和变换的参考点）的 Z 分量。默认为零。可动画的。
#### anchorPoint
&emsp;定义图层边界矩形的锚点。可动画的。
```c++
@property CGPoint anchorPoint;
```
&emsp;你可以使用单位坐标空间为此属性指定值。此属性的默认值为（0.5，0.5），它表示图层边界矩形的中心。视图的所有几何操作都在指定点附近发生。例如，对具有默认锚点的图层应用旋转变换会导致该图层绕其中心旋转。将锚点更改到其他位置将导致图层围绕该新点旋转。

&emsp;有关框架，边界，anchorPoint 和位置属性之间的关系的更多信息，请参见 Core Animation Programming Guide。

/* Defines the anchor point of the layer's bounds rect, as a point in normalized layer coordinates - '(0, 0)' is the bottom left corner of the bounds rect, '(1, 1)' is the top right corner. Defaults to '(0.5, 0.5)', i.e. the center of the bounds rect. Animatable. */
定义图层边界 rect 的锚点，作为归一化图层坐标中的点-“（0，0）”是边界rect的左下角，“（1，1）”是右上角。默认为'（0.5，0.5）'，即边界rect的中心。可动画的。
#### contentsScale
&emsp;应用于图层的比例因子。
```c++
@property CGFloat contentsScale API_AVAILABLE(macos(10.7), ios(4.0), watchos(2.0), tvos(9.0));
```
&emsp;此值定义层的逻辑坐标空间（以点为单位）和物理坐标空间（以像素为单位）之间的映射。比例因子越高，表示渲染时该层中的每个点都由一个以上的像素表示。例如，如果比例因子为2.0，并且图层边界为50 x 50点，则用于显示图层内容的位图大小为100 x 100像素。

&emsp;此属性的默认值为 1.0。对于附加到视图的图层，视图将比例因子自动更改为适合当前屏幕的值。对于你自己创建和管理的图层，你必须根据屏幕的分辨率和所提供的内容自行设置此属性的值。 Core Animation 使用你指定的值作为提示来确定如何呈现内容。

/* Defines the scale factor applied to the contents of the layer. If the physical size of the contents is '(w, h)' then the logical size (i.e. for contentsGravity calculations) is defined as '(w /contentsScale, h / contentsScale)'. Applies to both images provided explicitly and content provided via -drawInContext: (i.e. if contentsScale is two -drawInContext: will draw into a buffer twice as large as the layer bounds). Defaults to one. Animatable. */
定义应用于图层内容的比例因子。如果内容的物理大小为'（w，h）'，则逻辑大小（即用于 contentGravity 计算）定义为'（w / contentsScale，h / contentsScale）'。适用于显式提供的图像和通过 -drawInContext 提供的内容：（即，如果 contentScale 为两个，则 -drawInContext：将绘制为两倍于图层边界的缓冲区）。默认为1。可动画的。
### Managing the Layer’s Transform
#### transform
&emsp;转换应用于图层的内容。可动画的。
```c++
@property CATransform3D transform;
```
&emsp;默认情况下，此属性设置为标识转换。你应用于图层的所有变换都相对于图层的锚点进行。

/* A transform applied to the layer relative to the anchor point of its bounds rect. Defaults to the identity transform. Animatable. */
相对于其边界 rect 的锚点应用于图层的变换。默认为身份转换。可动画的。
#### sublayerTransform
&emsp;指定在渲染时应用于子层的变换。可动画的。
```c++
@property CATransform3D sublayerTransform;
```
&emsp;通常，你可以使用此属性为嵌入的图层添加透视图和其他查看效果。你可以通过将子层变换设置为所需的投影矩阵来添加透视图。此属性的默认值为身份转换。

/* A transform applied to each member of the `sublayers' array while rendering its contents into the receiver's output. Typically used as the projection matrix to add perspective and other viewing effects into the model. Defaults to identity. Animatable. */
在将其内容呈现到接收器的输出中时，将变换应用于“子层”数组的每个成员。通常用作投影矩阵，以将透视图和其他查看效果添加到模型中。默认为身份。可动画的。
#### - affineTransform
&emsp;返回图层变换的仿射版本。
```c++
- (CGAffineTransform)affineTransform;
```
&emsp;仿射变换结构，对应于图层的 transform 属性中的值。

/* Convenience methods for accessing the `transform' property as an affine transform. */
作为仿射变换访问“ transform”属性的便捷方法。
#### - setAffineTransform:
&emsp;将图层的变换设置为指定的仿射变换。
```c++
- (void)setAffineTransform:(CGAffineTransform)m;
```
&emsp;`m`: 仿射变换，用于图层的变换。
### Managing the Layer Hierarchy
#### sublayers
&emsp;包含图层子图层的数组。
```c++
@property(nullable, copy) NSArray<__kindof CALayer *> *sublayers;
```
&emsp;子层以从前到后的顺序列出。此属性的默认值为nil。

&emsp;将sublayers属性设置为填充有图层对象的数组时，该数组中的每个图层都必须尚未具有超图层-也就是说，其超图层属性当前必须为nil。

/* The array of sublayers of this layer. The layers are listed in back to front order. Defaults to nil. When setting the value of the property, any newly added layers must have nil superlayers, otherwise the behavior is undefined. Note that the returned array is not guaranteed to retain its elements. */
此层子层的数组。层以从后到前的顺序列出。默认为 零。设置属性的值时，任何新添加的层都必须具有nil个超级层，否则行为是不确定的。请注意，不能保证返回的数组保留其元素。
#### superlayer
&emsp;层的上层。
```c++
@property(nullable, readonly) CALayer *superlayer;
```
&emsp;超级层管理其子层的布局。

/* The receiver's superlayer object. Implicitly changed to match the hierarchy described by the 'sublayers' properties. */
接收者的超层对象。隐式更改以匹配“ sublayers”属性描述的层次结构。
#### - addSublayer:
&emsp;将图层添加到图层的子图层列表中。
```c++
- (void)addSublayer:(CALayer *)layer;
```
&emsp;`layer`: 要添加的层。

&emsp;如果 sublayers 属性中的数组为nil，则调用此方法将为该属性创建一个数组，并将指定的图层添加到该数组。

/* Add 'layer' to the end of the receiver's sublayers array. If 'layer' already has a superlayer, it will be removed before being added. */
在接收器的子层数组的末尾添加“层”。如果“层”已经具有超层，则将其删除后再添加。
#### - removeFromSuperlayer
&emsp;从其父层分离该层。
```c++
- (void)removeFromSuperlayer;
```
&emsp;你可以使用此方法从图层层次结构中删除图层（及其所有子图层）。此方法会同时更新超级图层的子图层列表，并将该图层的超级图层属性设置为nil。

/* Removes the layer from its superlayer, works both if the receiver is in its superlayer's 'sublayers' array or set as its 'mask' value. */
从接收者的上层移除该层，如果接收者位于其上层的“子层”数组中或设置为“掩码”值，则两者均可工作。
#### - insertSublayer:atIndex:
&emsp;将指定的图层插入到指定索引处的接收者的子图层列表中。
```c++
- (void)insertSublayer:(CALayer *)layer atIndex:(unsigned)idx;
```
&emsp;`layer`: 要插入当前层的子层。`idx`: 插入图层的索引。此值必须是 sublayers 数组中基于0的有效索引。

/* Insert 'layer' at position 'idx' in the receiver's sublayers array. If 'layer' already has a superlayer, it will be removed before being inserted. */
在接收器的子图层数组中的“ idx”位置插入“图层”。如果“层”已经具有超层，则在插入之前将其删除。
#### - insertSublayer:below:
&emsp;将指定的子层插入已经属于接收方的另一个子层下。
```c++
- (void)insertSublayer:(CALayer *)layer below:(nullable CALayer *)sibling;
```
&emsp;`layer`: 要插入当前层的子层。`sibling`: 当前层中的现有子层。图层中的图层在子图层阵列中插入到该图层的前面，因此在外观上看起来是在其后面。

&emsp;如果子图层不在接收者的子图层数组中，则此方法会引发异常。

/* Insert 'layer' either above or below the specified layer in the receiver's sublayers array. If 'layer' already has a superlayer, it will be removed before being inserted. */
在接收器的 sublayers 数组中指定层的上方或下方插入“ layer”。如果“层”已经具有超层，则在插入之前将其删除。
#### - insertSublayer:above:
&emsp;将指定的子层插入到已经属于接收方的另一个子层之上。
```c++
- (void)insertSublayer:(CALayer *)layer above:(nullable CALayer *)sibling;
```
&emsp;`layer`: 要插入当前层的子层。`sibling`: 当前层中的现有子层。图层中的图层插入到子图层阵列中的该图层之后，因此从视觉上显示在其前面。
#### - replaceSublayer:with:
&emsp;将指定的子图层替换为其他图层对象。
```c++
- (void)replaceSublayer:(CALayer *)oldLayer with:(CALayer *)newLayer;
```
&emsp;`oldLayer`: 要替换的层。`newLayer`: 用来替换 oldLayer 的图层。

&emsp;如果 oldLayer 不在接收者的子图层数组中，则此方法的行为是不确定的。

/* Remove 'oldLayer' from the sublayers array of the receiver and insert 'newLayer' if non-nil in its position. If the superlayer of 'oldLayer' is not the receiver, the behavior is undefined. */
从接收器的子层数组中删除“ oldLayer”，并在其位置非零时插入“ newLayer”。如果“ oldLayer”的超层不是接收者，则行为是不确定的。
### Updating Layer Display
#### - setNeedsDisplay
&emsp;将图层的内容标记为需要更新。
```c++
- (void)setNeedsDisplay;
```
&emsp;调用此方法将导致图层重新缓存其内容。这导致该层可能调用其委托的displayLayer：或drawLayer：inContext：方法。图层的contents属性中的现有内容将被删除，以便为新内容腾出空间。

/* Marks that -display needs to be called before the layer is next committed. If a region is specified, only that region of the layer is invalidated. */
标记-display需要在下一次提交层之前被调用。如果指定了区域，则仅该层的该区域无效。
#### - setNeedsDisplayInRect:
&emsp;将指定矩形内的区域标记为需要更新。
```c++
- (void)setNeedsDisplayInRect:(CGRect)r;
```
&emsp;`r`: 标记为无效的图层的矩形区域。你必须在图层自己的坐标系中指定此矩形。
#### needsDisplayOnBoundsChange
&emsp;一个布尔值，指示当其边界矩形更改时是否必须更新图层内容。
```c++
@property BOOL needsDisplayOnBoundsChange;
```
/* When true -setNeedsDisplay will automatically be called when the bounds of the layer changes. Default value is NO. */
如果为 true，则在更改图层边界时将自动调用-setNeedsDisplay。默认值为“否”。
#### - displayIfNeeded
&emsp;如果图层当前被标记为需要更新，则启动该图层的更新过程。
```c++
- (void)displayIfNeeded;
```
&emsp;你可以根据需要调用此方法，以在正常更新周期之外强制对图层内容进行更新。但是，通常不需要这样做。更新图层的首选方法是调用setNeedsDisplay，并让系统在下一个周期更新图层。

/* Call -display if receiver is marked as needing redrawing. */
如果接收方被标记为需要重绘，则调用-display。
#### - needsDisplay
&emsp;返回一个布尔值，指示该图层是否已标记为需要更新。
```c++
- (BOOL)needsDisplay;
```
&emsp;是，如果需要更新图层。

/* Returns true when the layer is marked as needing redrawing. */
将图层标记为需要重绘时，返回true。
#### + needsDisplayForKey:
&emsp;返回一个布尔值，指示对指定键的更改是否需要重新显示该图层。
```c++
+ (BOOL)needsDisplayForKey:(NSString *)key;
```
&emsp;`key`: 一个字符串，它指定图层的属性。

&emsp;Return Value: 如果该图层需要重新显示，则为 YES。

&emsp;子类可以重写此方法，如果在指定属性的值更改时应重新显示该图层，则返回 YES。更改属性值的动画也会触发重新显示。

&emsp;此方法的默认实现返回 NO。

/* Method for subclasses to override. Returning true for a given property causes the layer's contents to be redrawn when the property is changed (including when changed by an animation attached to the layer). The default implementation returns NO. Subclasses should call super for properties defined by the superclass. (For example, do not try to return YES for properties implemented by CALayer, doing will have undefined results.) */
子类重写的方法。对于给定的属性，返回true会导致更改属性时（包括通过附加到该图层的动画进行更改时）重绘该图层的内容。默认实现返回NO。子类应为超类定义的属性调用超类。（例如，不要尝试对 CALayer 实现的属性返回YES，这样做会产生不确定的结果。）
### Layer Animations
#### - addAnimation:forKey:
&emsp;将指定的动画对象添加到图层的渲染树。
```c++
- (void)addAnimation:(CAAnimation *)anim forKey:(nullable NSString *)key;
```
&emsp;`anim`: 要添加到渲染树的动画。该对象由渲染树复制，未引用。因此，对对象的后续修改不会传播到渲染树中。`key`: 标识动画的字符串。每个唯一键仅将一个动画添加到该层。特殊键kCATransition自动用于过渡动画。你可以为此参数指定 nil。

&emsp;如果动画的duration属性为零或负，则将 duration 更改为 kCATransactionAnimationDuration 事务属性的当前值（如果已设置）或默认值为 0.25 秒。

/* Attach an animation object to the layer. Typically this is implicitly invoked through an action that is an CAAnimation object.
将动画对象附加到图层。通常，这是通过作为 CAAnimation 对象的操作隐式调用的。

* 'key' may be any string such that only one animation per unique key is added per layer. The special key 'transition' is automatically used for transition animations. The nil pointer is also a valid key.
“键”可以是任何字符串，因此每个唯一键每个图层仅添加一个动画。特殊键 “过渡” 会自动用于过渡动画。 nil 指针也是有效的键。

* If the `duration' property of the animation is zero or negative it is given the default duration, either the value of the 'animationDuration' transaction property or .25 seconds otherwise.
如果动画的“持续时间”属性为零或负数，则指定默认持续时间，否则为“ animationDuration”交易属性的值，否则为.25秒。

* The animation is copied before being added to the layer, so any subsequent modifications to `anim' will have no affect unless it is added to another layer. */
在将动画添加到图层之前先对其进行复制，因此，除非对动画进行任何后续修改，否则将其添加到另一层都不会产生影响。
#### - animationForKey:
&emsp;返回具有指定标识符的动画对象。
```c++
- (nullable __kindof CAAnimation *)animationForKey:(NSString *)key;
```
&emsp;`key`: 一个字符串，指定动画的标识符。该字符串对应于你传递给 addAnimation：forKey：方法的标识符字符串。

&emsp;Return Value: 匹配标识符的动画对象；如果不存在这样的动画，则为nil。

&emsp;你可以使用此字符串来检索已经与图层关联的动画对象。但是，你不得修改返回对象的任何属性。这样做将导致不确定的行为。

/* Returns the animation added to the layer with identifier 'key', or nil if no such animation exists. Attempting to modify any properties of the returned object will result in undefined behavior. */
返回添加到带有标识符“键”的层的动画；如果不存在这样的动画，则返回 nil。尝试修改返回对象的任何属性将导致未定义的行为。
#### - removeAllAnimations
&emsp;删除所有附加到该图层的动画。
```c++
- (void)removeAllAnimations;
```
/* Remove all animations attached to the layer. */
删除所有附加到该图层的动画。
#### - removeAnimationForKey:
&emsp;使用指定的关键帧删除动画对象。
```c++
- (void)removeAnimationForKey:(NSString *)key;
```
&emsp;`key`: 要删除的动画的标识符。

/* Remove any animation attached to the layer for 'key'. */
删除任何附加到“关键点”层的动画。
#### - animationKeys
&emsp;返回一个字符串数组，这些字符串标识当前附加到该图层的动画。
```c++
- (nullable NSArray<NSString *> *)animationKeys;
```
&emsp;Return Value: 标识当前动画的NSString对象数组。

&emsp;数组的顺序与将动画应用于图层的顺序匹配。

/* Returns an array containing the keys of all animations currently attached to the receiver. The order of the array matches the order in which animations will be applied. */
返回一个数组，其中包含当前附加到接收器的所有动画的关键点。数组的顺序与应用动画的顺序匹配。
### Managing Layer Resizing and Layout
#### layoutManager
&emsp;负责布置图层的子图层的对象。
```c++
@property(strong) id<CALayoutManager> layoutManager;
```
&emsp;你分配给此属性的对象必须名义上实现 CALayoutManager 非正式协议非正式协议。如果图层的代表不处理布局更新，则分配给此属性的对象将有机会更新图层的子图层的布局。

&emsp;在macOS中，如果您的图层使用基于图层的约束来处理布局更改，则将CAConstraintLayoutManager类的实例分配给此属性。

&emsp;此属性的默认值为nil。
#### - setNeedsLayout
&emsp;使图层的布局无效，并将其标记为需要更新。
```c++
- (void)setNeedsLayout;
```
&emsp;你可以调用此方法来指示图层的子图层的布局已更改，必须进行更新。通常，在更改图层边界或添加或删除子图层时，系统会自动调用此方法。在macOS中，如果你图层的layoutManager属性包含一个实现invalidateLayoutOfLayer：方法的对象，则也将调用该方法。

&emsp;在下一个更新周期中，系统将调用需要布局更新的任何图层的 layoutSublayers 方法。

/* Marks that -layoutSublayers needs to be invoked on the receiver before the next update. If the receiver's layout manager implements the -invalidateLayoutOfLayer: method it will be called.

* This method is automatically invoked on a layer whenever its 'sublayers' or `layoutManager' property is modified, and is invoked on the layer and its superlayer whenever its 'bounds' or 'transform' properties are modified. Implicit calls to -setNeedsLayout are skipped if the layer is currently executing its -layoutSublayers method. */
只要修改了“ sublayers”或“ layoutManager”属性，便会在该层上自动调用此方法，并且只要修改其“ bounds”或“ transform”属性，便会在该层及其上层上自动调用此方法。如果图层当前正在执行其-layoutSublayers方法，则将跳过对-setNeedsLayout的隐式调用。
#### - layoutSublayers
&emsp;告诉图层更新其布局。
```c++
- (void)layoutSublayers;
```
&emsp;子类可以重写此方法，并使用它来实现自己的布局算法。你的实现必须设置由接收器管理的每个子层的框架。

&emsp;此方法的默认实现调用该图层的委托对象的layoutSublayersOfLayer：方法。如果没有委托对象，或者委托没有实现该方法，则此方法在layoutManager属性中调用对象的layoutSublayersOfLayer：方法。

/* Called when the layer requires layout. The default implementation calls the layout manager if one exists and it implements the -layoutSublayersOfLayer: method. Subclasses can override this to provide their own layout algorithm, which should set the frame of each sublayer. */
在图层需要布局时调用。默认实现会调用布局管理器（如果存在的话），并且会实现-layoutSublayersOfLayer：方法。子类可以重写此方法以提供自己的布局算法，该算法应设置每个子层的框架。
#### - layoutIfNeeded
&emsp;如果需要，请重新计算接收器的布局。
```c++
- (void)layoutIfNeeded;
```
&emsp;收到此消息后，将遍历图层的超级图层，直到找到不需要布局的祖先图层。然后在该祖先下的整个层树上执行布局。

/* Traverse upwards from the layer while the superlayer requires layout. Then layout the entire tree beneath that ancestor. */
从图层向上遍历，而上层需要布局。然后将整个树布置在该祖先下。
#### - needsLayout
&emsp;返回一个布尔值，指示是否已将图层标记为需要布局更新
```c++
- (BOOL)needsLayout;
```
&emsp;如果已将图层标记为需要布局更新，则为 YES。

/* Returns true when the receiver is marked as needing layout. */
当接收方被标记为需要布局时，返回true。
#### autoresizingMask
&emsp;一个位掩码，用于定义当其上层边界更改时如何调整其大小。
```c++
@property CAAutoresizingMask autoresizingMask;
```
&emsp;如果您的应用未使用布局管理器或约束来处理布局更改，则可以为该属性分配一个值，以响应超级图层范围的更改来调整图层的大小。有关可能值的列表，请参见CAAutoresizingMask。

&emsp;此属性的默认值为kCALayerNotSizable。
#### - resizeWithOldSuperlayerSize:
&emsp;通知接收者其上层大小已更改。
```c++
- (void)resizeWithOldSuperlayerSize:(CGSize)size;
```
&emsp;`size`: 上层的先前大小。

&emsp;当autoresizingMask属性用于调整大小并且层的边界更改时，该层在其每个子层上调用此方法。子层使用此方法调整自己的帧矩形以反映新的超层边界，可以直接从超层检索。超层的旧大小被传递给这个方法，这样子层就有了它必须进行的任何计算所需的信息。
#### - resizeSublayersWithOldSize:
&emsp;通知接收者的子层接收者的尺寸已更改。
```c++
- (void)resizeSublayersWithOldSize:(CGSize)size;
```
&emsp;`size`: 当前图层的先前大小。

&emsp;当将autoresizingMask属性用于调整大小并且此层的边界发生变化时，该层将调用此方法。默认实现会调用每个子层的 resizeWithOldSuperlayerSize：方法，以使其知道其上层的边界已更改。你不需要直接调用或重写此方法。
#### - preferredFrameSize
&emsp;返回其上层坐标空间中该层的首选大小。
```c++
- (CGSize)preferredFrameSize;
```
&emsp;Return Value: 图层的首选帧大小。

&emsp;在macOS中，此方法的默认实现调用其布局管理器的preferredSizeOfLayer:方法，即layoutManager属性中的对象。如果该对象不存在或未实现该方法，则此方法返回映射到其超层坐标空间的层当前边界矩形的大小。

/* Returns the preferred frame size of the layer in the coordinate space of the superlayer. The default implementation calls the layout manager if one exists and it implements the -preferredSizeOfLayer: method, otherwise returns the size of the bounds rect mapped into the superlayer. */
返回在超级层的坐标空间中该层的首选帧大小。默认实现会调用布局管理器（如果存在的话），并且会实现-preferredSizeOfLayer：方法，否则返回映射到超层中的rect的大小。
### Managing Layer Constraints
#### constraints
&emsp;用于定位当前图层的子图层的约束。
```c++
@property(copy) NSArray<CAConstraint *> *constraints;
```
&emsp;macOS应用程序可以使用此属性来访问其基于层的约束。在应用约束之前，还必须将CAConstraintLayoutManager对象分配给图层的layoutManager属性。

&emsp;iOS 应用程序不支持基于图层的约束。
#### - addConstraint:
&emsp;将指定的约束添加到图层。
```c++
- (void)addConstraint:(CAConstraint *)c;
```
&emsp;`c`: 约束对象添加到接收者的约束对象数组中。

&emsp;在macOS中，通常向层添加约束以管理该层子层的大小和位置。在应用约束之前，还必须将CAConstraintLayoutManager对象指定给层的layoutManager属性。有关管理基于层的约束的详细信息，请参见  Core Animation Programming Guide.。

&emsp;iOS 应用程序不支持基于图层的约束。
### Getting the Layer’s Actions
#### - actionForKey:
&emsp;返回分配给指定键的操作对象。
```c++
- (nullable id<CAAction>)actionForKey:(NSString *)event;
```
&emsp;`event`: 动作的标识符。

&emsp;Return Value: 返回提供键操作的对象。该对象必须实现 CAAction 协议。

&emsp;此方法搜索层的给定动作对象。动作定义层的动态行为。例如，层的可设置动画的特性通常具有相应的动作对象来启动实际动画。当该属性更改时，层将查找与属性名称关联的动作对象并执行它。还可以将自定义动作对象与层关联，以实现特定于应用程序的动作。


/* Returns the action object associated with the event named by the string 'event'. The default implementation searches for an action object in the following places:
返回与由字符串“ event” 命名的事件关联的操作对象。默认实现在以下位置搜索动作对象：

* 1. if defined, call the delegate method -actionForLayer:forKey: 如果已定义，则调用委托方法-actionForLayer：forKey：
* 2. look in the layer's `actions' dictionary 看一下图层的“动作”字典
* 3. look in any `actions' dictionaries in the `style' hierarchy 查看“样式”层次结构中的所有“动作”字典
* 4. call +defaultActionForKey: on the layer's class 在图层的类上调用+ defaultActionForKey：
*
* If any of these steps results in a non-nil action object, the following steps are ignored. If the final result is an instance of NSNull, it is converted to 'nil'. */
如果这些步骤中的任何一个导致非空操作对象，则以下步骤将被忽略。如果最终结果是NSNull的实例，则将其转换为'nil'。
#### actions
&emsp;包含图层动作的字典。
```c++
@property(nullable, copy) NSDictionary<NSString *, id<CAAction>> *actions;
```
&emsp;此属性的默认值为 nil。你可以使用此字典存储图层的自定义操作。搜索该词典的内容，作为 actionForKey：方法的标准实现的一部分。

/* A dictionary mapping keys to objects implementing the CAAction protocol. Default value is nil. */
字典将键映射到实现 CAAction 协议的对象。默认值为 nil。
#### + defaultActionForKey:
&emsp;返回当前类的默认操作。
```c++
+ (nullable id<CAAction>)defaultActionForKey:(NSString *)event;
```
&emsp;`event`: 动作的标识符。

&emsp;Return Value: 返回给定键的合适操作对象，或者没有与该键关联的操作对象的 nil。

&emsp;想要提供默认动作的类可以重写此方法，并使用它返回那些动作。

/* An "action" is an object that responds to an "event" via the CAAction protocol (see below). Events are named using standard dot-separated key paths. Each layer defines a mapping from event key paths to action objects. Events are posted by looking up the action object associated with the key path and sending it the method defined by the CAAction protocol.
“动作”是通过CAAction协议响应“事件”的对象（请参见下文）。使用标准的点分隔键路径来命名事件。每一层都定义了从事件键路径到操作对象的映射。通过查找与键路径关联的操作对象并向其发送CAAction协议定义的方法，可以发布事件。
 
 * When an action object is invoked it receives three parameters: the key path naming the event, the object on which the event happened (i.e. the layer), and optionally a dictionary of named arguments specific to each event.
 调用动作对象时，它会接收三个参数：命名事件的键路径，发生事件的对象（即图层），以及可选的特定于每个事件的命名参数字典。
 
 * To provide implicit animations for layer properties, an event with the same name as each property is posted whenever the value of the property is modified. A suitable CAAnimation object is associated by default with each implicit event (CAAnimation implements the action protocol).
 为了为图层属性提供隐式动画，只要属性值被修改，就会发布一个与每个属性同名的事件。默认情况下，合适的 CAAnimation 对象与每个隐式事件关联（CAAnimation 实现动作协议）。
 
 * The layer class also defines the following events that are not linked directly to properties:
 图层类还定义了以下未直接链接到属性的事件：
 
 * onOrderIn
  Invoked when the layer is made visible, i.e. either its superlayer becomes visible, or it's added as a sublayer of a visible layer
  当该图层变为可见时调用，即该图层的上层变为可见，或将其添加为可见层的子层
 
 * onOrderOut
 Invoked when the layer becomes non-visible. 当图层变为不可见时调用。

/* Returns the default action object associated with the event named by the string 'event'. The default implementation returns a suitable animation object for events posted by animatable properties, nil otherwise.
返回与由字符串“ event”命名的事件关联的默认操作对象。默认实现为可动画属性发布的事件返回合适的动画对象，否则为nil。

### Mapping Between Coordinate and Time Spaces
#### - convertPoint:fromLayer:
&emsp;将点从指定图层的坐标系转换为接收者的坐标系。
```c++
- (CGPoint)convertPoint:(CGPoint)p fromLayer:(nullable CALayer *)l;
```
&emsp;`p`: 指定l坐标系中位置的点。`l`: 在其坐标系中具有p的层。接收者和l和必须共享一个公共父层。此参数可以为nil。

&emsp;Return Value: 该点将转换为接收者的坐标系。

&emsp;如果为l参数指定nil，则此方法返回从图层帧的原点减去的原始点。

#### - convertPoint:toLayer:
&emsp;将点从接收者的坐标系转换为指定图层的坐标系。
```c++
- (CGPoint)convertPoint:(CGPoint)p toLayer:(nullable CALayer *)l;
```
&emsp;`p`: 指定l坐标系中位置的点。`l`: 要将坐标系p转换为的图层。接收者和l必须共享一个公共父层。此参数可以为nil。

&emsp;Return Value: 点转换为图层的坐标系。

&emsp;如果为l参数指定nil，则此方法返回添加到图层框架原点的原始点。
#### - convertRect:fromLayer:
&emsp;将矩形从指定图层的坐标系转换为接收者的坐标系。
```c++
- (CGRect)convertRect:(CGRect)r fromLayer:(nullable CALayer *)l;
```
&emsp;`r`: 指定l坐标系中位置的点。`l`: 在其坐标系中具有r的图层。接收者和l和必须共享一个公共父层。此参数可以为nil。

&emsp;Return Value: 矩形将转换为接收者的坐标系。

&emsp;如果为l参数指定nil，则此方法将返回原始rect，其原点将从图层帧的原点中减去。
#### - convertRect:toLayer:
&emsp;将矩形从接收者的坐标系转换为指定图层的坐标系。
```c++
- (CGRect)convertRect:(CGRect)r toLayer:(nullable CALayer *)l;
```
&emsp;`r`: 指定l坐标系中位置的点。`l`: 要转换其坐标系r的图层。接收者和l和必须共享一个公共父层。此参数可以为nil。

&emsp;Return Value: 矩形转换为l的坐标系。

&emsp;如果为l参数指定nil，则此方法将返回原始rect，并将其原点添加到图层框架的原点。
#### - convertTime:fromLayer:
&emsp;将时间间隔从指定层的时间空间转换为接收者的时间空间。
```c++
- (CFTimeInterval)convertTime:(CFTimeInterval)t fromLayer:(nullable CALayer *)l;
```
&emsp;`t`: 将时间间隔从指定层的时间空间转换为接收者的时间空间。`l`: 时空为t的图层。接收者和l和必须共享一个公共父层。

&emsp;Return Value: 时间间隔转换为接收者的时间空间。
#### - convertTime:toLayer:
&emsp;将时间间隔从接收者的时间空间转换为指定层的时间空间
```c++
- (CFTimeInterval)convertTime:(CFTimeInterval)t toLayer:(nullable CALayer *)l;
```
&emsp;`t`: 指定l坐标系中位置的点。`l`: 要将时间空间t转换为该层。接收者和l和必须共享一个公共父层。

&emsp;时间间隔转换为图层的时间空间。
### Hit Testing
#### - hitTest:
&emsp;返回包含指定点的图层层次结构中接收者的最远后代（包括自身）。
```c++
- (nullable __kindof CALayer *)hitTest:(CGPoint)p;
```
&emsp;`p`: 接收者的上层坐标系中的一点。

&emsp;Return Value: 包含 thePoint的图层；如果该点位于接收者的边界矩形之外，则为nil。

/* Returns the farthest descendant of the layer containing point 'p'. Siblings are searched in top-to-bottom order. 'p' is defined to be in the coordinate space of the receiver's nearest ancestor that isn't a CATransformLayer (transform layers don't have a 2D coordinate space in which the point could be specified). */
返回包含点“ p”的层的最远后代。兄弟姐妹以自上而下的顺序搜索。 “ p”被定义为位于接收者的最近祖先的坐标空间中，该坐标空间不是CATransformLayer（转换层没有可以在其中指定点的2D坐标空间）。

#### - containsPoint:
&emsp;返回接收方是否包含指定点。
```c++
- (BOOL)containsPoint:(CGPoint)p;
```
&emsp;`p`: 接收者坐标系中的一个点。

/* Returns true if the bounds of the layer contains point 'p'. */
如果图层的边界包含点“ p”，则返回true。
### Scrolling
#### visibleRect
&emsp;图层在其自己的坐标空间中的可见区域。
```c++
@property(readonly) CGRect visibleRect;
```
&emsp;可见区域是未被包含的滚动层剪切的区域。
#### - scrollPoint:
&emsp;在该层的最接近的祖先滚动层中启动滚动，以使指定点位于滚动层的原点。
```c++
- (void)scrollPoint:(CGPoint)p;
```
&emsp;`p`: 当前图层中应滚动到位置的点。

&emsp;如果 CAScrollLayer 对象不包含该图层，则此方法不执行任何操作。
#### - scrollRectToVisible:
&emsp;在该图层的最接近的祖先滚动图层中启动滚动，以使指定的矩形变为可见。
```c++
- (void)scrollRectToVisible:(CGRect)r;
```
&emsp;`r`: 要显示的矩形。

&emsp;如果 CAScrollLayer 对象不包含该图层，则此方法不执行任何操作。
### Identifying the Layer
#### name
&emsp;接收者的名字。
```c++
@property(nullable, copy) NSString *name;
```
&emsp;某些布局管理器使用图层名称来标识图层。此属性的默认值为 nil。

/* The name of the layer. Used by some layout managers. Defaults to nil. */
图层的名称。由某些布局管理器使用。默认为零。
### Key-Value Coding Extensions
#### - shouldArchiveValueForKey:
&emsp;返回一个布尔值，指示是否应归档指定键的值。
```c++
- (BOOL)shouldArchiveValueForKey:(NSString *)key;
```
&emsp;`key`: 收件人属性之一的名称。

&emsp;Return Value: 如果应将指定的属性归档，则为YES；否则，则为NO。

&emsp;默认实现返回 YES。

/* Called by the object's implementation of -encodeWithCoder:, returns false if the named property should not be archived. The base implementation returns YES. Subclasses should call super for unknown properties. */
由对象的-encodeWithCoder：的实现调用，如果不应存储命名属性，则返回false。基本实现返回YES。子类应为未知属性调用super。
#### + defaultValueForKey:
&emsp;指定与指定键关联的默认值。
```c++
+ (nullable id)defaultValueForKey:(NSString *)key;
```
&emsp;`key`: 收件人属性之一的名称。

&emsp;Return Value: 命名属性的默认值。如果未设置默认值，则返回nil。

&emsp;如果为图层定义自定义特性，但未设置值，则此方法将基于键的预期值返回适当的“零”默认值。例如，如果key的值是CGSize结构，则该方法返回一个包含（0.0,0.0）的大小结构，该结构封装在NSValue对象中。对于CGRect，返回一个空矩形。对于CGAffineTransform和CATransform3D，将返回相应的单位矩阵。

&emsp;如果key对于该类的属性未知，则该方法的结果不确定。

/* CALayer implements the standard NSKeyValueCoding protocol for all Objective C properties defined by the class and its subclasses. It dynamically implements missing accessor methods for properties declared by subclasses.
CALayer为该类及其子类定义的所有Objective C属性实现标准的NSKeyValueCoding协议。它为子类声明的属性动态实现缺少的访问器方法。
 
When accessing properties via KVC whose values are not objects, the standard KVC wrapping conventions are used, with extensions to support the following types:
通过值不是对象的KVC访问属性时，将使用标准的KVC包装约定，并带有扩展以支持以下类型：

C Type                  Class
------                  -----
CGPoint                 NSValue
CGSize                  NSValue
CGRect                  NSValue
CGAffineTransform       NSValue
CATransform3D           NSValue  */

/* Returns the default value of the named property, or nil if no default value is known. Subclasses that override this method to define default values for their own properties should call 'super' for unknown properties. */
返回指定属性的默认值，如果没有默认值，则返回 nil。重写此方法为自己的属性定义默认值的子类应为未知属性调用“ super”。

### Constants
#### CAAutoresizingMask
&emsp;这些常量由autoresizingMask属性使用。
```c++
typedef enum CAAutoresizingMask : unsigned int {
    ...
} CAAutoresizingMask;
```
+ kCALayerNotSizable = 0: 接收器无法调整大小。
+ kCALayerMinXMargin = 1U << 0: 接收者及其超级视图之间的左边界是灵活的。
+ kCALayerWidthSizable = 1U << 1: 接收器的宽度很灵活。
+ kCALayerMaxXMargin = 1U << 2: 接收者及其超级视图之间的右边距是灵活的。
+ kCALayerMinYMargin = 1U << 3: 接收器及其超级视图之间的底部边距很灵活。
+ kCALayerHeightSizable = 1U << 4: 接收器的高度是灵活的。
+ kCALayerMaxYMargin = 1U << 5: 接收者及其超级视图之间的上边界是灵活的。

#### Action Identifiers
&emsp;这些常量是actionForKey:，add使用的预定义操作标识符动画：福基：、defaultActionForKey:、removeAnimationForKey:、层筛选器和CAAction协议方法runActionForKey:对象:论据：。
```c++
NSString *const kCAOnOrderIn;
NSString *const kCAOnOrderOut;
NSString *const kCATransition;
```

+ kCAOnOrderIn: 表示当某个图层变为可见时（由于将结果插入可见图层层次结构或将该图层不再设置为隐藏）而采取的操作的标识符。
+ kCAOnOrderOut: 标识符，表示从图层层次结构中删除图层或隐藏图层时所采取的操作。
+ kCATransition: 代表过渡动画的标识符。

#### CAEdgeAntialiasingMask
&emsp;edgeAntialiasingMask属性使用此蒙版。
```c++
typedef NS_OPTIONS (unsigned int, CAEdgeAntialiasingMask)
{
  kCALayerLeftEdge      = 1U << 0,      /* Minimum X edge. */
  kCALayerRightEdge     = 1U << 1,      /* Maximum X edge. */
  kCALayerBottomEdge    = 1U << 2,      /* Minimum Y edge. */
  kCALayerTopEdge       = 1U << 3,      /* Maximum Y edge. */
};
```
+ kCALayerLeftEdge: 指定应该对接收者内容的左边缘进行抗锯齿处理。
+ kCALayerRightEdge: 指定应该对接收者内容的右边缘进行锯齿处理。
+ kCALayerBottomEdge: 指定应该对接收者内容的底部边缘进行锯齿处理。
+ kCALayerTopEdge: 指定应该对接收者内容的上边缘进行抗锯齿处理。

#### Identity Transform
&emsp;定义核心动画使用的身份转换矩阵。
```c++
const CATransform3D CATransform3DIdentity;
```
CATransform3DIdentity
&emsp;The identity transform: [1 0 0 0; 0 1 0 0; 0 0 1 0; 0 0 0 1].

#### Scaling Filters
&emsp;这些常量指定magnificationFilter和minificationFilter使用的缩放过滤器。
##### kCAFilterLinear
&emsp;线性插值滤波器。
```c++
const CALayerContentsFilter kCAFilterLinear;
```
##### kCAFilterNearest
&emsp;最近邻居插值滤波器。
```c++
const CALayerContentsFilter kCAFilterNearest;
```
##### kCAFilterTrilinear
&emsp;三线性缩小滤波器。启用mipmap生成。一些渲染器可能会忽略这一点，或施加其他限制，例如需要二维幂的源图像。
```c++
const CALayerContentsFilter kCAFilterTrilinear;
```
#### CATransform3D
&emsp;整个Core Animation中使用的标准转换矩阵。

&emsp;变换矩阵用于旋转，缩放，平移，倾斜和投影图层内容。提供了用于创建，连接和修改CATransform3D数据的功能。
```c++
struct CATransform3D
{
  CGFloat m11, m12, m13, m14;
  CGFloat m21, m22, m23, m24;
  CGFloat m31, m32, m33, m34;
  CGFloat m41, m42, m43, m44;
};
```

### Instance Properties

### Type Methods


## CAAction
&emsp;一个允许对象响应 CALayer 更改触发的 action 的接口。
```c++
@protocol CAAction
- (void)runActionForKey:(NSString *)event object:(id)anObject arguments:(nullable NSDictionary *)dict;
@end
```
&emsp;当使用动作标识符（键路径，外部动作名称或预定义的动作标识符）查询时，层将返回适当的动作对象（必须实现CAAction协议），并向其发送 `runActionForKey:object:arguments:` 消息。

/* Called to trigger the event named 'path' on the receiver. The object (e.g. the layer) on which the event happened is 'anObject'. The arguments dictionary may be nil, if non-nil it carries parameters associated with the event. */
调用以触发接收器上名为“ path”的事件。发生事件的对象（例如图层）是“ anObject”。参数字典可以为 nil，如果为非 nil，则其携带与事件关联的参数。
### runActionForKey:object:arguments:
&emsp;调用以触发标识符指定的操作。
```c++
- (void)runActionForKey:(NSString *)event object:(id)anObject arguments:(nullable NSDictionary *)dict;
```
&emsp;`key`: 动作的标识符。该标识符可以是相对于对象，任意外部动作或CALayer中定义的动作标识符之一的键或键路径。`anObject`: 应在其上发生操作的层。`dict`: 包含与此事件关联的参数的字典。可能为零。

## 参考链接
**参考链接:🔗**
+ [CALayer](https://developer.apple.com/documentation/quartzcore/calayer?language=objc)
+ [Core Animation Programming Guide](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/CoreAnimation_guide/Introduction/Introduction.html#//apple_ref/doc/uid/TP40004514-CH1-SW1)
