# iOS 响应者链工作原理总结

> &emsp;

// 1. 沿着 UIApplication -> UIWindow -> UIView -> subView 寻找第一响应者
// 2. 第一响应者处理 UIEvent，如果第一响应者不能处理这个 UIEvent，则其顺着 Responder Chin 寻找能处理这个 UIEvent 的响应者（next Responder）
// 3. Target-Action 设计模式

### hitTest:withEvent:
&emsp;返回包含指定点（`point`）的视图层次结构中 receiver 的最远后代（最远子视图，也可能是其自身）。
```c++
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event;
```
&emsp;`point`: receiver 的本地坐标系（bounds）中指定的点。`event`: 需要调用此方法的事件。如果要从事件处理代码外部调用此方法，则可以指定 nil。

&emsp;Return Value: view 对象是当前 view 的最远子视图，并且包含 `point`。如果该 `point` 完全位于 receiver 的视图层次之外，则返回 nil。

&emsp;此方法通过调用每个子视图的 pointInside:withEvent: 方法来遍历视图层次结构，以确定哪个子视图应接收 touch 事件。如果 pointInside:withEvent: 返回 YES，然后类似地遍历其子视图的层次结构，直到找到包含 `point` 的最前面的视图。如果视图不包含该 `point`，则将忽略其视图层次结构的分支。你很少需要自己调用此方法，但可以重写它以从子视图中隐藏 touch 事件。

&emsp;此方法将忽略 hidden 设置为 YES 的、禁用用户交互（userInteractionEnabled 设置为 NO）或 alpha 小于 0.01 的视图对象。确定点击（determining a hit）时，此方法不会考虑视图的内容。因此，即使 `point` 位于该视图内容的透明部分中，该视图仍然可以返回。

&emsp;超出 receiver’s bounds 的 `point` 永远不会被报告为命中，即使它们实际上位于 receiver 的一个子视图中。如果当前视图的 clipsToBounds 属性设置为 NO，并且受影响的子视图超出了视图的边界，则会发生这种情况。（例如一个 button 按钮超出了其父试图的 bounds，此时点击 button 未超出父视图的区域的话可以响应点击事件，如果点击 button 超出父视图的区域的话则不能响应点击事件）

&emsp;hitTest:withEvent: 寻找一个包含 `point` 的视图的过程可以理解如下：
```c++
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event{
    // 3 种状态无法响应事件
    // 1): userInteractionEnabled 为 NO，禁止了用户交互。
    // 2): hidden 为 YES，被隐藏了。
    // 3): alpha 小于等于 0.01，透明度小于 0.01。
    if (self.userInteractionEnabled == NO || self.hidden == YES ||  self.alpha <= 0.01) return nil;
    
    // 触摸点若不在当前视图上则无法响应事件
    if ([self pointInside:point withEvent:event] == NO) return nil;
    
    // ⬇️⬇️⬇️ 从后往前遍历子视图数组
    int count = (int)self.subviews.count;
    for (int i = count - 1; i >= 0; i--) {
        // 获取子视图
        UIView *childView = self.subviews[i];
        
        // 坐标系的转换，把触摸点在当前视图上坐标转换为在子视图上的坐标
        CGPoint childP = [self convertPoint:point toView:childView];
        
        // 询问子视图层级中的最佳响应视图
        UIView *fitView = [childView hitTest:childP withEvent:event];
        
        if (fitView) {
            // 如果子视图中有更合适的就返回
            return fitView;
        }
    }
    
    // 没有在子视图中找到更合适的响应视图，那么自身就是最合适的
    return self;
}
```
### pointInside:withEvent:
&emsp;返回一个布尔值，该值指示 receiver 是否包含 `point`。
```c++
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event;
```
&emsp;`point`: receiver 的本地坐标系（bounds）中指定的点。`event`: 需要调用此方法的事件。如果要从事件处理代码外部调用此方法，则可以指定 nil。

&emsp;如果 `point` 包含在 receiver 的 bounds 中，则返回 YES，否则返回 NO。
### convertPoint:toView:
&emsp;将 `point` 从 receiver 的坐标系转换为指定视图（`view`）的点（CGPoint）。
```c++
- (CGPoint)convertPoint:(CGPoint)point toView:(UIView *)view;
```
&emsp;`point`: receiver 的本地坐标系（bounds）中指定的点。

## UITouch
&emsp;表示屏幕上发生的触摸的位置（location）、大小（size）、移动（movement）和力度（force）的对象。
```c++
UIKIT_EXTERN API_AVAILABLE(ios(2.0)) @interface UITouch : NSObject
```
&emsp;你可以通过传递给响应器对象（UIResponder 或其子类）的 UIEvent 对象访问 touch 对象，以进行事件处理。touch 对象包括用于以下对象的访问器：
+ 发生触摸的 view 或 window
+ 触摸在 view 或 window 中的位置
+ 触摸的近似半径（approximate radius）
+ 触摸的力度（force）（在支持 3D Touch 或 Apple Pencil 的设备上）

