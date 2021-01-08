# iOS KVO 官方文档《Key-Value Observing Programming Guide》中文翻译

> &emsp;那么下面一起通过官方文档来全面的学习一下 KVO 吧！⛽️⛽️ 

## Introduction to Key-Value Observing Programming Guide（键值观察编程指南简介）
&emsp;键值观察是一种机制，它允许将其他对象的指定属性的更改通知给对象。

> &emsp;Important: 为了了解 key-value observing，你必须首先了解 key-value coding。

### At a Glance
&emsp;键值观察提供了一种机制，该机制允许将其他对象的特定属性的更改通知给对象。对于应用程序中模型层和控制器层（model and controller layers）之间的通信特别有用。 （在 OS X 中，控制器层（controller layer）绑定技术在很大程度上依赖于键值观察。）控制器对象通常观察模型对象的属性，而视图对象通过控制器观察模型对象的属性。但是，此外，模型对象可能会观察其他模型对象（通常是确定从属值何时更改）甚至是自身观察自己（再次确定从属值何时更改）。

&emsp;你可以观察到包括简单属性（attributes）、一对一关系（ to-one relationships）和一对多关系（to-many relationships）的属性。一对多关系的观察者被告知所做更改的类型，以及更改涉及哪些对象。
```c++
@interface BankAccount : NSObject
 
@property (nonatomic) NSNumber* currentBalance;              // An attribute 属性
@property (nonatomic) Person* owner;                         // A to-one relation 一对一关系
@property (nonatomic) NSArray< Transaction* >* transactions; // A to-many relation 一对多关系

@end
```
&emsp;一个简单的示例说明了 KVO 如何在你的应用程序中有用。假设一个 Person 对象与一个 Account 对象进行交互，该 Account 对象代表该人在银行的储蓄帐户。 Person 实例可能需要了解 Account 实例的某些方面何时更改，例如余额（balance）或利率（interestRate）。

![kvo_objects_properties](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/ab9e12bd0d1d49ec8ddd04cd1bc96bda~tplv-k3u1fbpfcp-watermark.image)

&emsp;如果这些属性是 Account 的公共属性，则该 Person 可以定期轮询 Account 以发现其变化，但这当然效率低下，并且通常不切实际。更好的方法是使用 KVO，这类似于在 Account 发生变化时 Person 接收到中断（interrupt ）。

&emsp;要使用 KVO，首先必须确保观察到的对象（在这种情况下为 Account）符合 KVO。通常，如果你的对象继承自 NSObject 并以通常的方式创建属性，则对象及其属性将自动符合 KVO 要求。也可以手动实现合规性。 KVO Compliance 描述了自动和手动键值观察之间的区别，以及如何实现两者。

&emsp;接下来，你必须注册你的 observer instance - Person 和 observed instance - Account。 Person sends an addObserver:forKeyPath:options:context: message to the Account, once for each observed key path, naming itself as the observer

![kvo_objects_add](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/f1914af9c83540b7b2359d6f307a1d16~tplv-k3u1fbpfcp-watermark.image)

&emsp;为了从 Account 接收更改通知，Person 实现了所有观察者（all observers）都必需实现的 observeValueForKeyPath:ofObject:change:context: 方法。每当注册的建路径之一发生更改时，Account 就会将此消息发送给 Person。然后，Person 可以根据更改通知采取适当的措施。

![kvo_objects_observe](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/ffbf4bbac70a4691bc83d972b64dff0b~tplv-k3u1fbpfcp-watermark.image)

&emsp;最后，当不再需要通知时，至少在释放之前，Person 实例必须通过向 Account 发送 removeObserver:forKeyPath: 消息来取消注册。

![kvo_objects_remove](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/3cc232de99304f65bc0d9fceb1f6951a~tplv-k3u1fbpfcp-watermark.image)

&emsp;Registering for Key-Value Observing 描述了注册、接收和取消注册键值观察通知的整个生命周期。

&emsp;KVO 的主要优点是，你不必实现自己的方案来在每次属性更改时发送通知。它定义良好（well-defined）的基础结构（infrastructure）具有框架级别（framework-level）的支持，使其易于采用，通常你不必向项目中添加任何代码。此外，基础结构（infrastructure）已经具备了完整的功能（full-featured），这使得支持单个属性的多个观察者以及依赖值变得很容易。

&emsp;与使用 NSNotificationCenter 的通知不同，没有 central object 为所有观察者（observers）提供更改通知。而是在进行更改时将通知直接发送到观察对象。 NSObject 提供了键值观察的基本实现，因此你几乎不需要重写这些方法。

&emsp;Key-Value Observing Implementation Details  描述了键值观察的实现方式。
## Registering for Key-Value Observing
&emsp;你必须执行以下步骤才能使对象接收到符合 KVO 的属性的键值观察通知：

+ 使用 addObserver:forKeyPath:options:context: 方法向 observer 注册 observed 对象。

+ 实现 observeValueForKeyPath:ofObject:change:context: 在 observer 内部接受更改通知消息。

+ 使用 removeObserver:forKeyPath: 方法注销 observer 当它不再应该接收消息时。至少，在 observer 从内存中释放之前调用此方法。

> &emsp;Important: 并非所有类的所有属性都符合 KVO。你可以按照 KVO Compliance 中所述的步骤，确保自己的类符合 KVO。通常，Apple 提供的框架中的属性只有在有文档记录的情况下才符合 KVO。
### Registering as an Observer
&emsp;观察对象（observing object）首先通过发送 addObserver:forKeyPath:options:context: 消息向（被）观察对象（observed object）注册自己，将其自身作为观察者（observer）和要观察的属性的关键路径传递。观察者（observer）还指定了一个 options 参数和一个 context 指针来管理通知的各个方面。
#### Options
&emsp;options 参数，指定为选项常量的按位 OR，既会影响通知中提供的更改字典的内容，又会影响生成通知的方式。

&emsp;你可以通过指定选项 NSKeyValueObservingOptionOld 选择从更改之前接收观察到的属性的值。你可以使用选项 NSKeyValueObservingOptionNew 来请求属性的新值。你可以通过这些选项的按位 OR 来接收新旧值。

&emsp;指示观察对象发送带有选项 NSKeyValueObservingOptionInitial 的立即更改通知（在 addObserver:forKeyPath:options:context: 返回之前）。你可以使用此附加的一次性通知在 observer 中建立属性的初始值。

&emsp;通过包含选项 NSKeyValueObservingOptionPrior，可以指示观察对象在属性更改之前发送通知（除了更改之后的常规通知之外）。change dictionary通过包含值为NSNumber wrapping YES的键NSKeyValueChangeNotificationIsPriorKey来表示预更改通知。那把钥匙不在别的地方。当观察者自己的KVO遵从性要求它为依赖于观察到的属性的属性调用-willChange…方法时，可以使用prechange通知。通常的变更后通知来得太晚，无法及时调用willChange。

&emsp;通过包括选项 NSKeyValueObservingOptionPrior，可以指示观察对象在属性更改之前（除了更改之后的常规通知）发送通知。更改字典通过将键 NSKeyValueChangeNotificationIsPriorKey 包括 NSNumber 值包装为 YES 来表示预更改通知。该 key 不存在。当观察者自己的 KVO 合规性要求其根据依赖于观察到的属性的其属性之一调用-willChange…方法之一时，可以使用更改前通知。







































## 参考链接
**参考链接:🔗**
+ [Key-Value Observing Programming Guide](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/KeyValueObserving/KeyValueObserving.html#//apple_ref/doc/uid/10000177-BCICJDHA)
