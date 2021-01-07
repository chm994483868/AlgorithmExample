# iOS KVC 官方文档《Key-Value Coding Programming Guide》中文翻译

> &emsp;日常开发中我们可能已经非常习惯于使用 valueForKey: 和 setValue:forKey:，不过可能有一些细节我们未深入过，那么下面一起通过官方文档来全面的学习一下 KVC 吧！⛽️⛽️ 

## About Key-Value Coding
&emsp;Key-value coding（键值编码）是由 NSKeyValueCoding 非正式协议启用的一种机制，对象采用这种机制来提供对其属性/成员变量的间接访问。当一个对象符合键值编码时，它的属性/成员变量可以通过一个简洁、统一的消息传递接口（setValue:forKey:）通过字符串参数寻址。这种间接访问机制补充了实例变量（自动生成的 _属性名 ）及其相关访问器方法（getter 方法）提供的直接访问。

&emsp;通常使用访问器方法来访问对象的属性。get 访问器（或 getter）返回属性的值。set 访问器（或 setter）设置属性的值。在 Objective-C 中，还可以直接访问属性的底层实例变量（由编译器生成的对应于属性的由 下划线和属性名拼接构成的实例变量）。以上述任何一种方式访问对象属性都是简单的，但需要调用特定于属性的方法或变量名。随着属性列表的增长或更改，访问这些属性的代码也必须随之增长或更改。相反，键值编码兼容对象提供了一个简单的消息传递接口，该接口在其所有属性中都是一致的。

&emsp;键值编码（key-value coding）是许多其他 Cocoa 技术的基础，例如 key-value observing、Cocoa bindings、Core Data 和 AppleScript-ability。在某些情况下，键值编码（key-value coding）还可以帮助简化代码。

&emsp;在上面提到 "Key-value coding（键值编码）是由 NSKeyValueCoding 非正式协议启用的一种机制"，话说 NSKeyValueCoding 是非正式协议，即它不同于我们常见的 NSCopying、NSCoding 等协议是通过 @protocol 直接来定义的，然后当哪些类要遵循此协议时在其类声明或者类延展后面添加 <NSCopying, NSCoding> 表示该类遵循此协议。而 NSKeyValueCoding 机制则是通过分类来实现的，在 Foundation 框架下有一个 NSKeyValueCoding.h 接口文件，其内部定义了多组分类接口，其中包括：@interface NSObject(NSKeyValueCoding)、@interface NSArray<ObjectType>(NSKeyValueCoding)、@interface NSDictionary<KeyType, ObjectType>(NSKeyValueCoding)、@interface NSMutableDictionary<KeyType, ObjectType>(NSKeyValueCoding)、@interface NSOrderedSet<ObjectType>(NSKeyValueCoding)、@interface NSSet<ObjectType>(NSKeyValueCoding)，其中 NSObject 基类已经实现了 NSKeyValueCoding 机制的所有接口，然后 NSArray、NSDictionary、NSMutableDictionary、NSOrderedSet、NSSet 这些子类则是对 setValue:forKey: 和 valueForKey: 函数进行重载。例如当对一个 NSArray 对象调用 setValue:forKey: 函数时，它内部是对数组中的每个元素调用 setValue:forKey: 函数。当对一个 NSArray 对象调用 valueForKey: 函数时，它返回一个数组，其中包含在数组的每个元素上调用 valueForKey: 的结果。返回的数组将包含 NSNull 元素，指代的是数组中某些元素调用 valueForKey: 函数返回 nil 的情况。

&emsp;那么这里我们先看一下 NSKeyValueCoding 的文档内容，然后再接着看 Key-Value Coding Programming Guide 文档。
### NSKeyValueCoding
&emsp; NSKeyValueCoding 是一种可以通过名称或键间接访问对象属性/成员变量的机制。

&emsp;读取和设置对象的值（属性值/成员变量值）的基本方法是: 
+ setValue:forKey: 它设置由指定键标识的属性的值。
+ valueForKey: 它返回由指定键标识的属性的值。

&emsp;因此，可以以一致的方式读取和设置对象的所有属性/成员变量。

&emsp;访问属性值的默认实现方式依赖于通常由对象实现的访问器（getter）方法（或在需要时直接访问实例变量（_属性名））。
#### valueForKey:
&emsp;返回由给定键标识的属性的值。
```c++
- (id)valueForKey:(NSString *)key;
```
&emsp;`key` 是 receiver 属性之一的名字。返回值是由 `key` 标识的属性的值。Key-Value Coding Programming Guide 中的 Accessor Search Patterns 中描述了 valueForKey: 用于查找要返回的正确值的搜索模式。
#### valueForKeyPath:
&emsp;返回由给定键路径标识的派生属性的值。
```c++
- (id)valueForKeyPath:(NSString *)keyPath;
```
&emsp;`keyPath` 表示属性关系的路径（有一个或多个关系），例如 "department.name" 或 "department.manager.lastName"。返回值是 `keyPath` 标识的派生属性的值。

&emsp;默认实现使用 valueForKey: 获取每个关系的目标对象，并将 valueForKey: 消息的结果返回到最终对象。

