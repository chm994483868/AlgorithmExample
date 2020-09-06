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
5. `extension` 和 `category` 都可以添加属性，但是 `category` 中的属性不能生成成员变量以及 `getter` 和 `setter` 方法的实现。
6. `extension` 不能像 `category` 那样拥有独立的实现部分（`@implementation` 部分），`extension` 所声明的方法必须依托对应类的实现部分来实现。

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

## 从 `clang` 编译文件来验证上面两个问题
&emsp;我们先用 `clang` 编译文件（这里建议大家在 `xcode` 和终端上自己试一下）。首先定义如下类 `CustomObject` 只声明一个属性:
```objective-c
// CustomObject.h
#import <Foundation/Foundation.h>
NS_ASSUME_NONNULL_BEGIN
@interface CustomObject : NSObject
@property (nonatomic, copy) NSString *customProperty;
@end
NS_ASSUME_NONNULL_END
// CustomObject.m
#import "CustomObject.h"
@implementation CustomObject
@end
```
然后打开终端进入到 `CustomObject.m` 文件所在文件夹，执行 `clang -rewrite-objc CustomObject.m` 指令，然后生成 `CustomObject.cpp` 文件，查看它：
`struct CustomObject_IMPL` 定义：
```c++
extern "C" unsigned long OBJC_IVAR_$_CustomObject$_customProperty;
struct CustomObject_IMPL {
    struct NSObject_IMPL NSObject_IVARS;
    NSString * _Nonnull _customProperty;
};

// @property (nonatomic, copy) NSString *customProperty;

/* @end */
```
看到为我们增加了 `_customProperty` 成员变量，`NSObject_IVARS` 是每个继承自 `NSObject` 都会有的成员变量。
`@implementation CustomObject` 部分：
```c++
// @implementation CustomObject

static NSString * _Nonnull _I_CustomObject_customProperty(CustomObject * self, SEL _cmd) { return (*(NSString * _Nonnull *)((char *)self + OBJC_IVAR_$_CustomObject$_customProperty)); }
extern "C" __declspec(dllimport) void objc_setProperty (id, SEL, long, id, bool, bool);

static void _I_CustomObject_setCustomProperty_(CustomObject * self, SEL _cmd, NSString * _Nonnull customProperty) { objc_setProperty (self, _cmd, __OFFSETOFIVAR__(struct CustomObject, _customProperty), (id)customProperty, 0, 1); }
// @end
```
看到我们的 `customProperty` 的 `setter` 和 `getter` 方法，到这里即可印证：**编译器自动生成了成员变量和对应的 setter 和 getter 方法。** 接着向下：
看到 `getter` 函数的实现:
```c++
return (*(NSString * _Nonnull *)((char *)self + OBJC_IVAR_$_CustomObject$_customProperty));
```
`self` 是我们的入参 `CustomObject * self`，然后它做了一个指针加法。这个 `OBJC_IVAR_$_CustomObject$_customProperty` 是什么呢？我们直接在这个文件 `command + f` 全局搜，其他 3 处分别如下:
```c++
// 1 定义，其实它是一个 unsigned long 
extern "C" unsigned long OBJC_IVAR_$_CustomObject$_customProperty;

// 2 _customProperty 成员变量位置相对 struct CustomObject 的偏移
#define __OFFSETOFIVAR__(TYPE, MEMBER) ((long long) &((TYPE *)0)->MEMBER)
extern "C" unsigned long int OBJC_IVAR_$_CustomObject$_customProperty __attribute__ ((used, section ("__DATA,__objc_ivar"))) =
__OFFSETOFIVAR__(struct CustomObject, _customProperty);

// 3 成员变量列表，看到只有我们的 _customProperty
static struct /*_ivar_list_t*/ {
    unsigned int entsize;  // sizeof(struct _prop_t)
    unsigned int count;
    struct _ivar_t ivar_list[1];
} _OBJC_$_INSTANCE_VARIABLES_CustomObject __attribute__ ((used, section ("__DATA,__objc_const"))) = {
    sizeof(_ivar_t),
    1,
    {{(unsigned long int *)&OBJC_IVAR_$_CustomObject$_customProperty, "_customProperty", "@\"NSString\"", 3, 8}}
};

// _ivar_t 定义
struct _ivar_t {
    // 指向 ivar 偏移位置的指针
    unsigned long int *offset;  // pointer to ivar offset location
    const char *name;
    const char *type;
    unsigned int alignment;
    unsigned int  size;
};
```
看到成员变量的访问是通过指针偏移来做的，而偏移距离都是结构体内存布局已经死死固定的。并且当 `category` 整合到它对应的类时，类的布局已固定，自然就不能再给它添加新的成员变量了。
`CustomObject.cpp` 下面还有属性和方法列表的结构体，这里不再展开了。

