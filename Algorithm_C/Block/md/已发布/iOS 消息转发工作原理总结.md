# iOS 消息转发工作原理总结

> &emsp;

## Selector
&emsp;选择器（Selector）是用于选择要为对象执行的方法的名称，或者是在编译源代码时替换该名称的唯一标识符。选择器本身不起任何作用。它只是标识一个方法。唯一使选择器方法名不同于普通字符串的是编译器确保选择器是唯一的。选择器之所以有用，是因为（与运行时一起）它的作用就像一个动态函数指针，对于给定的名称，它会自动指向一个方法的实现，该方法适用于与它一起使用的任何类。假设你有一个方法运行的选择器，类 Dog、Athlete 和 ComputerSimulation（每个类都实现了一个 run 方法）。选择器可以与每个类的实例一起使用，以调用其 run 方法，即使每个类的实现可能不同。
### Getting a Selector
&emsp;编译的选择器是 SEL 类型。获取选择器有两种常见方法：
+ 在编译时，使用编译器指令 @selector。
```c++
SEL aSelector = @selector(methodName);
```
+ 在运行时，使用 NSSelectorFromString 函数，其中 string 是方法的名称：
```c++
SEL aSelector = NSSelectorFromString(@"methodName");
```
&emsp;当你想让你的代码发送一个直到执行阶段才知道其名称的消息时，你可以使用从字符串建立的选取器。
### Using a Selector
&emsp;可以以 selector 为参数调用 performSelector: 方法和其他类似方法来执行 selector 方法。
```c++
SEL aSelector = @selector(run);

[aDog performSelector:aSelector];
[anAthlete performSelector:aSelector];
[aComputerSimulation performSelector:aSelector];
```
&emsp;（你可以在特殊情况下使用此技术，例如在实现使用 Target-Action 设计模式的对象时。通常情况下，你只需直接调用该方法。）
## NSMethodSignature
&emsp;方法的返回值和参数的类型信息记录。
```c++
NS_SWIFT_UNAVAILABLE("NSInvocation and related APIs not available")
@interface NSMethodSignature : NSObject
```
&emsp;使用 NSMethodSignature 对象转发接收对象不响应的消息，尤其是在分布式对象的情况下。通常，你可以使用 NSObject methodSignatureForSelector: instance方法创建 NSMethodSignature 对象（在macOS 10.5 及更高版本中，你还可以使用 signatureWithobjType:）。然后使用它创建一个 NSInvocation 对象，该对象作为参数传递给 forwardInvocation: 消息，以将调用发送到任何其他可以处理该消息的对象。在默认情况下，NSObject 调用 doesNotRecognizeSelector:，这会引发异常。对于分布式对象，NSInvocation 对象使用 NSMethodSignature 对象中的信息进行编码，并发送到消息接收方表示的真实对象。




## NSInvocation
&emsp;




## resolveInstanceMethod:
&emsp;动态地为实例方法的给定选择器（sel）提供实现。
```c++
+ (BOOL)resolveInstanceMethod:(SEL)sel OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);
```
&emsp;`sel`: 要解析（resolve）的选择器（SEL）的名称。

&emsp;Return Value: 如果找到方法并将其添加到 receiver，则为 YES，否则为 NO。（这里可能对 “并将其添加到 receiver” 感到疑惑，即根据 sel 参数在 resolveInstanceMethod: 函数内部为指定的类动态添加（class_addMethod）指定的方法实现。）

&emsp;这个方法和 resolveClassMethod: 允许你为给定的选择器动态地提供一个实现。（一个是实例方法一个是类方法）

