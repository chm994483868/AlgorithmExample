# iOS 从源码解析Run Loop (五)：NSPort mach_msg 相关内容解析

> &emsp;Port 相关的内容不知道如何入手学习，那么就从 NSPort 开始吧。

## NSPort 
&emsp;`NSPort` 表示通信通道（communication channel）的抽象类。
```c++
@interface NSPort : NSObject <NSCopying, NSCoding>
```
&emsp;通信发生在 NSPort 对象之间，这些对象通常位于不同的线程或任务中。分布式对象系统（distributed objects system）使用 NSPort 对象来回发送 NSPortMessage 对象。尽可能使用分布式对象（distributed objects）实现应用程序间通信（interapplication communication），并且仅在必要时使用 NSPort 对象。

&emsp;要接收传入的消息，必须将 NSPort 对象作为 input sources 添加到 NSRunLoop 对象中。 NSConnection 对象在初始化时会自动添加其接收端口（receive port）。

&emsp;当 NSPort 对象接收到端口消息时，它将消息通过 `handleMachMessage:` 或 `handlePortMessage:` 消息转发给其 delegate。delegate 应仅实现这些方法中的一种，以所需的任何形式处理传入的消息。`handleMachMessage:` 提供以 msg_header_t 结构开头的 "原始 Mach 消息" 的消息。`handlePortMessage:` 将消息作为 NSPortMessage 对象提供，它是 Mach 消息的面向对象封装。如果尚未设置委托，NSPort 对象将处理消息本身。

&emsp;使用完端口对象后，必须先显式地使端口对象无效，然后再向其发送释放消息。类似地，如果应用程序使用垃圾回收，则必须在删除对端口对象的任何强引用之前使其无效。如果不使端口无效，则生成的端口对象可能会延迟并导致内存泄漏。要使端口对象无效，请调用其 `invalidate` 方法。

&emsp;Foundation 定义了 NSPort 的三个具体子类。NSMachPort 和 NSMessagePort 只允许本地（在同一台机器上）通信。NSSocketPort 允许本地和远程通信，但对于本地情况，可能比其他端口更昂贵。使用 `allocWithZone:` 或 `port` 创建 `NSPort` 对象时，将改为创建 NSMachPort 对象。

&emsp;NSPort 符合 NSCoding 协议，但只支持 NSPortCoder 进行编码。NSPort 及其子类不支持 archiving。

### allocWithZone:
&emsp;返回 NSMachPort 类的实例。
```c++
+ (id)allocWithZone:(NSZone *)zone
```
&emsp;`zone`：要在其中分配新对象的内存区域。

&emsp;为了 Mach 上的向后兼容性，`allocWithZone:` 在发送到 NSPort 类时返回 NSMachPort 类的实例。否则，它将返回一个具体子类的实例，该实例可用于本地计算机上的线程或进程之间的消息传递，或者在 NSSocketPort 的情况下，在不同计算机上的进程之间进行消息传递。
### port
&emsp;创建并返回一个可以发送和接收消息的新 NSPort 对象。
```c++
+ (NSPort *)port;
```
### invalidate
&emsp;将 receiver 标记为无效，并向默认通知中心发布 NSPortDidBecomeInvalidNotification。（即发送一个 NSPortDidBecomeInvalidNotification 通知）
```c++
- (void)invalidate;
```
&emsp;必须在释放端口对象之前调用此方法（如果应用程序被垃圾回收，则删除对该对象的强引用）。
### valid
&emsp;指示 receiver 是否有效的布尔值。
```c++
@property (readonly, getter=isValid) BOOL valid;
```
&emsp;如果已知 receiver 无效，则为 NO，否则为 YES。NSPort 对象在其依赖于操作系统的底层通信资源关闭或损坏时变为无效。
### setDelegate:
&emsp;将 receiver’s delegate 设置为指定对象。
```c++
// NSPortDelegate 协议仅有一个可选的 - (void)handlePortMessage:(NSPortMessage *)message; 方法
- (void)setDelegate:(nullable id <NSPortDelegate>)anObject;
```
### delegate
&emsp;返回 receiver’s delegate，可能为 NULL。
```c++
- (nullable id <NSPortDelegate>)delegate;
```
### scheduleInRunLoop:forMode:
&emsp;这个方法应该由一个子类来实现，当在给定的 input mode（NSRunLoopMode）下添加到给定的 run loop 中时，它可以设置对端口的监视。
```c++
- (void)scheduleInRunLoop:(NSRunLoop *)runLoop forMode:(NSRunLoopMode)mode;
```
&emsp;不应直接调用此方法。
### removeFromRunLoop:forMode:
&emsp;这个方法应该由一个子类来实现，当在给定的 input mode（NSRunLoopMode）下从给定的 run loop 中删除时，停止对端口的监视。
```c++
- (void)removeFromRunLoop:(NSRunLoop *)runLoop forMode:(NSRunLoopMode)mode;
```
&emsp;不应直接调用此方法。
### reservedSpaceLength
&emsp;receiver 为发送数据而保留的空间字节数。默认长度为 0。
```c++
@property (readonly) NSUInteger reservedSpaceLength;
```
### sendBeforeDate:components:from:reserved:
&emsp;此方法是为具有自定义 NSPort 类型的子类提供的。（NSConnection）
```c++
- (BOOL)sendBeforeDate:(NSDate *)limitDate
            components:(nullable NSMutableArray *)components 
                  from:(nullable NSPort *) receivePort
              reserved:(NSUInteger)headerSpaceReserved;
```
&emsp;`limitDate`：消息发送的最后时刻。

