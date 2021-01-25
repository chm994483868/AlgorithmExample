# iOS《View Programming Guide for iOS》官方文档

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

&emsp;此方法的默认实现不执行任何操作。使用 Core Graphics 和 UIKit 等技术绘制视图内容的子类应重写此方法并在那里实现其绘制代码。如果视图以其他方式设置其内容，则不需要重写此方法。例如，如果视图仅显示背景色，或者视图直接使用底层对象设置其内容，则不需要重写此方法。

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