&emsp;可能看完上面还是一头雾水，下面我们通过一个例子看一下:
```c++
// Person 类有一个名字是 name 的属性 
@interface Person : NSObject
@property (nonatomic, copy) NSString *name;
@end

// Student 类有一个名字是 person 的属性
@interface Student : NSObject
@property (nonatomic, strong) Person *person;
@end

Person *person = [[Person alloc] init];
person.name = @"CHM";
Student *student = [[Student alloc] init];
student.person = person;

// 通过 valueForKeyPath: 入参为 @"person.name" 来读取 student 下 person 属性的 name
NSLog(@"📪📪 %@", [student valueForKeyPath:@"person.name"]);
```
&emsp;当我们通过 valueForKeyPath: 来读取 Person 的 name 属性值时，通过 @"person.name" 来读取。
#### dictionaryWithValuesForKeys:
&emsp;返回一个字典，其中包含由给定数组中的每个键标识的属性值。
```c++
- (NSDictionary<NSString *,id> *)dictionaryWithValuesForKeys:(NSArray<NSString *> *)keys;
```
&emsp;`keys` 包含标识 receiver 属性的 NSString 数组。返回值是包含 keys 中属性名称作为键的字典，相应的值是相应的属性值。

&emsp;默认实现为 keys 中的每个 key 调用 valueForKey:，并将字典中的 NSNull 值替换为返回的 nil 值（即当某个 key 返回 nil 时，在返回值字典中以 NSNull 替代）。（内部是调用 valueForKey:，不是 valueForKeyPath:，keys 不能包含  @"person.name" 此种形式的 key，如果 receiver 的 valueForUndefinedKey: 未实现的话会 crash）
#### valueForUndefinedKey:
&emsp;由 valueForKey: 调用，当找不到与给定键对应的属性时。
```c++
- (id)valueForUndefinedKey:(NSString *)key;
```
&emsp;`key` 是一个不等于任何 receiver 属性名称的字符串。

&emsp;子类可以重写此方法以返回未定义键的备用值。默认实现引发 NSUndefinedKeyException。
#### mutableArrayValueForKey:
&emsp;返回一个可变数组代理，该代理提供对给定键指定的有序一对多关系的读写访问（即通过 key 访问对象的数组属性）。
```c++
- (NSMutableArray *)mutableArrayValueForKey:(NSString *)key;
```
&emsp;`key` 是一对多关系的名称。返回值是一个可变数组代理，它提供对 key 指定的有序一对多关系的读写访问。

&emsp;添加到可变数组中的对象与 receiver 相关，而从可变数组中删除的对象则不再相关。默认实现识别与 valueForKey: 相同的简单访问器方法和数组访问器方法，并遵循相同的直接实例变量访问策略，但总是返回可变集合代理对象，而不是 valueForKey: 将返回的不可变集合。

&emsp;Key-Value Coding Programming Guide 中的 Accessor Search Patterns 描述了 mutableArrayValueForKey: 使用的搜索模式。

&emsp;如上给 Student 添加一个 `@property (nonatomic, strong) NSMutableArray *array;` 属性时，使用如下三种方式都能正常读取：
```c++
NSLog(@"📢📢 %@", [self.student mutableArrayValueForKey:@"array"]);
NSLog(@"📢📢 %@", [self.student valueForKey:@"array"]);
NSLog(@"📢📢 %@", [self.student valueForKeyPath:@"array"]);
```
&emsp;当使用 mutableArrayValueForKey: 函数读取非数组类型属性时，如 `@property (nonatomic, strong) Person *person;`，使用 `NSLog(@"📢📢 %@", [self.student mutableArrayValueForKey:@"person"]);` 读取则直接 crash，控制台输出: Terminating app due to uncaught exception 'NSInvalidArgumentException', reason: '-[Person count]: unrecognized selector sent to instance 0x600002b4d780'。

&emsp;还有一系列的：mutableArrayValueForKeyPath:/mutableSetValueForKey:/mutableSetValueForKeyPath:/mutableOrderedSetValueForKey:/mutableOrderedSetValueForKeyPath: 函数和 mutableArrayValueForKey: 函数相似。

&emsp;这里我们再详细看一下 mutableArrayValueForKey: 的用途，它可以帮助我们更快速的修改 可变/不可变集合类型的属性。还是上面的测试代码，我们给 Student 添加一个不可变数组属性 `@property (nonatomic, strong) NSArray<Person *> *personArray;`，测试如下代码：
```c++
self.student = [[Student alloc] init];

Person *person2 = [[Person alloc] init];
NSMutableArray *arr = [@[person2, person2, person2] mutableCopy];
self.student.personArray = arr;

NSLog(@"🛂 打印 %@", [self.student mutableArrayValueForKey:@"personArray"]);

[[self.student mutableArrayValueForKey:@"personArray"] removeLastObject];
NSLog(@"🛂🛂 移除后 %@", [self.student mutableArrayValueForKey:@"personArray"]);

[[self.student mutableArrayValueForKey:@"personArray"] addObject:person2];
NSLog(@"🛂🛂🛂 新增后 %@", [self.student mutableArrayValueForKey:@"personArray"]);

[((NSMutableArray *)[self.student valueForKey:@"personArray"]) removeLastObject];
NSLog(@"🛂🛂🛂🛂 valueForKey 移除后 %@", self.student.personArray);

[((NSMutableArray *)[self.student valueForKey:@"personArray"]) addObject:person2];
NSLog(@"🛂🛂🛂🛂🛂 valueForKey 新增后 %@", self.student.personArray);

// 控制台打印:
 🛂 打印 (
    "<Person: 0x600001fdd0e0>",
    "<Person: 0x600001fdd0e0>",
    "<Person: 0x600001fdd0e0>"
)
 🛂🛂 移除后 (
    "<Person: 0x600001fdd0e0>",
    "<Person: 0x600001fdd0e0>"
)
 🛂🛂🛂 新增后 (
    "<Person: 0x600001fdd0e0>",
    "<Person: 0x600001fdd0e0>",
    "<Person: 0x600001fdd0e0>"
)
 🛂🛂🛂🛂 valueForKey 移除后 (
    "<Person: 0x600001fdd0e0>",
    "<Person: 0x600001fdd0e0>"
)
 🛂🛂🛂🛂🛂 valueForKey 新增后 (
    "<Person: 0x600001fdd0e0>",
    "<Person: 0x600001fdd0e0>",
    "<Person: 0x600001fdd0e0>"
)
```
&emsp;可看到通过 mutableArrayValueForKey: 我们可以直接增加和移除 personArray 不可变数组中的内容。如果我们使用 self.student.personArray 的方式的话，则要来回进行读取和赋值。
#### setValue:forKey:
&emsp;将给定键指定的 receiver 的属性设置为给定值。
```c++
- (void)setValue:(id)value forKey:(NSString *)key;
```
&emsp;`key` 是 receiver 属性之一的名称。`value` 是由 `key` 标识的属性的值。