&emsp;touch 对象还包含一个时间戳，该时间戳指示何时发生触摸；一个整数，代表用户 tapped 屏幕的次数；以及触摸的阶段，其形式为常数，描述了触摸是开始，移动还是结束，或系统是否取消触摸。

&emsp;要了解如何使用 swipes，请阅读 Event Handling Guide for UIKit Apps 中的 Handling Swipe and Drag Gestures。

&emsp;touch 对象在多点触摸序列（multi-touch sequence）中始终存在。你可以在处理多点触控序列时存储对 touch 的引用，只要在序列结束时释放该引用即可。如果需要在多点触控序列之外存储有关 touch 的信息，请从 touch 中复制该信息。

&emsp;touch 的 gestureRecognizers 属性包含当前正在处理 touch 的 gesture recognizers。每个 gesture recognizer 都是 UIGestureRecognizer 的具体子类的实例。
### locationInView:
&emsp;返回给定 `view` 坐标系中 receiver 的当前位置。（即返回一个 UITouch 对象在 view 的坐标系中的位置（CGPoint））
```c++
- (CGPoint)locationInView:(nullable UIView *)view;
```
&emsp;`view`: 要在其坐标系中定位 touch 的视图对象。处理 touch 的自定义视图可以指定 self 以在其自己的坐标系中获取 touch 位置。传递 nil 以获取 window 坐标系中的 touch 位置。

&emsp;Return Value: 一个指定 receiver 在 view 中位置的 point。

&emsp;此方法返回 UITouch 对象在指定 view 的坐标系中的当前位置。因为 touch 对象可能已从另一个视图转发到一个视图，所以此方法将 touch 位置执行任何必要的转换到指定 view 的坐标系。
### previousLocationInView:
&emsp;返回 receiver 在给定 view 坐标系中的先前位置。
```c++
- (CGPoint)previousLocationInView:(nullable UIView *)view;
```
&emsp;`view`: 要在其坐标系中定位 touch 的视图对象。处理 touch 的自定义视图可以指定 self 以在其自己的坐标系中获取 touch 位置。传递 nil 以获取 window 坐标系中的 touch 位置。

&emsp;Return Value: 此方法返回 UITouch 对象在指定 view 的坐标系中的上一个位置。因为 touch 对象可能已从另一个视图转发到一个视图，所以此方法将 touch 位置执行任何必要的转换到指定 view 的坐标系。
### view
&emsp;触摸要传递到的视图（如果有的话）。
```c++
@property(nullable,nonatomic,readonly,strong) UIView *view;
```
&emsp;此属性的值是将 touches 传递到的 view 对象，不一定是 touch 当前所在的 view。例如，当 gesture recognizer 识别到 touch 时，此属性为 nil，因为没有 view 在接收 touch。
### window
&emsp;最初发生 touch 的 window。
```c++
@property(nullable,nonatomic,readonly,strong) UIWindow *window;
```
&emsp;该属性的值是最初发生 touch 的 window。该 window 可能与当前包含 touch 的 window 不同。
### majorRadius
&emsp;touch 的半径（以点（points）表示）。
```c++
@property(nonatomic,readonly) CGFloat majorRadius API_AVAILABLE(ios(8.0));
```
&emsp;使用此属性中的值确定硬件报告的 touch 大小。此值是大小的近似值，可以根据 majorRadiusTolerance 属性中指定的量而变化。
### majorRadiusTolerance
&emsp;touch 的半径的容差（以点表示）。
```c++
@property(nonatomic,readonly) CGFloat majorRadiusTolerance API_AVAILABLE(ios(8.0));
```
&emsp;此值确定 majorRadius 属性中值的准确性。将此值添加到半径以获得最大触摸半径。减去该值以获得最小触摸半径。
### preciseLocationInView:
&emsp;返回 touch 的精确位置（如果可用）。
```c++
- (CGPoint)preciseLocationInView:(nullable UIView *)view API_AVAILABLE(ios(9.1));
```
&emsp;`view`: 包含 touch 的视图。

&emsp;Return Value: touch 的精确位置。

