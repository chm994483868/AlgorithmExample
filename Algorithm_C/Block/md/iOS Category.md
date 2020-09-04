# iOS Category

## `extension` 延展(扩展)
&emsp;`extension` 和 `category` 不同，可以声明方法、属性和成员变量，但一般是私有方法、私有属性和私有成员变量。
### `extension` 存在形式
&emsp;`category` 拥有 `.h` 和 `.m` 文件，`extension` 则不然，`extension` 只存在一个 `.h` 文件，或者只能寄生于一个类的 `.m` 中（寄生在 `.m` 中是我们最常见的存在形式）。

+ "寄生" 形式 
比如，在 `BaseViewController.m` 文件中，我们通常会写一个 `extension`:
```objective-c
@interface BaseViewController () {
// 私有成员变量
}

// 私有属性
// 私有方法
@end
```

+ 单独定义 `.h` 文件
&emsp;另外我们也可以单独创建一个 `extension` 文件，
`command` + `N` -> `Objective-C File`，`File Type` 选择 `Extension`，`Class` 输入要创建 `extension` 的类名，`File` 输入相关名字的字符串 `xxx`，点击 `next` 后就会生成一个名字是 `类名+xxx.h` 的 `.h` 文件。如下:
`CusObject+extension.h` 文件：
```objective-c
#import <Foundation/Foundation.h>
#import "CusObject.h"

NS_ASSUME_NONNULL_BEGIN

@interface CusObject () {
    NSString *name;
}

@property (nonatomic, copy) NSString *nameTwo;
- (void)testMethod_Extension;

@end

NS_ASSUME_NONNULL_END
```
在 `CusObject.m` 中引入 `#import "CusObject+extension.h"`：
```objective-c
#import "CusObject.h"
#import "CusObject+extension.h"

@implementation CusObject

-(void)testMethod_Extension {
    NSLog(@"%@", name);
    NSLog(@"%@", self.nameTwo);
}

- (void)dealloc {
    NSLog(@"🍀🍀🍀 CusObject deallocing");
}

@end
```
如果把 `#import "CusObject+extension.h"` 引入放在 `CusObject.m` 中，表示`extension` 中的成员变量、属性和方法都是私有的。

注意：
如果把 `#import "CusObject+extension.h"` 放在 `CusObject.h` 最上面引入，会直接报错，这里有个一个定义先后的问题，此时 `CusObject+extension.h` 处于 `CusObject` 类定义前面，此时 `CusObject` 类定义还没有完成，我们就直接给它写扩展，此时是找不到类定义的。
我们可以把 `#import "CusObject+extension.h"`  放在下面，如下:
```c++
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface CusObject : NSObject

@end

NS_ASSUME_NONNULL_END

#import "CusObject+extension.h"
```
还有无论 `.m` 还是 `.h` 引入，`extension` 中定义的成员变量都是私有的。
```objective-c
objc->name = @"chm"; ❌❌ // Instance variable 'name' is private
```
### `extension` 和 `cateogry` 区别
1. `extension` 可以添加成员变量，`category` 不能添加成员变量。运行时加载类到内存以后，才会加载分类，这时类的内存布局已经确定（编译器还会对成员变量顺序做出优化，保证遵循内存对齐原则下类占用内存容量最少），如果再去添加成员变量就会破坏类的内存布局。在编译时确定的相对于类的起始地址的内存偏移（硬编码），确定各个成员变量的访问地址）。
2. `extension` 在编译期决议（就确定了是类的一部分），`category` 在运行期决议。
  `extension` 在编译期和头文件里的 `@interface` 以及实现文件里的 `@implement` 一起形成一个完整的类，`extension` 伴随类的产生而产生，亦随之一起消亡。  `Category` 中的方法是在运行时决议的，没有实现也可以运行，而 `Extension` 中的方法是在编译器检查的，没有实现会报错。
3. `extension` 一般用来隐藏类的私有信息，无法直接为系统的类扩展，但可以先创建系统类的子类再添加 `extension`。 
4. `category` 可以给系统提供的类添加分类。
5. `extension` 和 `category` 都可以添加属性，但是 `category` 的属性不能生成成员变量以及 `getter` 和 `setter` 方法的实现。

## `Category` 分类(类别)
&emsp;`category` 是 `Objective-C 2.0` 之后添加的语言特性，**它可以在不改变或不继承原类的情况下，动态地给类添加方法**。除此之外还有一些其他的应用场景:
1. 可以把类的的实现分开在几个不同的文件里面。这样做有几个显而易见的好处：
  + 可以减少单个文件的体积。
  + 可以把不同的功能组织到不同的 `category` 里面。
  + 可以由多个开发者共同完成一个类。
  + 可以按需加载想要的 `category`。
  + 声明私有方法。
