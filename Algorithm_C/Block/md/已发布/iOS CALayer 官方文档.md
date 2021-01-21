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




## 参考链接
**参考链接:🔗**
+ [CALayer](https://developer.apple.com/documentation/quartzcore/calayer?language=objc)
+ [Core Animation Programming Guide](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/CoreAnimation_guide/Introduction/Introduction.html#//apple_ref/doc/uid/TP40004514-CH1-SW1)