&emsp;使用此方法可获取 touch 的额外精度（如果可用）。不要使用返回点进行命中测试（hit testing）。在某些情况下，命中测试可能表示 touch 位于视图中，但针对更精确位置的命中测试可能表示 touch 在视图之外。
### precisePreviousLocationInView:
&emsp;返回 touch 的精确先前位置（如果可用）。
```c++
- (CGPoint)precisePreviousLocationInView:(nullable UIView *)view API_AVAILABLE(ios(9.1));
```
&emsp;使用此方法可以获得 touch 先前位置的额外精度（如果可用）。不要使用返回点进行命中测试。在某些情况下，命中测试可能表示 touch 位于视图中，但针对更精确位置的命中测试可能表示 touch 在视图之外。
### tapCount
&emsp;给定 touch 的 tap 次数。
```c++
@property(nonatomic,readonly) NSUInteger tapCount; // 在一定时间内在某个点内触摸
```
&emsp;此属性的值是一个整数，包含在预定义的时间段内此 touch 发生的点击数。使用此属性可评估用户是单点、双点、还是甚至三击特定 view 或 window。
### timestamp
&emsp;touch 发生的时间或上次发生 mutated 的时间。
```c++
@property(nonatomic,readonly) NSTimeInterval timestamp;
```
&emsp;此属性的值是自系统启动以来触发 touch 或上次更改 touch 的时间（以秒为单位）。你可以存储此属性的值，并将其与后续 UITouch 对象中的时间戳进行比较，以确定触摸的持续时间，如果 touch 正在轻扫，则确定移动速度。有关系统启动后的时间定义，请参阅 NSProcessInfo 类的 systemUptime 方法的说明。
### UITouchType
&emsp;touch 类型。
```c++
typedef NS_ENUM(NSInteger, UITouchType) {
    UITouchTypeDirect, // A direct touch from a finger (on a screen) 手指直接触摸（在屏幕上）
    UITouchTypeIndirect, // An indirect touch (not a screen) 间接触摸（不是屏幕）
    UITouchTypePencil API_AVAILABLE(ios(9.1)), // Add pencil name variant 添加 pencil 名称变体 
    UITouchTypeStylus API_AVAILABLE(ios(9.1)) = UITouchTypePencil, // A touch from a stylus (deprecated name, use pencil) 手写笔的触摸（已弃用，UITouchTypePencil）
    
    // A touch representing a button-based, indirect input device describing the input sequence from button press to button release
    // 表示基于按钮的间接输入设备的触摸，描述从按钮按下到按钮释放的输入序列
    UITouchTypeIndirectPointer API_AVAILABLE(ios(13.4), tvos(13.4)) API_UNAVAILABLE(watchos),
    
} API_AVAILABLE(ios(9.0));
```
+ UITouchTypeDirect: 与屏幕直接接触产生的触摸。当用户的手指接触屏幕时，会发生直接接触。
+ UITouchTypeIndirect: 不是接触屏幕造成的触摸。间接触摸是由与屏幕分离的触摸输入设备产生的。例如，Apple TV 遥控器的触控板会产生间接触摸。
+ UITouchTypePencil: Apple Pencil 的 touch。当 Apple Pencil 与设备的屏幕交互时，会发生 Pencil Touch。
+ UITouchTypeStylus: 已废弃，使用 UITouchTypePencil 代替。
### type
&emsp;表示 touch 类型的属性。
```c++
@property(nonatomic,readonly) UITouchType type API_AVAILABLE(ios(9.0));
```
&emsp;有关触摸类型的完整列表，请参阅 maximumPossibleForce。
### UITouchPhase
&emsp;touch 事件的阶段。
```c++
typedef NS_ENUM(NSInteger, UITouchPhase) {
    UITouchPhaseBegan, // whenever a finger touches the surface. 只要手指碰到表面。
    UITouchPhaseMoved, // whenever a finger moves on the surface. 当手指在表面上移动时。
    UITouchPhaseStationary, // whenever a finger is touching the surface but hasn't moved since the previous event. 当手指接触到表面，但自上次事件后没有移动时。
    UITouchPhaseEnded, // whenever a finger leaves the surface. 当手指离开表面。
    UITouchPhaseCancelled, // whenever a touch doesn't end but we need to stop tracking (e.g. putting device to face) 当触摸未结束但我们需要停止跟踪时（例如，接听电话时将设备放在脸上移动）
    
    UITouchPhaseRegionEntered   API_AVAILABLE(ios(13.4), tvos(13.4)) API_UNAVAILABLE(watchos),  // whenever a touch is entering the region of a user interface 每当触摸进入用户界面区域时
    
    // when a touch is inside the region of a user interface, but hasn’t yet made contact or left the region
    // 当触摸位于用户界面区域内，但尚未联系或离开该区域时
    UITouchPhaseRegionMoved     API_AVAILABLE(ios(13.4), tvos(13.4)) API_UNAVAILABLE(watchos),
    
    UITouchPhaseRegionExited    API_AVAILABLE(ios(13.4), tvos(13.4)) API_UNAVAILABLE(watchos),  // when a touch is exiting the region of a user interface 当触摸退出用户界面区域时
};
```
&emsp;UITouch 实例的阶段随着系统在事件过程中接收更新而改变。通过 phase 属性访问此值。
+ UITouchPhaseBegan: 屏幕上按下了对给定事件的 touch。
+ UITouchPhaseMoved: 给定事件的 touch 已在屏幕上移动。
+ UITouchPhaseStationary: 在屏幕上按下给定事件的 touch，但自上一个事件后就再也没有移动过。
+ UITouchPhaseEnded: 给定事件的 touch 已从屏幕上抬起。
+ UITouchPhaseCancelled: 例如，当用户将设备靠在脸上移动时，系统取消了对触摸的跟踪。
+ UITouchPhaseRegionEntered: 给定事件的触摸已进入屏幕上的 window。UITouchPhaseRegionEntered、UITouchPhaseRegionMoved 和 UITouchPhaseRegionSited 阶段并不总是与 UIHoverGestureRecognizer 的状态属性对齐。hover gesture recognizer 的状态仅适用于 gesture’s 视图的上下文，而 touch states 适用于 window。
+ UITouchPhaseRegionMoved: 给定事件的触摸在屏幕上的窗口内，但尚未按下。
+ UITouchPhaseRegionExited: 对给定事件的触摸在屏幕上留下了一个窗口。