2. 另外还衍生出 `category` 其他几个场景:
  + 模拟多继承（另外可以模拟多继承的还有 `protocol`）。
  + 把 `framework` 的私有方法公开。
## `category` 特点
1. `category` 只能给某个已有的类扩充方法，不能扩充成员变量。
2. `category` 中也可以添加属性，只不过 `@property` 只会生成 `setter` 和 `getter` 的声明，不会生成 `setter` 和 `getter` 的实现以及成员变量。
3. 如果 `category` 中的方法和类中的原用方法同名，运行时会优先调用 `category` 中的方法，也就是，`category` 中的方法会覆盖掉类中原有的方法，所以开发中尽量保证不要让分类中的方法和原有类中的方法名相同，避免出现这种情况的解决方案是给类的方法名统一添加前缀，比如 `category_`。
4. 如果多个 `category` 中存在同名的方法，运行时到底调用哪个方法由编译器决定，最后一个参与编译的方法会被调用。我们可以在 `Compile Sources` 中拖动不同分类的顺序来测试。
5. 调用优先级，`category` > 本类 > 父类。即优先调用 `category` 中的方法，然后调用本类方法，最后调用父类方法。注意：`category` 是在运行时添加的，不是在编译时。

注意：
+ `category` 的方法没有“完全替换掉”原来类已经有的方法，也就是说如果 `category` 和原来类都有 `methodA`，那么 `category` 附加完成之后，类的方法列表里会有两个 `methodA`。
+ `category` 的方法被放到了新方法列表的前面，而原来类的方法被放到了新方法列表的后面，这也就是我们平常所说的 `category` 的方法会“覆盖”掉原来类的同名方法，这是因为运行时在查找方法的时候是顺着方法列表的顺序查找的，它只要一找到对应名字的方法，就会罢休，殊不知后面可能还有一样名字的方法。

## 为什么 `category` 不能添加成员变量？
&emsp;`Objective-C` 中类是由 `Class` 类型来表示的，它实际上是一个指向 `objc_class` 结构体的指针，如下:
```c++
typedef struct objc_class *Class;
```
`objc_class` 结构体定义如下:
```c++
// objc_class

struct objc_class : objc_object {
// Class ISA;
Class superclass;
cache_t cache;             // formerly cache pointer and vtable
class_data_bits_t bits;    // class_rw_t * plus custom rr/alloc flags

class_rw_t *data() const {
    return bits.data();
}

...
};

// class_data_bits_t

struct class_data_bits_t {
    friend objc_class;

    // Values are the FAST_ flags above.
    uintptr_t bits;
    ...
public:

    class_rw_t* data() const {
        return (class_rw_t *)(bits & FAST_DATA_MASK);
    }
    ...

    // Get the class's ro data, even in the presence of concurrent realization.
    // fixme this isn't really safe without a compiler barrier at least
    // and probably a memory barrier when realizeClass changes the data field
    const class_ro_t *safe_ro() {
        class_rw_t *maybe_rw = data();
        if (maybe_rw->flags & RW_REALIZED) {
            // maybe_rw is rw
            return maybe_rw->ro();
        } else {
            // maybe_rw is actually ro
            return (class_ro_t *)maybe_rw;
        }
    }
    ...
};

// class_rw_t

struct class_rw_t {
    // Be warned that Symbolication knows the layout of this structure.
    uint32_t flags;
    uint16_t witness;
#if SUPPORT_INDEXED_ISA
    uint16_t index;
#endif

    explicit_atomic<uintptr_t> ro_or_rw_ext;

    Class firstSubclass;
    Class nextSiblingClass;
    ...

public:
    ...

    const method_array_t methods() const {
        auto v = get_ro_or_rwe();
        if (v.is<class_rw_ext_t *>()) {
            return v.get<class_rw_ext_t *>()->methods;
        } else {
            return method_array_t{v.get<const class_ro_t *>()->baseMethods()};
        }
    }

    const property_array_t properties() const {
        auto v = get_ro_or_rwe();
        if (v.is<class_rw_ext_t *>()) {
            return v.get<class_rw_ext_t *>()->properties;
        } else {
            return property_array_t{v.get<const class_ro_t *>()->baseProperties};
        }
    }

    const protocol_array_t protocols() const {
        auto v = get_ro_or_rwe();
        if (v.is<class_rw_ext_t *>()) {
            return v.get<class_rw_ext_t *>()->protocols;
        } else {
            return protocol_array_t{v.get<const class_ro_t *>()->baseProtocols};
        }
    }
};

// class_ro_t

struct class_ro_t {
    uint32_t flags;
    uint32_t instanceStart;
    uint32_t instanceSize;
#ifdef __LP64__
    uint32_t reserved;
#endif

    const uint8_t * ivarLayout;
    
    const char * name;
    method_list_t * baseMethodList;
    protocol_list_t * baseProtocols;
    const ivar_list_t * ivars;

    const uint8_t * weakIvarLayout;
    property_list_t *baseProperties;

    ...

    method_list_t *baseMethods() const {
        return baseMethodList;
    }
    ...
};
```
在上面一连串的数据结构定义中，`ivars` 是 `const ivar_list_t *`。在 `runtime` 中， `objc_class` 结构体大小是固定的，不可能往这个结构体中添加数据，只能修改。所以 `ivars` 指向一个固定区域，只能修改成员变量值，不能增加成员变量个数。`methodList` 是一个二维数组，所以可以修改 `*methodList` 的值来增加成员方法，虽没有办法扩展 `methodList` 指向的内存区域，却可以改变这个内存区域的值（存储的指针）。因此，可以动态的添加方法，但是不能添加成员变量。