下面我们 `clang` 编译 `category` 文件：
`NSObject+customCategory.h` 文件：
```c++
#import <Foundation/Foundation.h>
NS_ASSUME_NONNULL_BEGIN
@interface NSObject (customCategory)
@property (nonatomic, copy) NSString *categoryProperty_one;
@property (nonatomic, strong) NSMutableArray *categoryProperty_two;
- (void)customInstanceMethod_one;
- (void)customInstanceMethod_two;
+ (void)customClassMethod_one;
+ (void)customClassMethod_two;
@end
NS_ASSUME_NONNULL_END
```
`NSObject+customCategory.m` 文件：
```c++
#import "NSObject+customCategory.h"
@implementation NSObject (customCategory)
- (void)customInstanceMethod_one {
    NSLog(@"🧑‍🍳 %@ invokeing", NSStringFromSelector(_cmd));
}
- (void)customInstanceMethod_two {
    NSLog(@"🧑‍🍳 %@ invokeing", NSStringFromSelector(_cmd));
}
+ (void)customClassMethod_one {
    NSLog(@"🧑‍🍳 %@ invokeing", NSStringFromSelector(_cmd));
}
+ (void)customClassMethod_two {
    NSLog(@"🧑‍🍳 %@ invokeing", NSStringFromSelector(_cmd));
}
@end
```
下面开始浏览摘录 `NSObject+customCategory.cpp` 文件:
```c++
// @implementation NSObject (customCategory)
static void _I_NSObject_customCategory_customInstanceMethod_one(NSObject * self, SEL _cmd) {
    NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_NSObject_customCategory_740f85_mi_0, NSStringFromSelector(_cmd));
}
static void _I_NSObject_customCategory_customInstanceMethod_two(NSObject * self, SEL _cmd) {
    NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_NSObject_customCategory_740f85_mi_1, NSStringFromSelector(_cmd));
}
static void _C_NSObject_customCategory_customClassMethod_one(Class self, SEL _cmd) {
    NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_NSObject_customCategory_740f85_mi_2, NSStringFromSelector(_cmd));
}
static void _C_NSObject_customCategory_customClassMethod_two(Class self, SEL _cmd) {
    NSLog((NSString *)&__NSConstantStringImpl__var_folders_24_5w9yv8jx63bgfg69gvgclmm40000gn_T_NSObject_customCategory_740f85_mi_3, NSStringFromSelector(_cmd));
}
// @end
```
看到只有我们的两个实例方法和两个类方法，没有添加成员变量也没有任何属性的 `setter` 和 `getter` 方法。到这里即可印证：**category 不能添加属性。**
```c++
// 两个实例方法
static struct /*_method_list_t*/ {
    unsigned int entsize;  // sizeof(struct _objc_method)
    unsigned int method_count;
    struct _objc_method method_list[2];
} _OBJC_$_CATEGORY_INSTANCE_METHODS_NSObject_$_customCategory __attribute__ ((used, section ("__DATA,__objc_const"))) = {
    sizeof(_objc_method),
    2,
    {{(struct objc_selector *)"customInstanceMethod_one", "v16@0:8", (void *)_I_NSObject_customCategory_customInstanceMethod_one},
    {(struct objc_selector *)"customInstanceMethod_two", "v16@0:8", (void *)_I_NSObject_customCategory_customInstanceMethod_two}}
};

// 两个类方法
static struct /*_method_list_t*/ {
    unsigned int entsize;  // sizeof(struct _objc_method)
    unsigned int method_count;
    struct _objc_method method_list[2];
} _OBJC_$_CATEGORY_CLASS_METHODS_NSObject_$_customCategory __attribute__ ((used, section ("__DATA,__objc_const"))) = {
    sizeof(_objc_method),
    2,
    {{(struct objc_selector *)"customClassMethod_one", "v16@0:8", (void *)_C_NSObject_customCategory_customClassMethod_one},
    {(struct objc_selector *)"customClassMethod_two", "v16@0:8", (void *)_C_NSObject_customCategory_customClassMethod_two}}
};

// 两个属性
static struct /*_prop_list_t*/ {
    unsigned int entsize;  // sizeof(struct _prop_t)
    unsigned int count_of_properties;
    struct _prop_t prop_list[2];
} _OBJC_$_PROP_LIST_NSObject_$_customCategory __attribute__ ((used, section ("__DATA,__objc_const"))) = {
    sizeof(_prop_t),
    2,
    {{"categoryProperty_one","T@\"NSString\",C,N"},
    {"categoryProperty_two","T@\"NSMutableArray\",&,N"}}
};
```
看到类方法、实例方法和属性的结构体：
```c++
static struct _category_t _OBJC_$_CATEGORY_NSObject_$_customCategory __attribute__ ((used, section ("__DATA,__objc_const"))) = 
{
    "NSObject",
    0, // &OBJC_CLASS_$_NSObject,
    (const struct _method_list_t *)&_OBJC_$_CATEGORY_INSTANCE_METHODS_NSObject_$_customCategory,
    (const struct _method_list_t *)&_OBJC_$_CATEGORY_CLASS_METHODS_NSObject_$_customCategory,
    0,
    (const struct _prop_list_t *)&_OBJC_$_PROP_LIST_NSObject_$_customCategory,
};
```
以上三者构成 `_category_t` 实例。

## `category` 原理
> 1. 即使我们不引入 `category` 的头文件，`category` 中的方法也会被添加进主类中，我们可以通 `performSelector:` 等方式对 `category` 中的方法进行调用: 
  + 将 `category` 和它的主类（或元类）注册到哈希表中。
  + 如果主类（或元类）已实现，那么重建它的方法列表。
  2. 这里分了两种情况进行处理：
  + `category` 中的实例方法和属性被整合到主类中。
  + 类方法则被整合到元类中。
  + 对协议的处理比较特殊，`category` 中的协议被同时整合到主类和元类中。
  3. 最终都是通过调用 `static void remethodizeClass(Class cls)` 函数来重新整理类的数据。 

## `category` 相关数据结构
&emsp;到这里突然有些茫然，不知道从哪里入手，已知 `category` 是在 `runtime` 初始化时开始加载的，这里涉及到 `runtime` 的加载流程。暂且不表。我们还是先来一层一层剥开数据结构：

### `category_t`
```c++
typedef struct category_t *Category;

// classref_t is unremapped class_t*
typedef struct classref * classref_t;

struct category_t {
    const char *name; // 分类的名字
    classref_t cls; // 所属的类 
    struct method_list_t *instanceMethods; // 实例方法列表
    struct method_list_t *classMethods; // 类方法列表
    struct protocol_list_t *protocols; // 协议列表
    struct property_list_t *instanceProperties; // 实例属性列表
    // Fields below this point are not always present on disk.
    struct property_list_t *_classProperties; // 类属性？
    
    // 返回 类/元类 方法列表
    method_list_t *methodsForMeta(bool isMeta) {
        if (isMeta) return classMethods;
        else return instanceMethods;
    }

    property_list_t *propertiesForMeta(bool isMeta, struct header_info *hi);
    
    // 协议列表，元类没有协议列表
    protocol_list_t *protocolsForMeta(bool isMeta) {
        if (isMeta) return nullptr;
        else return protocols;
    }
};
```