### phase
&emsp;touch 阶段。属性值是一个常量，指示触摸是开始、移动、结束还是取消。有关此属性可能值的描述，请参见 UITouchPhase。
```c++
@property(nonatomic,readonly) UITouchPhase phase;
```
### gestureRecognizers
&emsp;接收 touch 对象的 gesture recognizers。
```c++
@property(nullable,nonatomic,readonly,copy)   NSArray <UIGestureRecognizer *> *gestureRecognizers API_AVAILABLE(ios(3.2));
```
&emsp;数组中的对象是抽象基类 UIGestureRecognizer 的子类的实例。如果当前没有接收 touch 的 gesture recognizers，则此属性包含空数组。

&emsp;下面大概是一些 3D Touch、Apple Pencil 相关的（触摸）内容，可直接忽略。
### force
&emsp;touch 的力，其中值 1.0 表示平均触摸力（由系统预先确定，而不是特定于用户）。
```c++
@property(nonatomic,readonly) CGFloat force API_AVAILABLE(ios(9.0));
```
&emsp;此属性在支持 3D Touch 或 Apple Pencil 的设备上可用。要在运行时检查设备是否支持 3D Touch，请读取应用程序中具有 trait 环境的任何对象的 trait 集合上 forceTouchCapability 属性的值。

&emsp;Apple Pencil 所报告的力是沿着 Pencil 的轴线测量的。如果想要垂直于设备的力，需要使用 altitudeAngle 值计算该值。

&emsp;Apple Pencil 报告的力最初是估计的，可能并不总是更新。要确定是否需要更新，请参阅 estimatedPropertiesExpectingUpdates 并查找 UITouchPropertyForce 标志。在这种情况下，estimationUpdateIndex 索引包含一个非 nil 值，你可以在更新发生时将该值与原始触摸相关联。当没有预期的力更新时，整个触摸序列通常不会有更新，因此可以对触摸序列应用自定义的、特定于工具的力曲线。
### maximumPossibleForce
&emsp;touch 的最大可能力。
```c++
@property(nonatomic,readonly) CGFloat maximumPossibleForce API_AVAILABLE(ios(9.0));
```
&emsp;该属性的值足够高，可以为 force 属性的值提供广泛的动态范围。

&emsp;此属性在支持 3D Touch 或 Apple Pencil 的设备上可用。要在运行时检查设备是否支持 3D Touch，请读取应用程序中具有 trait 环境的任何对象的 trait 集合上 forceTouchCapability 属性的值。
### altitudeAngle
&emsp;Pencil 的高度（弧度）。仅适用于 UITouchTypePencil 类型。

```c++
@property(nonatomic,readonly) CGFloat altitudeAngle API_AVAILABLE(ios(9.1));
```
&emsp;值为 0 弧度表示 Apple Pencil 与曲面平行。当 Apple Pencil 垂直于曲面时，此属性的值为 Pi/2。

&emsp;下面好像暂时用不到的两个方法。
### azimuthAngleInView:
&emsp;返回 Apple Pencil 的方位角（以弧度为单位）。
```c++
- (CGFloat)azimuthAngleInView:(nullable UIView *)view API_AVAILABLE(ios(9.1));
```
&emsp;在屏幕平面中，方位角是指手写笔指向的方向。当触针尖端接触屏幕时，当触针的帽端（即尖端对面的端部）指向设备屏幕的正x轴时，此属性的值为0弧度。当用户围绕笔尖顺时针方向摆动手写笔的笔帽端时，方位角会增加。
> &emsp;Note: 获取方位角（相对于方位单位矢量）的成本更高，但也更方便。
### azimuthUnitVectorInView:
&emsp;返回指向 Apple Pencil 方位角方向的单位向量。
```c++
- (CGVector)azimuthUnitVectorInView:(nullable UIView *)view API_AVAILABLE(ios(9.1));
```
&emsp;得到方位单位矢量比得到方位角要便宜。如果要创建变换矩阵，单位向量也会更有用。
### UITouchProperties
&emsp;一些可能会更新的 touch 属性的位掩码。
```c++
typedef NS_OPTIONS(NSInteger, UITouchProperties) {
    UITouchPropertyForce = (1UL << 0),
    UITouchPropertyAzimuth = (1UL << 1),
    UITouchPropertyAltitude = (1UL << 2),
    UITouchPropertyLocation = (1UL << 3), // For predicted Touches 对于预测的触摸
} API_AVAILABLE(ios(9.1));
```
+ UITouchPropertyForce: 位掩码中表示 force 的 touch 属性。
+ UITouchPropertyAzimuth: 位掩码中表示 azimuth（方位角） 的 touch 属性。（用于 Apple Pencil）
+ UITouchPropertyAltitude: 位掩码中表示 altitude（高度/海拔） 的 touch 属性。（用于 Apple Pencil）
+ UITouchPropertyLocation: 位掩码中表示 location 的 touch 属性。

