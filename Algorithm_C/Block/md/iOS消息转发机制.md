# iOS消息转发机制
当 OC 对象接收到**无法解读的消息**后，就会启动 “消息转发” 机制。
我们可以经由此过程告诉对象应该如何处理未知消息。消息转发过程大致可以分 3 个阶段：
1. 动态方法解析。
2. 查找备援接收者。
3. 完整的消息转发。

```
resolveInstanceMethod:
resolveClassMethod:

forwardingTargetForSelector:

forwardInvocation:
methodSignatureForSelector:

doesNotRecognizeSelector:
```
`id returnValue = [someObject methodName:parameter];` **someObject** 为 "接收者"(receiver), **methodName** 为 "选择子"(selector)，选择子和参数和称为 **"消息"(message)**。

编译器会将此消息转换为一条**标准的 C 语言函数调用**，所调用的函数也就是消息传递机制中的核心函数 `objc_msgSend()` 
```
// 函数原型如下：
void objc_msgSend(id self, SEL cmd,...)
```
函数接收 2 个或者 2 个以上的参数，前两个参数依次代表接收者、选择子，这两个是一定有的参数，后续的就是消息中的参数，顺序不变。
编译器会将刚才的例子中的消息转换为如下函数:
```
id returnValue = objc_msgSend(someObject, @selector(methodName), parameter);
```
objc_msgSend 函数会依据接收者和选择子的类型调用适当的方法。为此，该方法会在接收者所属的类中搜寻其 “方法列表” (list of methods)，如果能找到**与选择子名称相符的方法**，就跳转到它的实现代码。如果找不到，那就**沿着继承体系继续向上查找**，等找到合适的方法之后再跳转。如果最终还是找不到相符的方法，那就执行 “消息转发” (message-forwarding) 操作。

## 一、动态方法解析
+ 对象在收到无法解读的消息后，首先将调用其所属类的下列类方法：
```
+ (BOOL)resolveInstanceMethod:(SEL)sel;
```
该方法的参数就是那个未知的选择子，其返回值为 BOOL 类型，表示这个类是否能新增一个实例方法用以处理这个选择子。再继续往下执行转发机制之前，本类有机会新增一个处理此选择子的方法。假如尚未实现的方法不是实例方法而是类方法，那么运行期系统就会调用另外一个方法，该方法与 `resolveInstanceMethod:` 类似，叫做 `resolveClassMethod:`。使用这个方法前提是：**相关方法的实现代码已经写好，只等着运行的时候动态插在类里面就可以了。** 如果该方法返回 NO，就会进入下一个阶段: **寻找备援接收者**。

## 二、寻找备援接收者
+ 在这一阶段，运行期系统会询问接收者：**能不能把这条消息转给其他接收者来处理。** 与之对应的处理函数是:
```
- (id)forwardingTargetForSelector:(SEL)aSelector;
```
该方法的参数还是那个未知的选择子，若当前接收者能找到备援接收者（对象），则将其返回，若是找不到，就返回 nil。在一个对象内部，可能还有一系列其他对象，原对象可经由此方法将能够处理该选择子的相关内部对象返回，这样，在外界看来似乎是原对象亲自处理了这个消息。如果该方法返回 nil，就会进入下一个阶段：**完整的消息转发机制**。

## 三、完整的消息转发
+ 如果转发已经来到了这一阶段的话，就需要启动完整的消息转发机制了。首先创建 `NSInvocation` 对象，把与尚未处理的那条消息有关的全部细节封装到里面。此对象包含 `选择子(selector)`、`目标(target)`、及`参数(parameter)`。在触发 `NSInvocation` 对象时，**消息派发系统**会亲自将消息指派给目标对象。此阶段会调用下边的两个方法来转发消息：
```
- (void)forwardInvocation:(NSInvocation *)anInvocation;
- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector;
```
forwardInvocation 方法可以实现的很简单：只需要改变调用目标，使消息在新目标上得以调用即可。
当然，还有一种比较有用的实现方法：先以某种方式改变消息，比如追加参数、替换选择子等。
实现 forwardInvocation 方法时，如果发现某调用操作不应该本类操作，则需要调用 super 的同名方法，直至 NSObject。如果最后调用了 NSObject 类的方法，那么该方法还会继续调用 `doesNotRecognizeSelector`  从而抛出 crash。

**参考链接:**
[Objective-C 的消息转发机制](https://www.jianshu.com/p/03f4a95e43d8)
