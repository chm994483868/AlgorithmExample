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













## 参考链接
**参考链接:🔗**
+ [Using Responders and the Responder Chain to Handle Events](https://developer.apple.com/documentation/uikit/touches_presses_and_gestures/using_responders_and_the_responder_chain_to_handle_events)
+ [Responder object](https://developer.apple.com/library/archive/documentation/General/Conceptual/Devpedia-CocoaApp/Responder.html#//apple_ref/doc/uid/TP40009071-CH1-SW1)
+ [Events (iOS)](https://developer.apple.com/library/archive/documentation/General/Conceptual/Devpedia-CocoaApp/EventHandlingiPhone.html#//apple_ref/doc/uid/TP40009071-CH13-SW1)
+ [Target-Action](https://developer.apple.com/library/archive/documentation/General/Conceptual/CocoaEncyclopedia/Target-Action/Target-Action.html#//apple_ref/doc/uid/TP40010810-CH12)
+ [细数iOS触摸事件流动](https://juejin.cn/post/6844904175415853064)
+ [iOS 响应者链与事件处理](https://www.xiaobotalk.com/2020/03/responder-chain/)
+ [iOS开发系列--触摸事件、手势识别、摇晃事件、耳机线控](https://www.cnblogs.com/kenshincui/p/3950646.html)