### estimatedProperties
&emsp;一组 touch 属性，其值仅包含估计值。
```c++
@property(nonatomic,readonly) UITouchProperties estimatedProperties API_AVAILABLE(ios(9.1));
```
&emsp;此属性包含一个常量位掩码，表示无法立即报告哪些触摸属性。例如，Apple Pencil 记录了触摸的力度，但必须通过空中传输信息到底层 iPad。传输数据所产生的延迟可能会导致在向应用程序报告触摸后接收信息。

&emsp;不保证以后更新此属性中的值。有关希望更新其值的属性列表，请参阅 estimatedPropertiesExpectingUpdates。
### estimatedPropertiesExpectingUpdates
&emsp;一组 touch 属性，预计将来会更新这些属性的值。
```c++
@property(nonatomic,readonly) UITouchProperties estimatedPropertiesExpectingUpdates API_AVAILABLE(ios(9.1));
```
&emsp;此属性包含常量的位掩码，该位掩码指示哪些触摸属性无法立即报告，哪些触摸属性需要稍后更新。当此属性包含非空集时，可以期望 UIKit 稍后使用给定属性的更新值调用响应程序或手势识别器的 toucheEstimatedPropertiesUpdated: 方法。将 estimationUpdateIndex 属性中的值附加到应用程序的触摸数据副本。当 UIKit 稍后调用 toucheEstimatedPropertiesUpdated: 方法时，使用新 touch 的估计更新索引来定位和更新应用程序的 touch 数据副本。

&emsp;当此属性包含空集时，不需要更多更新。在该场景中，估计值或更新值是最终值。
### estimationUpdateIndex
&emsp;一个索引编号，用于将更新的 touch 与原始 touch 关联起来。
```c++
@property(nonatomic,readonly) NSNumber * _Nullable estimationUpdateIndex API_AVAILABLE(ios(9.1));
```
&emsp;此属性包含当前触摸数据的唯一标记。当触摸包含估计属性时，将此索引与其余触摸数据一起保存到应用程序的数据结构中。当系统稍后报告实际触摸值时，使用此索引定位应用程序数据结构中的原始数据，并替换先前存储的估计值。例如，当触摸包含估计属性时，可以将此属性用作字典中的键，字典的值是用于存储触摸数据的对象。

&emsp;对于包含估计属性的每个触摸，此属性的值都会单调增加。当触摸对象不包含估计或更新的属性时，此属性的值为零。
## Handling Touches in Your View（处理视图中的触摸）
&emsp;如果触摸处理（touch handling）与 view 的内容有复杂的链接（intricately linked），则直接在 view 子类上使用触摸事件（touch events）。

&emsp;如果你不打算对自定义视图使用手势识别器，则可以直接从视图本身处理触摸事件。因为视图是响应者，所以它们可以处理多点触控事件和许多其他类型的事件。当 UIKit 确定某个视图中发生了触摸事件时，它将调用该视图的 `touchesBegan:withEvent:`、`touchesMoved:withEvent:` 或 `touchesEnded:withEvent:` 方法。你可以在自定义视图中重写这些方法，并使用它们提供对触摸事件的响应。（来自 UIResponder）

&emsp;你在视图（或任何响应程序）中重写的处理触摸的方法对应于触摸事件处理过程的不同阶段。例如，图1 说明了触摸事件的不同阶段。当手指（或 Apple Pencil）接触屏幕时，UIKit 会创建 UITouch 对象，将触摸位置设置为适当的点，并将其 phase 属性设置为 UITouchPhaseBegan。当同一个手指在屏幕上移动时，UIKit 会更新触摸位置，并将触摸对象的 phase 属性更改为 UITouchPhaseMoved。当用户将手指从屏幕上提起时，UIKit 将 phase 属性更改为 UITouchPhaseEnded，触摸序列结束。

&emsp;Figure 1 The phases of a touch event（触摸事件的各个阶段）
![]()

&emsp;类似地，系统可以随时取消正在进行的触摸序列；例如，当来电中断应用程序时。当它这样做时，UIKit 通过调用 touchs 来通知你的视图 touchesCancelled:withEvent: 方法。你可以使用该方法对视图的数据结构执行任何必要的清理。