&emsp;如果 key 标识了一对一关系，将 value 指定的对象与 receiver 相关联，如果存在，则取消先前相关的对象的关联。

&emsp;给定一个集合对象（value）和一个标识多对多关系的 key，将集合中包含的对象与 receiver 关联，如果存在以前关联的对象，则取消关联。

&emsp;Key-Value Coding Programming Guide 中的 Accessor Search Patterns 中描述了 setValue:forKey: 使用的搜索模式。

&emsp;在使用引用计数的环境中，如果直接访问实例变量，则将保留值。
#### setValue:forKeyPath:
&emsp;将由给定键路径标识的属性的值设置为给定值。
```c++
- (void)setValue:(id)value forKeyPath:(NSString *)keyPath;
```
&emsp;`value` 是 `keyPath` 标识的属性的值。`keyPath` 表示属性关系的路径（有一个或多个关系），例如 "department.name" 或 "department.manager.lastName"。（同 valueForKeyPath:）

&emsp;此方法的默认实现使用 valueForKey: 获取每个关系的目标对象，并向最终对象发送 setValue:forKey: 消息。（即先读取最终对象然后为其赋值）

&emsp;如果使用此方法，并且目标对象未实现该 `value` 的访问器，默认行为是该对象保留 `value`，而不是复制或分配 `value`。
#### setNilValueForKey:
&emsp;由 setValue:forKey: 调用，当给定标量值的 nil 值时（例如 int 或 float，即使用 setValue:forKey: 给非对象类型的属性赋值时）。
```c++
- (void)setNilValueForKey:(NSString *)key;
```
&emsp;子类可以重写此方法以其他方式处理请求，例如用 0 或前哨值（sentinel value）代替 nil 并再次调用 setValue:forKey: 或直接设置变量。默认实现是引发 NSInvalidArgumentException。

&emsp;这里是针对非对象类型的属性例如 int / float。例如上面 Student 类添加一个 int 类型的属性 `@property (nonatomic, assign) int mark;`，然后我们使用 `[self.student setValue:nil forKey:@"mark"];` 设置的话，会直接 crash，此时我们可以重写 Student 的 setNilValueForKey: 函数来处理此种情况或者防止 crash。而属性是对象类型的话 value 传 nil 不会 crash。
#### setValuesForKeysWithDictionary:
&emsp;使用给定字典中的 value 设置 receiver 的属性，使用 value 对应的 key 来标识 receiver 的属性。
```c++
- (void)setValuesForKeysWithDictionary:(NSDictionary<NSString *,id> *)keyedValues;
```
&emsp;`keyedValues` 一个字典，其 key 标识 receiver 中的属性。receiver 中的属性值设置为字典中的 key 对应的 value。

&emsp;默认实现为 `keyedValues` 中的每个键值对调用  setValue:forKey: ，用 nil 替换 `keyedValues` 中的 NSNull 值。
#### setValue:forUndefinedKey:
&emsp;由 setValue:forKey: 调用，当它找不到给定键的属性时。
```c++
- (void)setValue:(id)value forUndefinedKey:(NSString *)key;
```
&emsp;`key` 一个不等于任何 receiver 属性名称的字符串。

&emsp;子类可以重写此方法以其他方式处理请求。默认实现引发 NSUndefinedKeyException。（同 valueForUndefinedKey:）
#### accessInstanceVariablesDirectly
&emsp;（只读）返回一个布尔值，该值指示 key-value coding 方法在未找到属性的访问器方法时是否应直接访问相应的实例变量。
```c++
@property(class, readonly) BOOL accessInstanceVariablesDirectly;
```
&emsp;如果键值编码方法应在找不到属性的访问器方法时直接访问相应的实例变量，则为 YES，否则为 NO。

&emsp;默认返回 YES。子类可以覆盖它以返回 NO，在这种情况下，key-value coding 方法将无法访问实例变量。
#### validateValue:forKey:error:
&emsp;返回一个布尔值，该值指示给定指针所指定的值对于给定键所标识的属性是否有效。
```c++
- (BOOL)validateValue:(inout id  _Nullable *)ioValue forKey:(NSString *)inKey error:(out NSError * _Nullable *)outError;
```
&emsp;`ioValue` 指向由 `inKey` 标识的属性的新值的指针。此方法可以修改或替换该值以使其有效。`inKey` receiver 属性之一的名称。`inKey` 必须指定一个属性或一对一关系。`outError` 如果必须进行验证并且 `ioValue` 不会转换为有效值，则返回时会包含一个 NSError 对象，该对象描述 `ioValue` 不是有效值的原因。