## `category` 中能添加属性吗？
`category` 不能添加成员变量（`instance variables`），那到底能不能添加属性（`@property`）呢？

从 `category` 的结构体开始分析:
`category_t` 定义:
```c++
// classref_t is unremapped class_t*
typedef struct classref * classref_t;
```
```c++
struct category_t {
    const char *name;
    classref_t cls;
    struct method_list_t *instanceMethods;
    struct method_list_t *classMethods;
    struct protocol_list_t *protocols;
    struct property_list_t *instanceProperties;
    // Fields below this point are not always present on disk.
    struct property_list_t *_classProperties;

    method_list_t *methodsForMeta(bool isMeta) {
        if (isMeta) return classMethods;
        else return instanceMethods;
    }

    property_list_t *propertiesForMeta(bool isMeta, struct header_info *hi);
    
    protocol_list_t *protocolsForMeta(bool isMeta) {
        if (isMeta) return nullptr;
        else return protocols;
    }
};
```
从 `category` 定义中可以看出 `category` 可以添加实例方法、类方法甚至可以实现协议、添加属性，同时也看到不能添加成员变量。
那为什么说不能添加属性呢？实际上，`category` 实际上允许添加属性，可以使用 `@property` 添加，但是能添加 `@property` 不代表可以添加“完整的”属性，通常我们说的添加属性是指编译器为我们生成了成员变量和对应的 `setter` 和 `getter` 方法来存取属性。在 `category` 中虽说可以书写 `@property`，但是不会生成 _成员变量，也不会生成添加属性的 `getter` 和 `setter` 方法的实现，所以尽管添加了属性，也无法使用点语法调用 `setter` 和 `getter` 方法。（实际上，点语法可以写，只不过在运行时调用到这个方法时会报找不到方法的错误: `unrecognized selector sent to instance ....`）。我们此时可以通过 `associated object` 来为属性手动实现 `setter` 和 `getter` 存取方法。可参考: [iOS AssociatedObject 底层实现原理](https://juejin.im/post/6868191269521358855)

## `category` 原理
1. 即使我们不引入 `category` 的头文件，`category` 中的方法也会被添加进主类中，我们可以通 `performSelector:` 等方式对 `category` 中的方法进行调用。
  此时分两种情况:
  1. 

```c++
typedef struct category_t *Category;

struct category_t {
    const char *name;
    classref_t cls;
    struct method_list_t *instanceMethods;
    struct method_list_t *classMethods;
    struct protocol_list_t *protocols;
    struct property_list_t *instanceProperties;
    // Fields below this point are not always present on disk.
    struct property_list_t *_classProperties;

    method_list_t *methodsForMeta(bool isMeta) {
        if (isMeta) return classMethods;
        else return instanceMethods;
    }

    property_list_t *propertiesForMeta(bool isMeta, struct header_info *hi);
    
    protocol_list_t *protocolsForMeta(bool isMeta) {
        if (isMeta) return nullptr;
        else return protocols;
    }
};
```

## 参考链接
**参考链接:🔗**
+ [Objective-C运行时-类别category](https://zhuanlan.zhihu.com/p/161100311)
+ [iOS Extension详解，及与Category的区别](https://www.jianshu.com/p/b45e1dd24e32)
+ [iOS Category详解](https://www.jianshu.com/p/c92b17a36b9e)
+ [iOS Category的使用及原理](https://www.jianshu.com/p/4ce54f78290a)
+ [iOS-分类（Category）](https://www.jianshu.com/p/01911be8ce83)
+ [iOS-Category原理](https://www.jianshu.com/p/9966940fcd9e)