&emsp;UIKit 为触摸屏幕的每个新手指创建一个新的 UITouch 对象。触摸本身是通过当前 UIEvent 对象传递的。UIKit 区分了来自手指和 Apple Pencil 的触摸，你可以对它们进行不同的处理。

> &emsp;Important: 在其默认配置中，视图仅接收与事件关联的第一个 UITouch 对象，即使有多个手指接触视图也是如此。要接收额外的触摸，必须将视图的 multipleTouchEnabled 属性设置为 true。也可以使用属性检查器在 Interface Builder 中配置此属性。

## UIResponder
&emsp;

xxxxxx

## UIEvent
&emsp;描述单个 user 与你的应用交互的对象。
```c++
UIKIT_EXTERN API_AVAILABLE(ios(2.0)) @interface UIEvent : NSObject
```
&emsp;应用程序可以接收许多不同类型的事件，包括触摸事件（touch events）、运动事件（motion events）、远程控制事件（remote-control events）和按键事件（press events）。
+ 触摸事件是最常见的，并且被传递到最初发生触摸的视图中。
+ 运动事件是 UIKit 触发的，与 Core Motion 框架报告的运动事件是分开的。
+ 远程控制事件允许响应者对象从外部附件或耳机接收命令，以便它可以管理音频和视频的管理，例如，播放视频或跳至下一个音轨。
+ 按键事件表示与游戏控制器、AppleTV 遥控器或其他具有物理按钮的设备的交互。
&emsp;可以使用类型（`type`）和子类型（`subtype`）属性确定事件的类型。

&emsp;触摸事件对象包含与事件有某种关系的触摸（即屏幕上的手指）。触摸事件对象可以包含一个或多个触摸，并且每个触摸都由 UITouch 对象表示。
当触摸事件发生时，系统将其路由到相应的响应程序并调用相应的方法，如 touchesBegan:withEvent:。然后，响应者使用 touches 来确定适当的行动方案。

&emsp;在多点触摸序列中，UIKit 在将更新的触摸数据传递到你的应用程序时会重用同一 UIEvent 对象。你永远不应 retain 事件对象或事件对象返回的任何对象。如果需要在用于处理该数据的响应程序方法之外保留数据，请将该数据从 UITouch 或 UIEvent 对象复制到本地数据结构。

&emsp;有关如何在 UIKit 应用中处理事件的更多信息，请参见 Event Handling Guide for UIKit Apps。（UIKit 文档内容过多，这里我们只阅读 Handling Touches in Your View 文档）
### UIEventType
&emsp;指定事件的常规类型。
```c++
typedef NS_ENUM(NSInteger, UIEventType) {
    UIEventTypeTouches,
    UIEventTypeMotion,
    UIEventTypeRemoteControl,
    UIEventTypePresses API_AVAILABLE(ios(9.0)),
};
```
&emsp;你可以从 type 属性获取事件的类型。为了进一步识别事件，你可能还需要确定其子类型，该子类型是从 subtype 属性获得的。
+ UIEventTypeTouches: 该事件与屏幕上的触摸有关。
+ UIEventTypeMotion: 该事件与设备的运动有关，例如用户摇晃设备。
+ UIEventTypeRemoteControl: 该事件是一个远程控制事件。远程控制事件源于从耳机或外部附件接收的命令，用于控制设备上的多媒体。
+ UIEventTypePresses: 该事件与按下物理按钮有关。