&emsp;如果 `ioValue` 指向的值对 `inKey` 标识的属性有效，或者该方法能够修改 `ioValue` 的值使其有效，则将布尔值设置为 YES；否则，将布尔值设置为 NO。
#### validateValue:forKeyPath:error:
&emsp;返回一个布尔值，该值指示给定指针指定的值对于相对于接收者的给定键路径是否有效。
```c++
- (BOOL)validateValue:(inout id  _Nullable *)ioValue forKeyPath:(NSString *)inKeyPath error:(out NSError * _Nullable *)outError;
```
&emsp;此方法的默认实现使用 valueForKey: 获取每个关系的目标对象，并返回对该属性调用 validateValue:forKey:error: 方法的结果。

&emsp;好了 NSKeyValueCoding 的内容我们差不多看完了，下面我们接着看 Key-Value Coding Programming Guide 文档。
### Using Key-Value Coding Compliant Objects（使用键值编码兼容对象）
&emsp;对象通常在继承 NSObject（直接或间接）时也会拥有 key-value coding 能力，因为 NSObject 采用 NSKeyValueCoding 协议并为基本方法提供默认实现（实际是 NSObject + NSKeyValueCoding 分类默认实现了 Key-Value Coding 使用的基本方法）。此类对象允许其他对象通过紧凑的消息传递接口执行以下操作：
+ **访问对象属性。** 该协议指定方法，例如泛型 getter 函数：valueForKey: 和泛型 setter 函数：setValue:forKey:，用于通过名称或键（参数化为字符串）访问对象属性。这些方法和相关方法的默认实现使用键来定位基础数据并与之交互，如 **Accessing Object Properties** 中所述。
+ **操作集合属性。** 访问方法的默认实现与对象的集合属性（例如 NSArray` 对象）一起使用，就像其他任何属性一样。此外，如果对象为属性定义了集合访问器方法，则它将启用对集合内容的键值访问。这通常比直接访问更有效，并允许您通过标准化的接口处理自定义集合对象，如 **Accessing Collection Properties** 中所述。
+ **在集合对象上调用集合运算符。** 访问 key-value coding 兼容对象中的集合属性时，可以将集合运算符插入键字符串，如 **Using Collection Operators** 中所述。集合运算符指示默认的 NSKeyValueCoding getter 实现对集合执行操作，然后返回新的、经过筛选的集合版本，或者返回表示集合某些特征的单个值（平均值、总和等）。
+ **访问非对象属性。** 协议的默认实现检测非对象属性，包括标量（scalars，int/float 等）和结构（structures），并自动将它们包装和展开为对象，以便在协议接口上使用，如 **Representing Non-Object Values** 中所述。此外，该协议声明了一种方法（setNilValueForKey:），允许兼容对象在通过键值编码接口对非对象属性设置 nil 值时提供适当的操作。 
+ **通过键路径访问属性。** 当你具有与 key-value coding 兼容的对象的层次结构时，可以使用基于键路径的方法调用来通过单个调用在层次结构内深入（直到最终目标），获取或设置值。（即沿着 @"xxx.xx.x" 一路向下）
### Adopting Key-Value Coding for an Object（为对象采用键值编码）
&emsp;为了使你自己的对象符合键值编码的要求，请确保它们采用 NSKeyValueCoding 非正式协议并实现相应的方法，例如 valueForKey: 作为通用 getter 和 setValue:forKey: 作为通用 setter。幸运的是，如上所述，NSObject 采用了此协议，并为这些和其他必要方法提供了默认实现。因此，如果你从 NSObject（或者其许多子类中的任何一个）派生对象，则许多工作已经为你完成。（实现接口都在 NSObject + NSKeyValueCoding 分类中）

&emsp;为了使默认方法完成它们的工作，需要确保对象的访问器方法和实例变量遵循某些定义良好的（明确的）模式。这允许默认实现根据键值编码的消息查找对象的属性。然后，你可以选择通过提供验证方法和处理某些特殊情况的方法来扩展和自定义键值编码
### Key-Value Coding with Swift
&emsp;默认情况下，从 NSObject 或其子类继承的 Swift 对象的属性符合键值编码。而在 Objective-C 中，属性的访问器和实例变量必须遵循特定的模式，Swift 中的标准属性声明自动保证了这一点。另一方面，协议的许多特性要么不相关，要么使用 Objective-C 中不存在的 native Swift 构造或技术处理得更好。例如，由于所有 Swift 属性都是对象，因此你永远不会使用默认实现对非对象属性的特殊处理。

&emsp;因此，当键值编码协议方法直接转换为 Swift 时，本指南主要关注 Objective-C，在 Objective-C 中，你需要做更多的工作来确保遵从性，并且键值编码通常是最有用的。需要在 Swift 中采用显著不同方法的情况在整个指南中均有说明。

&emsp;有关将 Swift 与 Cocoa 技术结合使用的更多信息，请阅读 Using Swift with Cocoa and Objective-C (Swift 3)。有关 Swift 的完整说明，请阅读 The Swift Programming Language (Swift 3)。
### Other Cocoa Technologies Rely on Key-Value Coding（其他 Cocoa 技术依赖键值编码）
&emsp;符合键值编码的对象可以参与各种依赖于这种访问的 Cocoa 技术，包括：
+ **键值观察（Key-value observing）。** 此机制使对象能够注册由另一个对象的属性更改驱动的异步通知，如 **Key-Value Observing Programming Guide** 中所述。
+ **Cocoa bindings.** 这一系列技术完全实现了 Model-View-Controller 范例，其中模型封装了应用程序数据，视图显示和编辑该数据，并且控制器在两者之​​间进行中介。阅读 **Cocoa Bindings Programming Topics**，以了解有关 Cocoa Bindings 的更多信息。
+ **Core Data.** 
+ **AppleScript.** 这种脚本语言可以直接控制可编写脚本的应用程序和 macOS 的许多部分。Cocoa 的脚本支持利用键值编码来获取和设置可脚本化对象中的信息。NSScriptKeyValueCoding 非正式协议中的方法提供了处理键值编码的附加功能，包括通过索引获取和设置多值键中的键值，以及将键值强制（或转换）为适当的数据类型。AppleScript Overview 提供 AppleScript 及其相关技术的高级概述。
## Accessing Object Properties（访问对象属性）
&emsp;对象（类定义）通常在其接口声明中指定属性，这些属性属于以下几类之一：
+ **Attributes.** 这些是简单值，例如标量（int/float 等）、字符串或布尔值。值对象（例如 NSNumber）和其他不可变类型（例如 NSColor）也被视为 attributes。
+ **To-one relationships.** 它们是自身具有属性的可变对象。对象的属性可以更改，而对象本身不会更改。例如，银行帐户对象可能有一个 owner 属性，该属性是 Person 对象的实例，Person 对象本身有一个 address 属性。owner 的地址可以更改，而不必更改银行账户持有的 owner 参考资料。银行账户的 owner 没有改变，只有他们的地址发生改变。
+ **To-many relationships.** 这些是集合对象。尽管也可以使用自定义集合类，但通常使用 NSArray 或 NSSet 的实例来保存此类集合。

&emsp;清单 2-1 中声明的 BankAccount 对象（类）演示了每种类型的属性。

&emsp;Listing 2-1Properties of the BankAccount object（BankAccount 对象的属性）
```c++
@interface BankAccount : NSObject
 
