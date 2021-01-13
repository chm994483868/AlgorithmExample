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

&emsp;touch 对象在多点触摸序列（multi-touch sequence）中始终存在。



你可以在处理多点触摸序列时存储对触摸的引用，只要在序列结束时释放该引用即可。如果你需要在多点触摸序列之外存储有关触摸的信息，请从触摸中复制该信息。

&emsp;touch 的 gestureRecognizers 属性包含当前正在处理 touch 的 gesture recognizers。每个 gesture recognizers 都是 UIGestureRecognizer 的具体子类的实例。

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
+ [Responder object](https://developer.apple.com/library/archive/documentation/General/Conceptual/Devpedia-CocoaApp/Responder.html#//apple_ref/doc/uid/TP40009071-CH1-SW1)
+ [Events (iOS)](https://developer.apple.com/library/archive/documentation/General/Conceptual/Devpedia-CocoaApp/EventHandlingiPhone.html#//apple_ref/doc/uid/TP40009071-CH13-SW1)
+ [Target-Action](https://developer.apple.com/library/archive/documentation/General/Conceptual/CocoaEncyclopedia/Target-Action/Target-Action.html#//apple_ref/doc/uid/TP40010810-CH12)
+ [细数iOS触摸事件流动](https://juejin.cn/post/6844904175415853064)
+ [iOS 响应者链与事件处理](https://www.xiaobotalk.com/2020/03/responder-chain/)
+ [iOS开发系列--触摸事件、手势识别、摇晃事件、耳机线控](https://www.cnblogs.com/kenshincui/p/3950646.html)