### UIEventSubtype
&emsp;指定事件的子类型（相对于其常规类型）。
```c++
typedef NS_ENUM(NSInteger, UIEventSubtype) {
    // available in iPhone OS 3.0
    UIEventSubtypeNone                              = 0,
    
    // for UIEventTypeMotion, available in iPhone OS 3.0
    UIEventSubtypeMotionShake                       = 1,
    
    // for UIEventTypeRemoteControl, available in iOS 4.0
    UIEventSubtypeRemoteControlPlay                 = 100,
    UIEventSubtypeRemoteControlPause                = 101,
    UIEventSubtypeRemoteControlStop                 = 102,
    UIEventSubtypeRemoteControlTogglePlayPause      = 103,
    UIEventSubtypeRemoteControlNextTrack            = 104,
    UIEventSubtypeRemoteControlPreviousTrack        = 105,
    UIEventSubtypeRemoteControlBeginSeekingBackward = 106,
    UIEventSubtypeRemoteControlEndSeekingBackward   = 107,
    UIEventSubtypeRemoteControlBeginSeekingForward  = 108,
    UIEventSubtypeRemoteControlEndSeekingForward    = 109,
};
```
&emsp;你可以从 subtype 属性获取事件的子类型。
+ UIEventSubtypeNone: 该事件没有子类型。这是 UIEventTypeTouches 常规类型的事件的子类型。
+ UIEventSubtypeMotionShake: 该事件与用户摇晃设备有关。它是 UIEventTypeMotion 常规事件类型的子类型。
+ UIEventSubtypeRemoteControlPlay: 播放音频或视频的远程控制事件。它是 UIEventTypeRemoteControl 常规事件类型的子类型。
+ UIEventSubtypeRemoteControlPause: 暂停音频或视频的远程控制事件。它是 UIEventTypeRemoteControl 常规事件类型的子类型。
+ UIEventSubtypeRemoteControlStop: 用于停止播放音频或视频的远程控制事件。它是 UIEventTypeRemoteControl 常规事件类型的子类型。
+ UIEventSubtypeRemoteControlTogglePlayPause: 在播放和暂停之间切换音频或视频的远程控制事件。它是 UIEventTypeRemoteControl 常规事件类型的子类型。
+ UIEventSubtypeRemoteControlNextTrack: 跳至下一个音频或视频轨道的远程控制事件。它是 UIEventTypeRemoteControl 常规事件类型的子类型。
+ UIEventSubtypeRemoteControlPreviousTrack: 跳到上一个音频或视频轨道的远程控制事件。它是 UIEventTypeRemoteControl 常规事件类型的子类型。
+ UIEventSubtypeRemoteControlBeginSeekingBackward: 一个远程控制事件，开始通过音频或视频媒体向后搜索。它是 UIEventTypeRemoteControl 常规事件类型的子类型。
+ UIEventSubtypeRemoteControlEndSeekingBackward: 结束通过音频或视频媒体向后搜索的远程控制事件。它是 UIEventTypeRemoteControl 常规事件类型的子类型。 
+ UIEventSubtypeRemoteControlBeginSeekingForward: 一个开始通过音频或视频介质向前搜索的远程控制事件。它是 UIEventTypeRemoteControl 常规事件类型的子类型。
+ UIEventSubtypeRemoteControlEndSeekingForward: 结束通过音频或视频介质向前搜索的远程控制事件。它是 UIEventTypeRemoteControl 常规事件类型的子类型。

### type
&emsp;返回事件的类型。
```c++
@property(nonatomic,readonly) UIEventType     type API_AVAILABLE(ios(3.0));
```
&emsp;此属性返回的 UIEventType 常量指示此事件的常规类型，例如，它是触摸事件还是运动事件。
### subtype
&emsp;返回事件的子类型。
```c++
@property(nonatomic,readonly) UIEventSubtype  subtype API_AVAILABLE(ios(3.0));
```
&emsp;此属性返回的 UIEventSubtype 常量指示与常规类型相关的事件的子类型，该子类型是从 type 属性返回的。
### timestamp
&emsp;事件发生的时间。
```c++
@property(nonatomic,readonly) NSTimeInterval  timestamp;
```
&emsp;此属性包含自系统启动以来经过的秒数。有关此时间值的描述，请参见 NSProcessInfo 类的 systemUptime 方法的描述。
### allTouches
&emsp;返回与事件关联的所有 touches。
```c++
@property(nonatomic, readonly, nullable) NSSet <UITouch *> *allTouches; // 只读集合
```
&emsp;一组 UITouch 对象，表示与事件关联的所有 touches。

&emsp;如果事件的 touches 源自不同的 views 和 windows，则从此方法获得的 UITouch 对象将与不同的响应者对象相关联。
### touchesForWindow:
&emsp;从事件中返回属于指定 window 的触摸对象。
```c++
- (nullable NSSet <UITouch *> *)touchesForWindow:(UIWindow *)window;
```
&emsp;`window`: 最初发生 touches 的 UIWindow 对象。

&emsp;一组 UITouch 对象，它们代表属于指定 window 的触摸。
### touchesForView:
&emsp;从事件中返回属于指定 view 的触摸对象。
```c++
- (nullable NSSet <UITouch *> *)touchesForView:(UIView *)view;
```
&emsp;`view`: 最初发生 touches 的 UIView 对象。

&emsp;一组 UITouch 对象，它们代表属于指定 view 的触摸。
### touchesForGestureRecognizer:
&emsp;返回要传递到指定 gesture recognizer 的触摸对象。
```c++
- (nullable NSSet <UITouch *> *)touchesForGestureRecognizer:(UIGestureRecognizer *)gesture API_AVAILABLE(ios(3.2));
```
&emsp;`gesture`: 抽象基类 UIGestureRecognizer 的子类的实例。必须将此 gesture-recognizer object  附加到视图，以接收对该视图及其子视图进行了经过 hit-tested 触摸。

&emsp;一组表示触摸的 UITouch 对象，这些对象将传递给 receiver 表示的事件的指定 gesture recognizer。
### coalescedTouchesForTouch:
&emsp;返回与指定 main touch 关联的所有 touches。（一组针对给定 main touch 未传递的触摸事件的辅助 UITouch。这还包括 main touch 本身的辅助版本。）
```c++
- (nullable NSArray <UITouch *> *)coalescedTouchesForTouch:(UITouch *)touch API_AVAILABLE(ios(9.0));
```
&emsp;`touch`: 与 event 一起报告的 main touch 对象。你指定的触摸对象确定返回附加触摸的序列。

