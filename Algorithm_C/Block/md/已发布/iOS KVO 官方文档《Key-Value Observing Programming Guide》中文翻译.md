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
&emsp;观察对象（observing object）首先通过发送 addObserver:forKeyPath:options:context: 消息向（被观察对象）观察对象（observed object）注册自己，将其自身作为观察者（observer）和要观察的属性的关键路径传递。观察者（observer）还指定了一个 options 参数和一个 context 指针来管理通知的各个方面。
#### Options
```c++
// - (void)addObserver:(NSObject *)observer forKeyPath:(NSString *)keyPath options:(NSKeyValueObservingOptions)options context:(nullable void *)context;
// - (void)observeValueForKeyPath:(nullable NSString *)keyPath ofObject:(nullable id)object change:(nullable NSDictionary<NSKeyValueChangeKey, id> *)change context:(nullable void *)context;

// 用于 -addObserver:forKeyPath:options:context: 和 -addObserver:toObjectsAtIndexes:forKeyPath:options:context: 函数的选项
typedef NS_OPTIONS(NSUInteger, NSKeyValueObservingOptions) {
    // 通知中发送的更改字典（(NSDictionary<NSKeyValueChangeKey,id> *)change）是否应分别包含 NSKeyValueChangeNewKey 和 NSKeyValueChangeOldKey 条目。
    NSKeyValueObservingOptionNew = 0x01,
    NSKeyValueObservingOptionOld = 0x02,
    
    // 在观察者注册方法返回之前，是否应立即将通知发送给观察者。
    // 如果还指定了 NSKeyValueObservingOptionNew，则通知中的更改字典将始终包含 NSKeyValueChangeNewKey 条目，但绝不包含 NSKeyValueChangeOldKey 条目。
    //（在初始通知中，观察到的属性的当前值可能是旧的，但对于观察者却是新的。）
    // 你可以使用此选项，而不是同时显式调用观察者的 -observeValueForKeyPath:ofObject:change:context: 方法。
    // 当此选项与 -addObserver:toObjectsAtIndexes:forKeyPath:options:context: 一起使用时，将向要添加观察者的每个索引对象发送一个通知。
    
    // 当调用 -addObserver:forKeyPath:options:context: 函数注册观察者并在 options 参数中包含 NSKeyValueObservingOptionInitial 选项时，
    // 此时会立即调用一次观察者的 -observeValueForKeyPath:ofObject:change:context: 函数，如果 options 参数是 NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew，
    // 则在 change 字典中将始终包含 NSKeyValueChangeNewKey 值。（如果被观察者的 keyPath 对应的属性的值为 nil 则change 字典中 NSKeyValueChangeNewKey 的值为 NULL）
    
    // 当 options 参数是 NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew 时，change 字典打印结果：（keyPath 对应属性值是 nil/有值 时）
    // {kind = 1; new = "<null>";} {kind = 1; new = CHM;}
    // 当 options 参数是 NSKeyValueObservingOptionInitial 时，打印 change 字典，仅包含 kind: {kind = 1;}
    
    // 即 NSKeyValueObservingOptionInitial 选项的作用仅是为了在注册观察者时收到一次通知。
    //（连带着可以包含 new 值或者不包含 new 值，看开发者意愿，如果选项是 NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionOld 时，change 字典也只包含 kind）
    
    NSKeyValueObservingOptionInitial API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0)) = 0x04,
    
    // 是否应在每次更改之前和之后将单独的通知发送给观察者，而不是在更改之后将单个通知发送给观察者。
    // 更改之前发送的通知中的 chnage 字典始终包含 NSKeyValueChangeNotificationIsPriorKey 条目，其值为 [NSNumber numberWithBool: YES]，但从不包含 NSKeyValueChangeNewKey 条目。
    
    // options 是 NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld | NSKeyValueObservingOptionPrior 时，更改前打印：{kind = 1; notificationIsPrior = 1; old = CHM;}
    // 更改后打印：{kind = 1; new = JAY; old = CHM;}
    // options 是 NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld 时，只收到一个更改后的打印，和上面的同时使用三个选项时是相同的，
    // 更改后打印：{kind = 1; new = JAY; old = CHM;}
    
    // 当观察者自己的 KVO 兼容性要求它为其自身的属性之一调用 -willChange... 方法之一时，可以使用此选项，并且该属性的值取决于所观察对象的属性的值。
    //（在这种情况下，为响应更改后收到的 -observeValueForKeyPath:ofObject:change:context: 消息而轻易地适当调用 -willChange... 为时已晚。）
    
    // 指定此选项后，更改后发送的通知中的 change 字典包含与未指定此选项时将包含的条目相同的条目，但 NSOrderedSets 表示的有序唯 一对多关系 除外。
    // 对于这些更改，对于 NSKeyValueChangeInsertion 和 NSKeyValueChangeReplacement 更改，
    // will-change 通知的 change 字典包含一个 NSKeyValueChangeIndexesKey（和 NSKeyValueChangeOldKey，如果是替换，则在注册时指定 NSKeyValueObservingOptionOld 选项），它给出了操作可能更改的索引（和对象）。
    // 更改之后，第二个通知包含报告实际更改内容的条目。对于 NSKeyValueChangeRemoval 更改，按索引清除是精确的。
    
    NSKeyValueObservingOptionPrior API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0)) = 0x08
};
```
&emsp;options 参数，指定为（多个）选项常量的按位 or（或者单个的选项常量），既会影响通知中提供的更改字典的内容，又会影响生成通知的方式。

&emsp;你可以通过指定选项 NSKeyValueObservingOptionOld 选择从更改之前接收观察到的属性的值（属性的旧值）。你可以使用选项 NSKeyValueObservingOptionNew 来请求属性的新值（属性的旧值）。你可以通过这些选项的按位 or 来接收新旧值。

&emsp;指示观察对象发送带有选项 NSKeyValueObservingOptionInitial 的立即更改通知（在 addObserver:forKeyPath:options:context: 返回之前）。你可以使用此附加的一次性通知在 observer 中建立属性的初始值。（）







&emsp;通过包含选项 NSKeyValueObservingOptionPrior，可以指示观察对象在属性更改之前发送通知（除了更改之后的常规通知之外）。change dictionary通过包含值为 NSNumber wrapping YES 的键 NSKeyValueChangeNotificationIsPriorKey 来表示预更改通知。那把钥匙不在别的地方。当观察者自己的KVO遵从性要求它为依赖于观察到的属性的属性调用-willChange…方法时，可以使用prechange通知。通常的变更后通知来得太晚，无法及时调用willChange。

&emsp;通过包括选项 NSKeyValueObservingOptionPrior，可以指示观察对象在属性更改之前（除了更改之后的常规通知）发送通知。更改字典通过将键 NSKeyValueChangeNotificationIsPriorKey 包括 NSNumber 值包装为 YES 来表示预更改通知。该 key 不存在。当观察者自己的 KVO 合规性要求其根据依赖于观察到的属性的其属性之一调用-willChange…方法之一时，可以使用更改前通知。







































## 参考链接
**参考链接:🔗**
+ [Key-Value Observing Programming Guide](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/KeyValueObserving/KeyValueObserving.html#//apple_ref/doc/uid/10000177-BCICJDHA)