&emsp;Objective-C 方法就是一个至少包含两个参数 self 和 _cmd 的 C 函数。使用 class_addMethod 函数，可以将函数作为方法添加到类中。给定以下函数：
```c++
void dynamicMethodIMP(id self, SEL _cmd) {
    // implementation ....
}
```
&emsp;可以使用 resolveInstanceMethod: 将其作为方法（称为 resolveThisMethodDynamically）动态添加到类中，如下所示：
```c++
+ (BOOL)resolveInstanceMethod:(SEL)aSEL {
    if (aSEL == @selector(resolveThisMethodDynamically)) {
          class_addMethod([self class], aSEL, (IMP)dynamicMethodIMP, "v@:");
          return YES;
    }
    
    return [super resolveInstanceMethod:aSel];
}
```

&emsp;This method is called before the Objective-C forwarding mechanism is invoked. If respondsToSelector: or instancesRespondToSelector: is invoked, the dynamic method resolver is given the opportunity to provide an IMP for the given selector first.

&emsp;在调用 Objective-C 转发机制之前调用此方法。如果调用 respondsToSelector: 或 instancesRespondToSelector:，则动态方法解析器（dynamic method resolver）将有机会首先为给定的选择器提供 IMP。
## resolveClassMethod:
&emsp;为类方法的给定选择器（sel）动态提供实现。
```c++
+ (BOOL)resolveClassMethod:(SEL)sel OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);
```
&emsp;此方法允许你动态提供给定选择器的实现，同 resolveInstanceMethod: 函数。（为 sel 动态添加实现）
## forwardingTargetForSelector:
&emsp;返回未识别消息应首先指向的对象。（Returns the object to which unrecognized messages should first be directed.）
```c++
- (id)forwardingTargetForSelector:(SEL)aSelector OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);
```
&emsp;`aSelector`: receiver 未实现的方法的选择器。

&emsp;Return Value: 未识别的消息应首先指向的对象。

&emsp;如果对象实现（或继承）此方法，并返回一个非 nil（非 self）结果，那么返回的对象将被用作新的 receiver 对象，消息调度将恢复到这个新对象（调用返回的新对象的 aSelector 方法）。（显然，如果你从这个方法返回 self，代码将落入一个无限循环。）

&emsp;如果你在非根类（非 NSObject）中实现此方法，如果你的类对于给定的选择器没有要返回的内容，那么你应该返回调用 super 实现的结果（return [super forwardingTargetForSelector:aSelector];）。

&emsp;这种方法使对象有机会在更昂贵的 forwardInvocation: 机制接管之前重定向发送给它的未知消息。当你只想将消息重定向到另一个对象时，这非常有用，并且可以比常规转发快一个数量级。如果转发的目标是捕获 NSInvocation，或者在转发过程中操纵参数或返回值，那么它就没有用了。
## methodSignatureForSelector:
&emsp;返回一个 NSMethodSignature 对象，该对象包含由给定选择器标识的方法的描述。
```c++
- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector OBJC_SWIFT_UNAVAILABLE("");
```
&emsp;`aSelector`: 标识要返回实现地址的方法的选择器。当 receiver 是实例时，aSelector 应该标识实例方法；当 receiver 是类时，aSelector 应该标识类方法。

&emsp;Return Value: 一个 NSMethodSignature 对象，其中包含由 aSelector 标识的方法的描述，如果找不到该方法，则为 nil。

&emsp;该方法用于 protocols 的实现。此方法还用于必须创建 NSInvocation 对象的情况，例如在消息转发期间。如果你的对象维护 delegate 或能够处理它不直接实现的消息，则应该重写此方法以返回适当的方法签名。
## forwardInvocation:
&emsp;被子类重写以将消息转发到其他对象。
```c++
- (void)forwardInvocation:(NSInvocation *)anInvocation OBJC_SWIFT_UNAVAILABLE("");
```
&emsp;当一个对象被发送一条没有相应方法（实现）的消息时，运行时系统给接收者一个机会将消息委托给另一个接收者。它通过创建表示消息的 NSInvocation 对象并向接收方发送 forwardInvocation: 包含此 NSInvocation 对象作为参数的消息来代理消息。然后，接收方的 forwardInvocation: 方法可以选择将消息转发到另一个对象。（如果该对象也不能响应消息，它也将有机会转发消息。）