&emsp;Return Value: UITouch 对象的数组，代表自上次传递事件以来针对指定触摸报告的所有触摸。数组中对象的顺序与将触摸报告给系统的顺序匹配，最后一次触摸是你在 touch 参数中指定的同一触摸的副本。如果 touch 参数中的对象与当前事件不关联，则返回值为 nil。

&emsp;使用此方法可获取系统接收到但未在上一个 UIEvent 对象中传递的任何其他接触。一些设备以高达 240 赫兹的频率收集触摸数据，这通常高于将触摸传送到应用程序的速率。尽管这些额外的触摸数据提供了更高的精度，但许多应用程序并不需要这种精度，也不想招致与处理它们相关的开销。但是，想要提高精度的应用程序可以使用此方法来检索额外的触摸对象。例如，绘图应用程序可以使用这些触摸来获取用户绘图输入的更精确记录。然后，你可以将额外的触摸数据应用于应用程序的内容。如果你想要合并的触摸或使用传递给响应者方法的一组触摸，请使用此方法，但不要将两组触摸混合在一起。此方法返回自上次事件以来报告的触摸的完整序列，包括报告给响应者方法的触摸的副本。传递给响应程序方法的事件只包含序列中的最后一次触摸。类似地，alltouchs 属性只包含序列中的最后一次触摸。（想到了 Apple pencil 在 iPad 上使用时，iPad 屏幕刷新会提高到 120赫兹）
### predictedTouchesForTouch:
&emsp;返回预计将针对指定触摸发生的触摸数组。（一组辅助 UITouch，用于预测在给定的 main touch 下会发生的触摸事件。这些预测可能与触摸的真实行为不完全匹配，因此应将其解释为一种估计。）
```c++
- (nullable NSArray <UITouch *> *)predictedTouchesForTouch:(UITouch *)touch API_AVAILABLE(ios(9.0));
```
&emsp;`touch`: 与事件一起报告的 main touch 对象。你指定的触摸对象用于确定返回附加触摸的序列。

&emsp;Return Value: 一组 UITouch 对象，它们表示系统将预测的下一组触摸。数组中对象的顺序与预期将触摸传递到你的应用程序的顺序匹配。该数组不包括你在 touch 参数中指定的原始触摸。如果 touch 参数中的对象与当前事件没有关联，则返回值为 nil。

&emsp;使用此方法可以最小化用户的触摸输入和屏幕内容呈现之间的明显延迟。处理来自用户的触摸输入并将该信息转换为绘图命令需要时间，而将这些绘图命令转换为呈现的内容则需要额外的时间。如果用户的手指或 Apple Pencil 移动速度足够快，这些延迟可能会导致当前触摸位置和渲染内容之间出现明显的间隙。为了最大限度地减少感觉到的延迟，请使用此方法的预期效果作为对内容的附加临时输入。

&emsp;此方法返回的触摸表示系统根据用户过去的输入估计用户的触摸输入将在何处。仅将这些触摸暂时附加到用于绘制或更新内容的结构中，并在收到带有新触摸的新事件后立即丢弃它们。当与合并的触摸和高效的绘图代码结合使用时，你可以创建一种感觉，即用户的输入被立即处理，几乎没有延迟。这种感觉改善了用户对绘图应用程序或任何让用户直接在屏幕上操作对象的应用程序的体验。










## 参考链接
**参考链接:🔗**
+ [Using Responders and the Responder Chain to Handle Events](https://developer.apple.com/documentation/uikit/touches_presses_and_gestures/using_responders_and_the_responder_chain_to_handle_events)
+ [Handling Touches in Your View](https://developer.apple.com/documentation/uikit/touches_presses_and_gestures/handling_touches_in_your_view)
+ [Responder object](https://developer.apple.com/library/archive/documentation/General/Conceptual/Devpedia-CocoaApp/Responder.html#//apple_ref/doc/uid/TP40009071-CH1-SW1)
+ [Events (iOS)](https://developer.apple.com/library/archive/documentation/General/Conceptual/Devpedia-CocoaApp/EventHandlingiPhone.html#//apple_ref/doc/uid/TP40009071-CH13-SW1)
+ [Target-Action](https://developer.apple.com/library/archive/documentation/General/Conceptual/CocoaEncyclopedia/Target-Action/Target-Action.html#//apple_ref/doc/uid/TP40010810-CH12)
+ [细数iOS触摸事件流动](https://juejin.cn/post/6844904175415853064)
+ [iOS 响应者链与事件处理](https://www.xiaobotalk.com/2020/03/responder-chain/)
+ [iOS开发系列--触摸事件、手势识别、摇晃事件、耳机线控](https://www.cnblogs.com/kenshincui/p/3950646.html)