### `method_t`
&emsp;方法的数据结构，很简单。
```c++
struct method_t {
    SEL name; // 方法名、选择子
    const char *types; // 方法类型
    
    // using MethodListIMP = IMP;
    MethodListIMP imp; // 方法实现

    // 根据选择子的地址进行排序
    struct SortBySELAddress :
        public std::binary_function<const method_t&,
                                    const method_t&, bool>
    {
        bool operator() (const method_t& lhs,
                         const method_t& rhs)
        { return lhs.name < rhs.name; }
    };
};
```
可参考 [stl 中 std::binary_function 的使用](https://blog.csdn.net/tangaowen/article/details/7547475)

### `entsize_list_tt` 

下面先看一下超长的 `entsize_list_tt`，它可理解为一个数据容器，拥有自己的迭代器用于遍历所有元素。 
（这里盲猜，`ent` 应该是 `entry` 的缩写）

```c++
/***********************************************************************
* entsize_list_tt<Element, List, FlagMask>
* Generic implementation of an array of non-fragile structs.
*
* Element is the struct type (e.g. method_t)
* Element 是结构体类型，如: method_t

* List is the specialization of entsize_list_tt (e.g. method_list_t)
* List 是 entsize_list_tt 指定类型，如: method_list_t

* FlagMask is used to stash extra bits in the entsize field
*   (e.g. method list fixup markers)
* 标记位
* FlagMask 用于将多余的位藏匿在 entsize 字段中
* 如: 方法列表修复标记
**********************************************************************/
template <typename Element, typename List, uint32_t FlagMask>
struct entsize_list_tt {
    // entsize 和 flags，还没有看清其含义，待研究... 
    uint32_t entsizeAndFlags;
    // 容器的容量
    uint32_t count;
    // 第一个元素
    Element first;
    
    // 元素的大小
    uint32_t entsize() const {
        return entsizeAndFlags & ~FlagMask;
    }
    
    // 取出 flags
    uint32_t flags() const {
        return entsizeAndFlags & FlagMask;
    }

    // 根据索引返回指定元素的的引用，这 i 可以等于 count
    // 意思是可以返回最后一个元素的后面
    Element& getOrEnd(uint32_t i) const {
        // 断言，i 不能超过 count
        ASSERT(i <= count);
        // 首先取出 first 地址然后指针偏移 i * ensize() 个长度
        // 然后转换为 Element 指针，然后取出指针指向内容返回
        // 返回类型是 Element 引用
        return *(Element *)((uint8_t *)&first + i*entsize()); 
    }
    // 在索引范围内返回 Element 引用
    Element& get(uint32_t i) const { 
        ASSERT(i < count);
        return getOrEnd(i);
    }
    
    // 容器占用的总字节长度吗？（既一个元素的长度乘以总元素的个数吗？）
    size_t byteSize() const {
        return byteSize(entsize(), count);
    }
    
    // entsize 应该是一个元素的长度，然后 count 是元素的数量
    static size_t byteSize(uint32_t entsize, uint32_t count) {
        // 首先算出 struct entsize_list_tt 的长度：
        // uint32_t entsizeAndFlags + uint32_t count + Element first
        // 三个成员变量的总长度，然后加上 (count - 1) 个元素的长度
        return sizeof(entsize_list_tt) + (count-1)*entsize;
    }

    // 复制一份 List
    List *duplicate() const {
        // 开辟 byteSize() 长度空间并置为 1
        auto *dup = (List *)calloc(this->byteSize(), 1);
        // 成员变量赋值
        dup->entsizeAndFlags = this->entsizeAndFlags;
        dup->count = this->count;
        // 原数据的从 begin() 到 end() 的内容复制到以 dup->begin()
        // 为起始地址的空间内
        std::copy(begin(), end(), dup->begin());
        return dup;
    }
    
    // 自定义的迭代器的声明，实现在下面
    struct iterator;
    
    const iterator begin() const {
        // static_cast 是一个 c++ 运算符，功能是把一个表达式转换为某种类型，
        // 但没有运行时类型检查来保证转换的安全性。
        // 把 this 强制转换为 const List *
        // 0，对应下面 iterator 的构造函数实现可知，
        // 把 element 指向第 1 个元素
        
        // 即返回指向容器第一个元素的迭代器
        return iterator(*static_cast<const List*>(this), 0); 
    }
    
    // 同上，少了两个 const 修饰，前面的 const 表示函数返回值为 const 不可变
    // 后面的 const 表示函数执行过程中不改变原始对象里的内容
    iterator begin() { 
        return iterator(*static_cast<const List*>(this), 0); 
    }
    
    // 即返回指向容器最后一个元素的后面的迭代器，
    // 注意这里不是指向最后一个元素，
    // 而是指向最后一个的后面
    const iterator end() const { 
        return iterator(*static_cast<const List*>(this), count); 
    }
    
    // 同上，去掉了两个 const 限制
    iterator end() { 
        return iterator(*static_cast<const List*>(this), count); 
    }
    
    // 下面是自定义的迭代器
    struct iterator {
        // 每个元素的大小
        uint32_t entsize;
        // 当前迭代器的索引
        uint32_t index;  // keeping track of this saves a divide in operator-
        // 元素指针
        Element* element;

        // 类型定义
        typedef std::random_access_iterator_tag iterator_category;
        typedef Element value_type;
        typedef ptrdiff_t difference_type;
        typedef Element* pointer;
        typedef Element& reference;
        
        // 构造函数
        iterator() { }
        
        // 构造函数
        iterator(const List& list, uint32_t start = 0)
            : entsize(list.entsize())
            , index(start)
            , element(&list.getOrEnd(start))
        { }

        // 重载操作符
        const iterator& operator += (ptrdiff_t delta) {
            // 指针偏移
            element = (Element*)((uint8_t *)element + delta*entsize);
            // 更新 index
            index += (int32_t)delta;
            // 返回 *this
            return *this;
        }
        
        const iterator& operator -= (ptrdiff_t delta) {
            element = (Element*)((uint8_t *)element - delta*entsize);
            index -= (int32_t)delta;
            return *this;
        }
        
        // 以下都是 += 和 -= 的应用
        const iterator operator + (ptrdiff_t delta) const {
            return iterator(*this) += delta;
        }
        const iterator operator - (ptrdiff_t delta) const {
            return iterator(*this) -= delta;
        }

        iterator& operator ++ () { *this += 1; return *this; }
        iterator& operator -- () { *this -= 1; return *this; }
        iterator operator ++ (int) {
            iterator result(*this); *this += 1; return result;
        }
        iterator operator -- (int) {
            iterator result(*this); *this -= 1; return result;
        }
        
        // 两个迭代器的之间的距离
        ptrdiff_t operator - (const iterator& rhs) const {
            return (ptrdiff_t)this->index - (ptrdiff_t)rhs.index;
        }

        // 返回元素指针或引用
        Element& operator * () const { return *element; }
        Element* operator -> () const { return element; }
        operator Element& () const { return *element; }

        // 判等，看到的是直接比较 element 的地址
        // 哦哦，也不是，== 可能被抽象类型 Element 重写了
        bool operator == (const iterator& rhs) const {
            return this->element == rhs.element;
        }
        // 不等
        bool operator != (const iterator& rhs) const {
            return this->element != rhs.element;
        }
        
        // 比较
        bool operator < (const iterator& rhs) const {
            return this->element < rhs.element;
        }
        bool operator > (const iterator& rhs) const {
            return this->element > rhs.element;
        }
    };
};
```
`FlagMask` 含义等接下来分析 `method_list_t` 时再展开。

### `method_list_t`
```c++
// Two bits of entsize are used for fixup markers.
// entsize 的后两位用于固定标记
struct method_list_t : entsize_list_tt<method_t, method_list_t, 0x3> {
    bool isUniqued() const;
    bool isFixedUp() const;
    void setFixedUp();
    
    // 返回指定 meth 的 index
    //（指针距离除以元素宽度）
    uint32_t indexOfMethod(const method_t *meth) const {
        uint32_t i = 
            (uint32_t)(((uintptr_t)meth - (uintptr_t)this) / entsize());
        ASSERT(i < count);
        return i;
    }
};
```
在 `objc-runtime-new.mm` 看下 `method_list_t` 的函数实现:
```c++
static uint32_t uniqued_method_list = 1;
bool method_list_t::isUniqued() const {
    return (flags() & uniqued_method_list) != 0;
}

static uint32_t fixed_up_method_list = 3;
bool method_list_t::isFixedUp() const {
    return flags() == fixed_up_method_list;
}

void method_list_t::setFixedUp() {
    runtimeLock.assertLocked();
    ASSERT(!isFixedUp());
    entsizeAndFlags = entsize() | fixed_up_method_list;
}
```
```c++
/*
  Low two bits of mlist->entsize is used as the fixed-up marker.
  method_list_t 的 entsize 的低两位用作固定标记。
  
  PREOPTIMIZED VERSION:
  预优化版本：
  
    Method lists from shared cache are 1 (uniqued) or 3 (uniqued and sorted).
    来自 shared cache 的 Method lists 为 1（唯一）或 3（唯一且已排序）
    
    (Protocol method lists are not sorted because of their extra parallel data)
    Runtime fixed-up method lists get 3.
    Runtime 固定方法列表获取 3，（这里是指 method_list_t 继承 entsize_list_tt 的模版参数 hardcode 是 0x3 吗？）
  UN-PREOPTIMIZED VERSION:
  未预优化版本：
  
    Method lists from shared cache are 1 (uniqued) or 3 (uniqued and sorted)
    来自 shared cache 的 Method lists 为 1（唯一）或 3（唯一且已排序）
    
    Shared cache's sorting and uniquing are not trusted, but do affect the
    location of the selector name string.
    
    Runtime fixed-up method lists get 2.
*/

static uint32_t fixed_up_method_list = 3;
static uint32_t uniqued_method_list = 1;
```
`method_list_t` 的 `FlagMask` 是 `0x3`，即二进制: `0b11`。这里还没有看懂这个逻辑，先向下，等下再回过头来看...

### `protocol_list_t`
```c++
struct protocol_list_t {
    // count is pointer-sized by accident.
    // 是指 count 是根据指针宽度计算的吗？
    uintptr_t count;
    
    // typedef uintptr_t protocol_ref_t;  // protocol_t *, but unremapped
    // protocol_ref_t 为 protocol_t * 
    // 此处虽然数组长度用的 0，不过它是运行期可变大小的
    protocol_ref_t list[0]; // variable-size
    
    // 字节容量，同 entsize_list_tt，但是这里 count 没有减 1
    // 因为数组初始用的 0 
    size_t byteSize() const {
        return sizeof(*this) + count*sizeof(list[0]);
    }

//    static inline void *
//    memdup(const void *mem, size_t len)
//    {
//        void *dup = malloc(len);
//        memcpy(dup, mem, len);
//        return dup;
//    }

    // 复制函数
    protocol_list_t *duplicate() const {
        return (protocol_list_t *)memdup(this, this->byteSize());
    }

    // 类型定义
    typedef protocol_ref_t* iterator;
    typedef const protocol_ref_t* const_iterator;
    
    // begin 指针
    const_iterator begin() const {
        return list;
    }
    iterator begin() {
        return list;
    }
    
    // 结束位置指针
    const_iterator end() const {
        return list + count;
    }
    iterator end() {
        return list + count;
    }
};
```

### `property_list_t`
```c++
struct property_list_t : entsize_list_tt<property_t, property_list_t, 0> {
};
```
继承自 `entsize_list_tt`，这里它的 `FlagMask` `hardcode` 是 `0`。

### `propertiesForMeta`
```c++
/***********************************************************************
* category_t::propertiesForMeta

* Return a category's instance or class properties.
* 返回 category 的 实例 或 类 属性。(但是我们好像从来没有在在类中见过属性呀...)

* hi is the image containing the category.
**********************************************************************/
property_list_t *
category_t::propertiesForMeta(bool isMeta, struct header_info *hi)
{
    if (!isMeta) return instanceProperties;
    else if (hi->info()->hasCategoryClassProperties()) return _classProperties;
    else return nil;
}
```
`header_info` 涉及到 `runtime` 初始化加载数据，这里暂且不表。 

到这里 `category_t` 相关的数据结构基本看完了，并不复杂。在之前我们用 `clang` 编译我们的类文件和分类文件的时候，已经看到生成的 `_category_t` 结构体，下面我们再解读一下 `clang` 以后的 `.cpp` 文件内容：

### `_OBJC_$_CATEGORY_INSTANCE_METHODS_NSObject_$_customCategory`
编译器生成实例方法列表保存在 **DATA段的** `objc_const` `section` 里（`struct /*_method_list_t*/`）。 
```c++
static struct /*_method_list_t*/ {
    unsigned int entsize;  // sizeof(struct _objc_method)
    unsigned int method_count;
    struct _objc_method method_list[2];
} _OBJC_$_CATEGORY_INSTANCE_METHODS_NSObject_$_customCategory __attribute__ ((used, section ("__DATA,__objc_const"))) = {
    sizeof(_objc_method),
    2,
    {{(struct objc_selector *)"customInstanceMethod_one", "v16@0:8", (void *)_I_NSObject_customCategory_customInstanceMethod_one},
    {(struct objc_selector *)"customInstanceMethod_two", "v16@0:8", (void *)_I_NSObject_customCategory_customInstanceMethod_two}}
};
```

### `_OBJC_$_CATEGORY_CLASS_METHODS_NSObject_$_customCategory`
编译器生成类方法列表保存在 **DATA段的** `objc_const` `section` 里（`struct /*_method_list_t*/`）。
```c++
static struct /*_method_list_t*/ {
    unsigned int entsize;  // sizeof(struct _objc_method)
    unsigned int method_count;
    struct _objc_method method_list[2];
} _OBJC_$_CATEGORY_CLASS_METHODS_NSObject_$_customCategory __attribute__ ((used, section ("__DATA,__objc_const"))) = {
    sizeof(_objc_method),
    2,
    {{(struct objc_selector *)"customClassMethod_one", "v16@0:8", (void *)_C_NSObject_customCategory_customClassMethod_one},
    {(struct objc_selector *)"customClassMethod_two", "v16@0:8", (void *)_C_NSObject_customCategory_customClassMethod_two}}
};
```

### `_OBJC_$_PROP_LIST_NSObject_$_customCategory`
编译器生成属性列表保存在 **DATA段的** `objc_const` `section` 里（`struct /*_prop_list_t*/`）。
```c++
static struct /*_prop_list_t*/ {
    unsigned int entsize;  // sizeof(struct _prop_t)
    unsigned int count_of_properties;
    struct _prop_t prop_list[2];
} _OBJC_$_PROP_LIST_NSObject_$_customCategory __attribute__ ((used, section ("__DATA,__objc_const"))) = {
    sizeof(_prop_t),
    2,
    {{"categoryProperty_one","T@\"NSString\",C,N"},
    {"categoryProperty_two","T@\"NSMutableArray\",&,N"}}
};
```

还有一个需要注意到的事实就是 `category` 的名字用来给各种列表以及后面的 `category` 结构体本身命名，而且有 `static` 来修饰，所以在同一个编译单元里我们的 `category` 名不能重复，否则会出现编译错误。

### `_OBJC_$_CATEGORY_NSObject_$_customCategory`
编译器生成 `_category_t` 本身 `_OBJC_$_CATEGORY_NSObject_$_customCategory` 并用前面生成的实例方法、类方法、属性列表来初始化。
还用 `OBJC_CLASS_$_NSObject` 来动态指定 `_OBJC_$_CATEGORY_NSObject_$_customCategory` 所属的类。
```c++
extern "C" __declspec(dllimport) struct _class_t OBJC_CLASS_$_NSObject;

static struct _category_t _OBJC_$_CATEGORY_NSObject_$_customCategory __attribute__ ((used, section ("__DATA,__objc_const"))) = 
{
    "NSObject",
    0, // &OBJC_CLASS_$_NSObject,
    (const struct _method_list_t *)&_OBJC_$_CATEGORY_INSTANCE_METHODS_NSObject_$_customCategory,
    (const struct _method_list_t *)&_OBJC_$_CATEGORY_CLASS_METHODS_NSObject_$_customCategory,
    0,
    (const struct _prop_list_t *)&_OBJC_$_PROP_LIST_NSObject_$_customCategory,
};

// 设置 cls
static void OBJC_CATEGORY_SETUP_$_NSObject_$_customCategory(void ) {
    _OBJC_$_CATEGORY_NSObject_$_customCategory.cls = &OBJC_CLASS_$_NSObject;
}

#pragma section(".objc_inithooks$B", long, read, write)
__declspec(allocate(".objc_inithooks$B")) static void *OBJC_CATEGORY_SETUP[] = {
    (void *)&OBJC_CATEGORY_SETUP_$_NSObject_$_customCategory,
};
```
### `L_OBJC_LABEL_CATEGORY_$`
最后，编译器在 **DATA段下的** `objc_catlist` `section` 里保存了一个长度为 1 的 `struct _category_t *` 数组 `L_OBJC_LABEL_CATEGORY_$`，如果有多个 `category`，会生成对应长度的数组，用于运行期 `category` 的加载，到这里编译器的工作就接近尾声了。
```c++
static struct _category_t *L_OBJC_LABEL_CATEGORY_$ [1] __attribute__((used, section ("__DATA, __objc_catlist,regular,no_dead_strip")))= {
    &_OBJC_$_CATEGORY_NSObject_$_customCategory,
};
```
这时我们大概会有一个疑问，这些准备好的的 `_category_t` 数据什么时候附加到类上去呢？或者是存放在内存哪里等着我们去调用它里面的实例函数或类函数呢？**已知分类数据是会全部追加到类本身上去的。** 不是类似 `weak` 机制或者 `associated object` 机制等，再另外准备哈希表存放数据，然后根据对象地址去查询处理数据等这样的模式。
下面我们就开始研究分类的数据是如何追加到本类上去的。

## `category` 相关函数
&emsp;`category` 的加载涉及到 `runtime` 的初始化及加载流程，因为 `runtime` 相关的内容比较多，这里只一笔带过，详细内容准备开新篇来讲。本篇只研究`runtime` 初始化加载过程中连带到 `category` 的加载。
`Objective-C` 的运行是依赖 `runtime` 来做的，而 `runtime` 和其他系统库一样，是由 `OS X` 和 `iOS` 通过 `dyld(the dynamic link editor)` 来动态加载的。

### `_objc_init`
在 `Source/objc-os.mm` P907 可看到其入口方法 `_objc_init`：
```c++
/***********************************************************************
* _objc_init
* Bootstrap initialization. 引导程序初始化。

* Registers our image notifier with dyld.
* 通过 dyld 来注册我们的 image.

* Called by libSystem BEFORE library initialization time
* library 初始化之前由 libSystem 调用
**********************************************************************/

void _objc_init(void)
{
    // 用一个静态变量标记，保证只进行一次初始化
    static bool initialized = false;
    if (initialized) return;
    initialized = true;
    
    // fixme defer initialization until an objc-using image is found?
    // fixme 推迟初始化，直到找到一个 objc-using image？
    
    // 读取会影响 runtime 的环境变量，
    // 如果需要，还可以打印一些环境变量。
    environ_init();
    
    tls_init();
    
    // 运行 C++ 静态构造函数，
    // 在 dyld 调用我们的静态构造函数之前，libc 调用 _objc_init（），因此我们必须自己做。
    static_init();
    
    runtime_init();
    
    // 初始化 libobjc 的异常处理系统，
    // 由 map_images（）调用。
    exception_init();
    
    
    cache_init();
    _imp_implementationWithBlock_init();

    _dyld_objc_notify_register(&map_images, load_images, unmap_image);

#if __OBJC2__
    // 标记 _dyld_objc_notify_register 的调用是否已完成。
    didCallDyldNotifyRegister = true;
#endif
}
```
***看到 `_dyld_objc_notify_register` 函数的第一个参数是 `map_imags` 的函数地址。`_objc_init` 里面调用 `map_images` 最终会调用 `objc-runtime-new.mm` 里面的 `_read_images` 函数，而 `category` 加载到类上面正是从 `_read_images` 函数里面开始的。***
可能这里已经发生修改，在 `load_images` 函数里面调用 `loadAllCategories()` 函数，且它的前面有一句 `didInitialAttachCategories = true;` 这个全局静态变量默认为 `false`，在这里被设置为 `true`，且整个 `objc4` 唯一的一次赋值操作，那么可以断定: 在 `load_images` 函数里面调用 `loadAllCategories()` 一定是早于 `_read_images` 里面的 `for` 循环里面调用 `load_categories_nolock` 函数。

### `map_images`
```c++
/***********************************************************************
* map_images
* Process the given images which are being mapped in by dyld.
* 处理由 dyld 映射的给定 images。

* Calls ABI-agnostic code after taking ABI-specific locks.
* 取得 ABI-specific 锁后调用 ABI-agnostic.

* Locking: write-locks runtimeLock
* rutimeLock 是一个全局的互斥锁（mutex_t runtimeLock;）
**********************************************************************/
void
map_images(unsigned count, const char * const paths[],
           const struct mach_header * const mhdrs[])
{
    // 加锁
    mutex_locker_t lock(runtimeLock);
    // 调用 map_images_nolock 函数
    return map_images_nolock(count, paths, mhdrs);
}
```
### `map_images_nolock`
```c++
void 
map_images_nolock(unsigned mhCount, const char * const mhPaths[],
                  const struct mach_header * const mhdrs[])
{
...
if (hCount > 0) {
    _read_images(hList, hCount, totalClasses, unoptimizedTotalClasses);
}
...
}
```
### `_read_images`
```c++
/***********************************************************************
* _read_images
* Perform initial processing of the headers in the linked
* list beginning with headerList. 
* 从 headerList 开始对链接列表中的标头执行初始处理。
*
* Called by: map_images_nolock
* 由 map_images_nolock 调用
*
* Locking: runtimeLock acquired by map_images
* 由 map_images 函数获取 runtimeLock 
**********************************************************************/
void _read_images(header_info **hList, uint32_t hCount, int totalClasses, int unoptimizedTotalClasses)
{
...
// Discover categories. Only do this after the initial category
// attachment has been done.
// 发现 categories。仅在完成初始类别附件（category_t 结构体列表，包含该类所有的类别）
// 后才执行此操作。
//（大概是指编译器生成并保存在 DATA段下的 `objc_catlist` `section` 的 `struct _category_t *` 数组吗？）

// For categories present at startup,
// discovery is deferred until the first load_images call after the
// call to _dyld_objc_notify_register completes. rdar://problem/53119145
// 对于启动时出现的类别，
// discovery 被推迟，直到 _dyld_objc_notify_register 的调用完成后第一次调用 load_images。

if (didInitialAttachCategories) {
    for (EACH_HEADER) {
        load_categories_nolock(hi);
    }
}
...
}

/***********************************************************************
* didInitialAttachCategories
* Whether the initial attachment of categories present at startup has been done.
* 启动时出现的类别的初始附件是否已完成，
**********************************************************************/
static bool didInitialAttachCategories = false;
```
### `EACH_HEADER`
```c++
// header_info **hList 
// hList 是一个元素是 header_info * 的数组

#define EACH_HEADER \
hIndex = 0;         \
hIndex < hCount && (hi = hList[hIndex]); \
hIndex++
```

### `load_images`
```c++
void
load_images(const char *path __unused, const struct mach_header *mh)
{
    if (!didInitialAttachCategories && didCallDyldNotifyRegister) {
        // 全局的唯一一次把 didInitialAttachCategories 置为 true
        didInitialAttachCategories = true;
        loadAllCategories();
    }
    ...
}
```

### `loadAllCategories`
循环调用 `load_categories_nolock` 函数，由于目前对 `runtime` 初始化加载流程不熟悉，暂时无法定论加载 `category` 是从哪开始的，但是目前可以确定的是加载 `category` 是调用 `load_categories_nolock` 函数来做的，下面我们就详细分析 `load_categories_nolock` 函数。
```c++
static void loadAllCategories() {
    mutex_locker_t lock(runtimeLock);

    for (auto *hi = FirstHeader; hi != NULL; hi = hi->getNext()) {
        load_categories_nolock(hi);
    }
}
```

### `load_categories_nolock` 
在 `for` 循环里面执行 `load_categories_nolock` 函数，这里 `header_info` 涉及到 `Apple` 的二进制格式和 `load` 机制，这里暂且不表，等到 `runtime` 初始化加载相关篇章时再讲。
这里不影响我们解读 `load_categories_nolock` 函数。

**这里会涉及懒加载的类和非懒加载的类的，此处先不表，不影响我们阅读原始代码，我们先硬着头把函数实现一行一行读完。**

```c++
static void load_categories_nolock(header_info *hi) {
    // 是否有类属性？（目前我们还没有见过给类添加属性的操作）
    bool hasClassProperties = hi->info()->hasCategoryClassProperties();

    size_t count;
    auto processCatlist = [&](category_t * const *catlist) {
        // catlist 是保存一个 category_t * 的指针，
        // 且有一个 const 修饰，表示该指针的指向是固定的，但是指向的内存里面的内容是可以修改的
        
        // 这个数据大概是指编译器生成并保存在 DATA段下的
        // `objc_catlist` `section` 的 `struct _category_t *` 数组吗？
        
        // 遍历数组
        for (unsigned i = 0; i < count; i++) {
            // 取得 category_t 指针
            category_t *cat = catlist[i];
            // 取得 category_t 所属的类
            Class cls = remapClass(cat->cls);
            
            // struct locstamped_category_t {
            //    category_t *cat;
            //    struct header_info *hi;
            // };
            // 构建一个 locstamped_category_t 的局部变量
            locstamped_category_t lc{cat, hi};

            if (!cls) {
                // 如类不存在，执行 log
                // Category's target class is missing (probably weak-linked).
                // Ignore the category.
                if (PrintConnecting) {
                    _objc_inform("CLASS: IGNORING category \?\?\?(%s) %p with "
                                 "missing weak-linked target class",
                                 cat->name, cat);
                }
                continue;
            }
            
            // Process this category.
            // 处理此 category。
            // 判断 cls 是否是 Stub Class
            if (cls->isStubClass()) {
                // Stub classes are never realized. 
                // Stub classes don't know their metaclass until they're initialized,
                // so we have to add categories with class methods or properties to the stub itself.
                // methodizeClass() will find them and add them to the metaclass as appropriate.
                
                // 大概意思是说 Stub classes 开始时不确定是元类还是类吗？
                if (cat->instanceMethods ||
                    cat->protocols ||
                    cat->instanceProperties ||
                    cat->classMethods ||
                    cat->protocols ||
                    (hasClassProperties && cat->_classProperties))
                {
                    // 该类还没有实现，把 lc 添加到类的原始数据上吗？（这里还不知道怎么形容没有实现的类）
                    objc::unattachedCategories.addForClass(lc, cls);
                }
            } else {
                // First, register the category with its target class.
                // Then, rebuild the class's method lists (etc) if the class is realized.
                // 首先，将 category 注册给其目标类。然后，如果该类已实现了，则重建该类的方法列表（等）。
                
                // 把实例方法、协议、属性添加到类
                if (cat->instanceMethods ||  cat->protocols
                    ||  cat->instanceProperties)
                {
                    if (cls->isRealized()) {
                        // 该类已实现，则重建类的方法列表等
                        attachCategories(cls, &lc, 1, ATTACH_EXISTING);
                    } else {
                        // 该类还没有实现，把 lc 添加到类的原始数据上吗？（这里不知道怎么形容没有实现的类）
                        objc::unattachedCategories.addForClass(lc, cls);
                    }
                }

                // 看到 cat->protocols 也会被添加到元类中
                // 把类方法、协议添加到元类
                if (cat->classMethods  ||  cat->protocols
                    ||  (hasClassProperties && cat->_classProperties))
                {
                    if (cls->ISA()->isRealized()) {
                        attachCategories(cls->ISA(), &lc, 1, ATTACH_EXISTING | ATTACH_METACLASS);
                    } else {
                        objc::unattachedCategories.addForClass(lc, cls->ISA());
                    }
                }
            }
        }
    };
    
    // _getObjc2CategoryList 取得原始 category 数据
    processCatlist(_getObjc2CategoryList(hi, &count));
    // _getObjc2CategoryList2 取得原始 category 数据
    processCatlist(_getObjc2CategoryList2(hi, &count));
}
```
看到 `category` 中的协议会同时添加到类和元类。

### `unattachedCategories`
```c++
static UnattachedCategories unattachedCategories;
```

### `addForClass`
`class UnattachedCategories` 继承自 `ExplicitInitDenseMap`，模版抽象类型是: `<Class, category_list>`
```c++
class UnattachedCategories : public ExplicitInitDenseMap<Class, category_list>
{
public:
    void addForClass(locstamped_category_t lc, Class cls)
    {
        // 加锁
        runtimeLock.assertLocked();
        
        // log 语句
        if (slowpath(PrintConnecting)) {
            _objc_inform("CLASS: found category %c%s(%s)",
                         cls->isMetaClass() ? '+' : '-',
                         cls->nameForLogging(), lc.cat->name);
        }

        // 这里有又看到了 try_emplace 函数，在关联对象一篇中我们有用到。
        // 关联对象用到的是在全局 AssociationsHashMap 中尝试插入
        // <DisguisedPtr<objc_object>, ObjectAssociationMap> 
        // 返回值类型是 std::pair<iterator, bool>
        
        // 尝试插入 <Class, category_list>
        auto result = get().try_emplace(cls, lc);
        
        // 插入失败时，会执行 append 函数，这里是保证 category 的数据内容
        // 必须拼到所属类中去吗 ？
        
        if (!result.second) {
            result.first->second.append(lc);
        }
    }
    ...
}
```

### `attachCategories`
最最最重要的一个函数。
```c++
// Attach method lists and properties and protocols from categories to a class.
// 将 方法列表 以及 属性 和 协议 从 categories 附加到类。

// Assumes the categories in cats are all loaded and sorted by load order, oldest categories first.
// 假定 cats 中的所有类别均按加载顺序进行加载和排序，最早的类别在前。
// oldest categories first 是指后编译的分类在前面吗 ？
static void
attachCategories(Class cls, const locstamped_category_t *cats_list, uint32_t cats_count,
                 int flags)
{
    // log
    if (slowpath(PrintReplacedMethods)) {
        printReplacements(cls, cats_list, cats_count);
    }
    // log
    if (slowpath(PrintConnecting)) {
        _objc_inform("CLASS: attaching %d categories to%s class '%s'%s",
                     cats_count, (flags & ATTACH_EXISTING) ? " existing" : "",
                     cls->nameForLogging(), (flags & ATTACH_METACLASS) ? " (meta)" : "");
    }

    /*
     * Only a few classes have more than 64 categories during launch.
     * 在启动期间，只有少数几个类具有超过 64 个 categories。
     * This uses a little stack, and avoids malloc.
     * 这将使用一个较小的栈，并避免使用 malloc。
     *
     * Categories must be added in the proper order, which is back to front.
     * 必须按正确的顺序添加类别，这是从前到后的。
     *
     * To do that with the chunking, we iterate cats_list from front to back,
     * build up the local buffers backwards, and call attachLists on the chunks. 
     * attachLists prepends the lists, so the final result is in the expected order.
     * 为此，我们从前到后迭代 cats_list，向后建立本地缓冲区，然后在块上调用 attachLists。attachLists 
     * 在列表的前面，因此最终结果按预期顺序排列。
     */
    
    // 在编译时即可得出常量值
    constexpr uint32_t ATTACH_BUFSIZ = 64;
    // 方法列表 数组元素是 method_list_t *
    method_list_t   *mlists[ATTACH_BUFSIZ];
    // 属性列表 数组元素是 property_list_t *
    property_list_t *proplists[ATTACH_BUFSIZ];
    // 协议列表 数组元素是 protocol_list_t *
    protocol_list_t *protolists[ATTACH_BUFSIZ];

    uint32_t mcount = 0;
    uint32_t propcount = 0;
    uint32_t protocount = 0;
    bool fromBundle = NO;
    
    // 根据入参 flags 判断是否是元类
    bool isMeta = (flags & ATTACH_METACLASS);
    // 外部扩展
    auto rwe = cls->data()->extAllocIfNeeded();

    for (uint32_t i = 0; i < cats_count; i++) {
        // locstamped_category_t 
        auto& entry = cats_list[i];
        
        // 根据 isMeta 取出 category_t 中的实例方法列表或者类方法列表
        method_list_t *mlist = entry.cat->methodsForMeta(isMeta);
        if (mlist) {
            // 判断方法个数
            if (mcount == ATTACH_BUFSIZ) {
                // 
                prepareMethodLists(cls, mlists, mcount, NO, fromBundle);
                rwe->methods.attachLists(mlists, mcount);
                mcount = 0;
            }
            mlists[ATTACH_BUFSIZ - ++mcount] = mlist;
            fromBundle |= entry.hi->isBundle();
        }

        property_list_t *proplist =
            entry.cat->propertiesForMeta(isMeta, entry.hi);
        if (proplist) {
            if (propcount == ATTACH_BUFSIZ) {
                rwe->properties.attachLists(proplists, propcount);
                propcount = 0;
            }
            proplists[ATTACH_BUFSIZ - ++propcount] = proplist;
        }

        protocol_list_t *protolist = entry.cat->protocolsForMeta(isMeta);
        if (protolist) {
            if (protocount == ATTACH_BUFSIZ) {
                rwe->protocols.attachLists(protolists, protocount);
                protocount = 0;
            }
            protolists[ATTACH_BUFSIZ - ++protocount] = protolist;
        }
    }

    if (mcount > 0) {
        prepareMethodLists(cls, mlists + ATTACH_BUFSIZ - mcount, mcount, NO, fromBundle);
        rwe->methods.attachLists(mlists + ATTACH_BUFSIZ - mcount, mcount);
        if (flags & ATTACH_EXISTING) flushCaches(cls);
    }

    rwe->properties.attachLists(proplists + ATTACH_BUFSIZ - propcount, propcount);

    rwe->protocols.attachLists(protolists + ATTACH_BUFSIZ - protocount, protocount);
}
```

## 参考链接
**参考链接:🔗**
+ [Objective-C运行时-类别category](https://zhuanlan.zhihu.com/p/161100311)
+ [iOS Extension详解，及与Category的区别](https://www.jianshu.com/p/b45e1dd24e32)
+ [iOS Category详解](https://www.jianshu.com/p/c92b17a36b9e)
+ [iOS-分类（Category）](https://www.jianshu.com/p/01911be8ce83)
+ [iOS Category的使用及原理](https://www.jianshu.com/p/4ce54f78290a)
+ [iOS-Category原理](https://www.jianshu.com/p/9966940fcd9e)
+ [category工作原理](https://www.jianshu.com/p/7de5f06af5c7)
+ [iOS开发笔记之六十七——Category使用过程中的一些注意事项](https://blog.csdn.net/lizitao/article/details/77196620)
+ [结合 category 工作原理分析 OC2.0 中的 runtime](https://blog.csdn.net/qq_26341621/article/details/54140140)
+ [深入理解Objective-C：Category](https://tech.meituan.com/2015/03/03/diveintocategory.html)
+ [iOS 捋一捋Category加载流程及+load](https://www.jianshu.com/p/fd176e806cf3)
+ [十：底层探索 - 分类的加载](https://juejin.im/post/6844904115814793224)
+ [Category的实现原理](https://www.jianshu.com/p/7aaac3e70637)