&emsp;因此，forwardInvocation: 消息允许对象与其他对象建立关系，对于某些消息，这些对象将代替它执行。在某种意义上，转发对象能够 “继承” 它将消息转发到的对象的一些特征。

> &emsp;Important: 要响应对象本身无法识别的方法，除了 forwardInvocation: 之外，还必须重写  methodSignatureForSelector:。转发消息的机制使用从 methodSignatureForSelector: 获取的信息（NSMethodSignature 对象）来创建要转发的 NSInvocation 对象。重写方法必须为给定的选择器提供适当的方法签名，方法可以是预先制定一个方法签名，也可以是向另一个对象请求一个方法签名。

&emsp;forwardInvocation: 方法的实现有两个任务：
+ 定位一个对象，该对象可以响应在 `anInvocation` 中编码的消息。对于所有消息，此对象不必相同。
+ 使用 `anInvocation` 将消息发送到该对象。调用将保存结果，运行时系统将提取该结果并将其传递给原始发送者。

&emsp;在一个简单的情况下，对象只将消息转发到一个目的地（如下面示例中假设的 friend 实例变量），forwardInvocation: 方法可以如下所示：
```c++
- (void)forwardInvocation:(NSInvocation *)invocation {
    SEL aSelector = [invocation selector];
 
    if ([friend respondsToSelector:aSelector])
        [invocation invokeWithTarget:friend];
    else
        [super forwardInvocation:invocation];
}
```
&emsp;转发的消息必须具有固定数量的参数；不支持可变数量的参数（采用 printf() 样式）。

&emsp;转发消息的返回值将返回给原始发件人。所有类型的返回值都可以传递给发送方：id 类型、结构体（structures）、双精度浮点数。

&emsp;forwardInvocation: 方法的实现可以做的不仅仅是转发消息。forwardInvocation: 例如，可以用于合并响应各种不同消息的代码，从而避免了为每个选择器编写单独方法的必要性。forwardInvocation: 方法可能还会在对给定消息的响应中包含其他几个对象，而不是只将其转发给一个对象。

&emsp;NSObject 的 forwardInvocation: 实现只是调用 doesNotRecognizeSelector: 方法；它不转发任何消息。因此，如果选择不实现 forwardInvocation:，则向对象发送无法识别的消息将引发异常。
## doesNotRecognizeSelector:
&emsp;处理 receiver 无法识别的消息。
```c++
- (void)doesNotRecognizeSelector:(SEL)aSelector;
```
&emsp;`aSelector`: 用于标识 receiver 未实现或识别的方法的选择器。

&emsp;每当对象接收到它无法响应或转发的 aSelector 消息时，运行时系统就会调用此方法。此方法反过来引发 NSInvalidArgumentException，并生成错误消息。

&emsp;任何 doesNotRecognizeSelector: 消息通常只由运行时系统发送。但是，可以在程序代码中使用它来防止方法被继承。例如，NSObject 子类可能会放弃 copy 或 init 方法，方法是重新实现它以包含 doesNotRecogniteSelector: 消息，如下所示：
```c++
- (id)copy {
    [self doesNotRecognizeSelector:_cmd];
}
```
&emsp;_cmd 变量是传递给当前选择器的每个方法的隐藏参数；在本例中，它标识 copy 方法的选择器。此代码防止子类的实例响应 copy 消息，或阻止超类转发 copy 消息，尽管 respondsToSelector: 仍将报告 receiver 有权访问 copy 方法（即 [self respondsToSelector:@selector(copy)] 调用返回 YES）。

&emsp;如果重写此方法，则必须在实现结束时调用 super 或引发 NSInvalidArgumentException 异常。换句话说，这个方法不能正常返回；它必须总是导致抛出异常。




## 参考链接
**参考链接:🔗**
+ [Selector](https://developer.apple.com/library/archive/documentation/General/Conceptual/DevPedia-CocoaCore/Selector.html#//apple_ref/doc/uid/TP40008195-CH48)