&emsp;`components`：消息组件。

&emsp;`receivePort`：接收端口。

&emsp;`headerSpaceReserved`：为 header 保留的字节数。

&emsp;NSConnection 在适当的时间调用此方法。不应直接调用此方法。此方法可能引发 NSInvalidSendPortException、NSInvalidReceivePortException 或 NSPortSendException，具体取决于发送端口的类型和错误的类型。
### sendBeforeDate:msgid:components:from:reserved:
&emsp;此方法是为具有自定义 NSPort 类型的子类提供的。（NSConnection）
```c++
- (BOOL)sendBeforeDate:(NSDate *)limitDate 
                 msgid:(NSUInteger)msgID 
            components:(NSMutableArray *)components 
                  from:(NSPort *)receivePort 
              reserved:(NSUInteger)headerSpaceReserved;
```
&emsp;`msgID`：message ID。

&emsp;NSConnection 在适当的时间调用此方法。不应直接调用此方法。此方法可能引发 NSInvalidSendPortException、NSInvalidReceivePortException 或 NSPortSendException，具体取决于发送端口的类型和错误的类型。

&emsp;`components` 数组由一系列 NSData 子类的实例和一些NSPort子类的实例组成。由于 NSPort 的一个子类不一定知道如何传输 NSPort 的另一个子类的实例（即使知道另一个子类也可以做到），因此，`components` 数组中的所有 NSPort 实例和 `receivePort` 参数必须属于接收此消息的 NSPort 的同一子类。如果在同一程序中使用了多个 DO transports，则需要格外小心。
### NSPortDidBecomeInvalidNotification
&emsp;从 `invalidate` 方法发布，当解除分配 NSPort 或它发现其通信通道已损坏时调用该方法。通知对象是无效的 NSPort 对象。此通知不包含 userInfo 字典。
```c++
FOUNDATION_EXPORT NSNotificationName const NSPortDidBecomeInvalidNotification;
```
&emsp;NSSocketPort 对象无法检测到其与远程端口的连接何时丢失，即使远程端口位于同一台计算机上。因此，它不能使自己失效并发布此通知。相反，你必须在发送下一条消息时检测超时错误。

&emsp;发布此通知的 NSPort 对象不再有用，因此所有接收者都应该注销自己的任何涉及 NSPort 的通知。接收此通知的方法应在尝试执行任何操作之前检查哪个端口无效。特别是，接收所有 NSPortDidBecomeInvalidNotification 消息的观察者应该知道，与 window server 的通信是通过 NSPort 处理的。如果此端口无效，drawing operations 将导致致命错误。









## 参考链接
**参考链接:🔗**
+ [runloop 源码](https://opensource.apple.com/tarballs/CF/)
+ [Run Loops 官方文档](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Multithreading/RunLoopManagement/RunLoopManagement.html#//apple_ref/doc/uid/10000057i-CH16-SW1)
+ [iOS RunLoop完全指南](https://blog.csdn.net/u013378438/article/details/80239686)
+ [iOS源码解析: runloop的底层数据结构](https://juejin.cn/post/6844904090330234894)
+ [iOS源码解析: runloop的运行原理](https://juejin.cn/post/6844904090166624270)
+ [深入理解RunLoop](https://blog.ibireme.com/2015/05/18/runloop/)
+ [iOS底层学习 - 深入RunLoop](https://juejin.cn/post/6844903973665636360)
+ [一份走心的runloop源码分析](https://cloud.tencent.com/developer/article/1633329)
+ [NSRunLoop](https://www.cnblogs.com/wsnb/p/4753685.html)
+ [iOS刨根问底-深入理解RunLoop](https://www.cnblogs.com/kenshincui/p/6823841.html)
+ [RunLoop总结与面试](https://www.jianshu.com/p/3ccde737d3f3)
+ [Runloop-实际开发你想用的应用场景](https://juejin.cn/post/6889769418541252615)
+ [RunLoop 源码阅读](https://juejin.cn/post/6844903592369848328#heading-17)
+ [do {...} while (0) 在宏定义中的作用](https://www.cnblogs.com/lanxuezaipiao/p/3535626.html)
+ [CFRunLoop 源码学习笔记(CF-1151.16)](https://www.cnblogs.com/chengsh/p/8629605.html)
+ [操作系统大端模式和小端模式](https://www.cnblogs.com/wuyuankun/p/3930829.html)
+ [CFBag](https://nshipster.cn/cfbag/)