@property (nonatomic) NSNumber* currentBalance;              // An attribute
@property (nonatomic) Person* owner;                         // A to-one relation
@property (nonatomic) NSArray< Transaction* >* transactions; // A to-many relation
 
@end
```
&emsp;为了维护封装，对象通常为其接口上的属性提供访问器方法。对象的作者可以显式地编写这些方法，也可以依赖编译器自动合成它们。无论哪种方式，使用这些访问器之一的代码的作者都必须在编译代码之前将属性名写入代码中。访问器方法的名称成为使用它的代码的静态部分。例如，给定清单 2-1 中声明的 bank account 对象，编译器合成一个 setter，你可以为 myAccount 实例调用它：
```c++
[myAccount setCurrentBalance:@(100.0)];
```
&emsp;这是直接的，但缺乏灵活性。另一方面，键值编码兼容对象提供了一种更通用的机制，可以使用字符串标识符访问对象的属性。
### Identifying an Object’s Properties with Keys and Key Paths（使用键和键路径识别对象的属性）
&emsp;键（key）是标识特定属性的字符串。通常，按照约定，表示属性的键是代码中显示的属性本身的名称。键必须使用 ASCII 编码，不能包含空格，并且通常以小写字母开头（尽管也有例外，例如在许多类中找到的 URL 属性）。

&emsp;因为清单 2-1 中的 BankAccount 类是符合键值编码的，所以它可以识别的 keys 如：owner、currentBalance 和 transactions，它们是其属性的名称。不需要调用 setCurrentBalance: 方法，你可以通过其键设置值：
```c++
[myAccount setValue:@(100.0) forKey:@"currentBalance"];
```
&emsp;实际上，你可以使用相同的方法设置 myAccount 对象的所有属性，使用不同的键参数。因为参数是字符串，所以它可以是在运行时操作的变量。

&emsp;键路径是一组点（.）分隔键，用于指定要遍历的对象属性序列。序列中第一个 key 的属性相对于接收方，并且每个后续密钥相对于前一个属性的值进行评估。关键路径对于通过单个方法调用深入到对象的层次结构非常有用。

&emsp;例如，假定 Person 和 Address 类也符合键值编码，则应用于银行帐户实例的键路径 owner.address.street 指的是存储在银行帐户所有者地址中的街道字符串的值。

> &emsp;NOTE: 在 Swift 中，你可以使用 #keyPath 表达式，而不是使用字符串来指示键或键路径。正如 Using Swift with Cocoa and Objective-C (Swift 3) guide 中的 “键和键路径” 部分所述，这提供了编译时检查的优势。
### Getting Attribute Values Using Keys（使用键获取属性值）
&emsp;当一个对象采用 NSKeyValueCoding 协议时，它符合键值编码要求。从 NSObject 继承的对象提供了协议基本方法的默认实现，它自动采用具有某些默认行为的协议（NSObject + NSKeyValueCoding 分类中实现的方法）。此类对象至少实现以下基本的基于键的 getters：
+ `valueForKey:` - 返回由键参数命名的属性的值。如果根据 Accessor Search Patterns 中描述的规则找不到由键命名的属性，则对象会向自己发送一个 `valueForUndefinedKey:` 消息。`valueForUndefinedKey:` 的默认实现引发了 NSUndefinedKeyException，但是子类可以重写此行为并更优雅地处理这种情况。 
+ `valueForKeyPath:` - 返回相对于 receiver 的指定键路径的值。键路径序列中的任何对象，如果对特定键不兼容键值编码，即 `valueForKey:` 的默认实现找不到访问器方法，都会收到 `valueForUndefinedKey:` 消息。
+ `dictionaryWithValuesForKeys:` - 返回相对于 receiver 的键数组的值。该方法为数组中的每个键调用 `valueForKey:`。返回的 NSDictionary 包含数组中所有键的值。
> &emsp;NOTE: 集合对象（如 NSArray、NSSet 和 NSDictionary）不能包含 nil 作为值。相反，可以使用 NSNull 对象表示 nil 值。NSNull 提供表示对象属性的 nil 值的单个实例。`dictionaryWithValuesForKeys:` 和相关 `setValuesForKeysWithDictionary:` 的默认实现会自动在 NSNull（在 dictionary 参数中）和 nil（在存储的属性中）之间进行转换。

&emsp;使用键路径寻址属性时，如果键路径中除最后一个键以外的任何键是一个一对多关系（即它引用了一个集合），则返回的值是一个集合，其中包含一对多键右侧键的所有值。例如，请求键路径的 transactions.payee 返回一个数组，其中包含所有 transactions 的所有 payee 对象。这也适用于键路径中的多个数组。键路径 accounts.transactions.payee 返回一个数组，其中包含所有 accounts 中所有 transactions 的所有 payee 对象。（例如上面的例子中我们给 Student 添加一个 `@property (nonatomic, strong) NSArray<Person *> *personArray;` 这样的属性，然后 `NSLog(@"❇️❇️ %@", [self.student valueForKeyPath:@"personArray.name"]);` 这样调用时，打印的就是 personArray 属性中的每个 Person 对象的 name 构成的一个字符串数组。）
### Setting Attribute Values Using Keys（使用键设置属性值）
&emsp;与 getter 一样，key-value coding 兼容的对象还提供了一小部分通用 setter，这些 setter 基于 NSObject 中 NSKeyValueCoding 协议的实现（NSObject + NSKeyValueCoding 分类中实现的方法），具有默认行为：
+ `setValue:forKey:` - 将相对于接收消息的对象的指定键的值设置为给定值。`setValue:forKey:` 的默认实现会自动解包表示标量（int/float 等）和结构（struct）的 NSNumber 和 NSValue 对象，并将它们的值分配给属性。有关包装和展开语义的详细信息，请参见 Representing Non-Object Values。（我们对这里解释一下，例如还是我们上面的 Student 类，它有一个 int 类型的属性 `@property (nonatomic, assign) int mark;`，那么我们使用 setValue:forKey: 时要把 value 包成一个 NSNumber 类型，因为 value 只接收 id 类型，如: `[self.student setValue:@(99) forKey:@"mark"];`，这样我们就直接给 mark 属性赋值了，key-value coding 自动完成了解包赋值。）
  如果指定的键对应于接收 setter 消息的对象是不具有的属性，则该对象会向自己发送一个 `setValue:forUndefinedKey:` 消息且 `setValue:forUndefinedKey:` 的默认实现是引发 NSUndefinedKeyException。但是，子类可以重写此方法以自定义方式处理请求防止 crash。
+ `setValue:forKeyPath:` - 在相对于 receiver 的指定键路径处设置给定值。键路径序列中不符合特定键的键值编码的任何对象都会收到 `setValue:forUndefinedKey:` 消息。
+ `setValuesForKeysWithDictionary:` - 使用指定字典中的值设置 receiver 的属性，使用字典键标识属性。默认实现调用 `setValue:forKey:` 对于每个键值对，根据需要用 nil 替换 NSNull 对象。

&emsp;在默认实现中，当你尝试将非对象属性设置为 nil 值时，键值编码兼容对象会向自己发送 `setNilValueForKey:` 消息。`setNilValueForKey:` 的默认实现引发 NSInvalidArgumentException，但对象可能会重写 `setNilValueForKey:`  以替换默认值或标记值，如  Handling Non-Object Values 中所述。
### Using Keys to Simplify Object Access（使用键简化对象访问）
&emsp;要了解基于键的 getter 和 setter 如何简化代码，请参考以下示例。在 macOS 中，NSTableView 和 NSOutlineView 对象将标识符字符串与其每一列相关联。如果支持该 table 的 model 对象不符合键值编码，则 table 的数据源方法将被迫依次检查每个列标识符，以找到要返回的正确属性，如清单 2-2 所示。此外，将来当你将另一个属性（在本例中是 Person 对象）添加到 model 中时，还必须重新访问数据源方法，添加另一个条件以测试新属性并返回相关值。

&emsp;Listing 2-2 Implementation of data source method without key-value coding（没有键值编码的数据源方法的实现）
```c++
- (id)tableView:(NSTableView *)tableview objectValueForTableColumn:(id)column row:(NSInteger)row {
    id result = nil;
    Person *person = [self.people objectAtIndex:row];
 
    if ([[column identifier] isEqualToString:@"name"]) {
        result = [person name];
    } else if ([[column identifier] isEqualToString:@"age"]) {
        result = @([person age]);  // Wrap age, a scalar, as an NSNumber
    } else if ([[column identifier] isEqualToString:@"favoriteColor"]) {
        result = [person favoriteColor];
    } // And so on...
 
    return result;
}
```
&emsp;另一方面，清单 2-3 展示了同一数据源方法的一个更加紧凑的实现，它利用了一个键值编码兼容的 Person 对象。仅使用 `valueForKey:` getter，数据源方法使用列标识符作为键返回适当的值。除了更短之外，它还更通用，因为在以后添加新列时，只要列标识符始终与模型对象的属性名匹配，它就可以继续工作，而不会发生变化。

&emsp;Listing 2-3 Implementation of data source method with key-value coding（键值编码的数据源方法的实现）
```c++
- (id)tableView:(NSTableView *)tableview objectValueForTableColumn:(id)column row:(NSInteger)row {
    return [[self.people objectAtIndex:row] valueForKey:[column identifier]];
}
```
## Accessing Collection Properties（访问集合属性）
&emsp;与键值编码兼容的对象以公开其他属性的方式公开其多个属性。你可以像使用 `valueForKey:` 和 `setValue:forKey:`（或它们的等效键路径）一样，获取或设置 **集合对象**，就像获取其他任何对象一样。但是，当你想要操纵这些集合的内容时，通常使用协议定义的可变代理方法是最有效的。

&emsp;该协议为访问 **集合对象** 定义了三种不同的代理方法，每种方法都有一个键（key）和一个键路径(keyPath)变体：
+ `mutableArrayValueForKey:` 和 `mutableArrayValueForKeyPath:` 它们返回行为类似于 NSMutableArray 对象的代理对象。
+ `mutableSetValueForKey:` 和 `mutableSetValueForKeyPath:` 它们返回行为类似于 NSMutableSet 对象的代理对象。
+ `mutableOrderedSetValueForKey:` 和 `mutableOrderedSetValueForKeyPath:` 它们返回行为类似于 NSMutableOrderedSet 对象的代理对象。

&emsp;在代理对象上进行操作、向其中添加对象、从中删除对象或替换其中的对象时，协议的默认实现会相应地修改基础属性。这比使用 `valueForKey:` 获取一个不可变的集合对象、创建一个具有更改内容的已修改集合对象，然后再使用 `setValue:forKey:` 为指定属性赋值方便灵活很多。在许多情况下，这些方法提供了额外的好处，即维护集合对象中保存的对象的键值观察遵从性（see Key-Value Observing Programming Guide for details）。
## Using Collection Operators（使用集合运算符）
&emsp;当你向键值编码兼容的对象发送 `valueForKeyPath:` 消息时，可以在键值路径中嵌入一个集合操作符。集合操作符是前面带有 at 符号（@）的一小串关键字中的一个，它指定 getter 在返回数据之前应该执行的操作，以某种方式处理数据。NSObject 提供的 `valueForKeyPath:` 默认实现了此行为。

&emsp;当一个键路径包含一个集合操作符时，该操作符前面的键路径的任何部分（称为 left key path）表示相对于消息的接收者要操作的集合。如果将消息直接发送到集合对象（如 NSArray 实例），则可能会忽略 left key path。操作符后面的键路径部分（称为 right key path）指定运算符应处理的集合中的元素的属性。除 @count 之外的所有集合运算符都需要正确的键路径。图 4-1说明了 operator key path 格式。（下面的代码示例可以帮助我们理解这 3 部分的含义）

&emsp;Figure 4-1 Operator key path format

![]()

&emsp;集合运算符表现出三种基本的行为类型：
+ Aggregation Operators（集合运算符）以某种方式合并集合的对象，并返回通常与在正确的键路径中命名的属性的数据类型匹配的单个对象。 @count 运算符是一个例外，它不使用任何右键路径，并且始终返回 NSNumber 实例。
+ Array Operators（数组运算符）返回一个 NSArray 实例，其中包含命名集合中保存的对象的某些子集。
+ Nesting Operators（嵌套运算符）处理包含其他集合的集合，并根据运算符返回以某种方式组合嵌套集合对象的 NSArray 或 NSSet 实例。

### Sample Data
&emsp;下面的描述包括演示如何调用每个操作符的代码段，以及这样做的结果。这些依赖于 Listing 2-1 所示的 BankAccount 类，它包含一个 Transaction 对象数组。每一个都表示一个简单的 checkbook entry，如 Listing 4-1 所示。（如下是简单的两个类定义）

&emsp;Listing 2-1Properties of the BankAccount object（BankAccount 对象的属性）
```c++
@interface BankAccount : NSObject
 
@property (nonatomic) NSNumber* currentBalance;              // An attribute
@property (nonatomic) Person* owner;                         // A to-one relation
@property (nonatomic) NSArray< Transaction* >* transactions; // A to-many relation
 
@end
```
&emsp;Listing 4-1 Interface declaration for the Transaction object（Transaction 对象的接口声明）
```c++
@interface Transaction : NSObject
 
@property (nonatomic) NSString* payee;   // To whom
@property (nonatomic) NSNumber* amount;  // How much
@property (nonatomic) NSDate* date;      // When
 
@end
```
&emsp;为了便于讨论，假设你的 BankAccount 实例有一个用 Listing 4-1 中所示的数据填充的 transactions 数组，并且你从 BankAccount 对象内部进行示例调用。（self 表示的是一个 BankAccount 对象）

Table 4-1 Example data for the Transactions objects（Transactions 对象的示例数据）
| payee values | amount values formatted as currency | date values formatted as month day, year |
| -- | -- | -- |
| Green Power | $120.00 | Dec 1, 2015 |
| Green Power | $150.00 | Jan 1, 2016 |
| Green Power | $170.00 | Feb 1, 2016 |
| Car Loan | $250.00 | Jan 15, 2016 |
| Car Loan | $250.00 | Feb 15, 2016 |
| Car Loan | $250.00 | Mar 15, 2016 |
| ... | ... | ... |
### Aggregation Operators
&emsp;聚合运算符处理一个数组或一组属性，生成反映集合某些方面的单个值。
#### @avg（求平均值）
&emsp;指定 @avg 运算符时，valueForKeyPath: 读取由集合的每个元素的右键路径指定的属性，将其转换为 double（用 0 代替 nil 值），并计算这些值的算术平均值，然后返回存储在 NSNumber 实例中的结果。

&emsp;要获取 Table 4-1 中的样本数据中的平均交易金额：
```c++
// transactionAverage 是 self.transactions 数组中的每个 Transaction 对象的 amount 属性的平均值。
NSNumber *transactionAverage = [self.transactions valueForKeyPath:@"@avg.amount"];
```
&emsp;transactionAverage 的格式化结果是 456.54 美元。
#### @count
&emsp;当指定 @count 运算符时，valueForKeyPath: 返回的 NSNumber 实例表示集合中的对象数。正确的键路径（如果存在）将被忽略。

&emsp;要获取交易中交易对象的数量：
```c++
// numberOfTransactions 是 self.transactions 数组中的元素的个数。
NSNumber *numberOfTransactions = [self.transactions valueForKeyPath:@"@count"];
```
&emsp;numberOfTransactions 的值为 13。
#### @max
&emsp;当指定 @max 运算符时，valueForKeyPath: 在由右键路径命名的集合条目中搜索并返回最大的条目。搜索使用 compare: 方法进行比较，该方法由许多 Foundation 类（例如 NSNumber 类）定义。因此，由右键路径指示的属性必须包含一个对该消息有意义响应的对象（即集合中的元素必须实现了 compare: 函数）。搜索将忽略 nil 的集合条目。

&emsp;要获取 table 4-1 中列出的 transaction 中的日期值的最大值（即最近一笔事务的日期），请执行以下操作：
```c++
NSDate *latestDate = [self.transactions valueForKeyPath:@"@max.date"];
```
&emsp;格式化的 latestDate 值为 Jul 15, 2016。
#### @min
&emsp;指定 @min 运算符时，valueForKeyPath: 在由右键路径命名的集合项中搜索，并返回最小的项。搜索使用 compare: 方法进行比较，这是由许多基础类（如 NSNumber 类）定义的。因此，右键路径所指示的属性必须包含对该消息作出有意义响应的对象。搜索将忽略 nil 的集合条目。

&emsp;要获取 table 4-1 中列出的 transactions 中最早的 transaction 的日期值中的最小值，请执行以下操作：
```c++
NSDate *earliestDate = [self.transactions valueForKeyPath:@"@min.date"];
```
&emsp;格式化的 earliestDate 值为 Dec 1, 2015。
#### @sum
&emsp;指定 @sum 运算符时，valueForKeyPath: 读取由右键路径为集合的每个元素指定的属性，将其转换为 double（用 0 代替 nil 值），并计算这些值的和。然后返回存储在 NSNumber 实例中的结果。
```c++
NSNumber *amountSum = [self.transactions valueForKeyPath:@"@sum.amount"];
```
&emsp;amountSum 的格式化结果为 $ 5,935.00。
### Array Operators
&emsp;数组运算符使 valueForKeyPath: 返回一个对象数组，该对象数组与右键路径指示的一组特定对象相对应。

> &emsp;IMPORTANT: 如果使用数组运算符时任何叶子对象（leaf objects）为 nil，则 valueForKeyPath: 方法将引发异常。

#### @distinctUnionOfObjects
&emsp;当指定 @distinctUnionOfObjects 运算符时，valueForKeyPath: 创建并返回一个数组，该数组包含与右键路径指定的属性相对应的集合的不同对象。

&emsp;要获得省略重复值的 transactions 中 transaction 的 payee 属性值的集合，请执行以下操作：
```c++
// distinctPayees 是 self.transactions 数组中的每个 Transaction 对象的 payee 的值组成的字符串数组（忽略重复的 payee）。
NSArray *distinctPayees = [self.transactions valueForKeyPath:@"@distinctUnionOfObjects.payee"];
```
&emsp;生成的 distinctPayees 数组包含以下字符串中的每个字符串的一个实例：Car Loan、General Cable、Animal Hospital、Green Power、Mortgage。

> &emsp;NOTE: @unionOfObjects 运算符提供类似的行为，但不删除重复的对象。
#### @unionOfObjects
&emsp;当指定 @unionOfObjects 运算符时，valueForKeyPath: 创建并返回一个数组，该数组包含与右键路径指定的属性相对应的集合的所有对象。与 @distinctUnionOfObjects 不同，不会删除重复的对象。

&emsp;要获取 transactions 中 transaction 的 payee 属性值的集合：
```c++
// payees 是 self.transactions 数组中的每个 Transaction 对象的 payee 的值组成的字符串数组（不忽略重复的 payee）。
NSArray *payees = [self.transactions valueForKeyPath:@"@unionOfObjects.payee"];
```
&emsp;所得的 payees 数组包含以下字符串：Green Power、Green Power、Green Power、Car Loan、Car Loan、Car Loan、General Cable、General Cable、General Cable、Mortgage、Mortgage、Mortgage、Animal Hospital。（注意重复项）

> &emsp;NOTE: 与 @distinctUnionOfArrays 运算符提供类似的行为，但是删除重复的对象。
### Nesting Operators
&emsp;















## 参考链接
**参考链接:🔗**
+ [Key-Value Coding Programming Guide](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/KeyValueCoding/index.html#//apple_ref/doc/uid/10000107i)
+ []()
