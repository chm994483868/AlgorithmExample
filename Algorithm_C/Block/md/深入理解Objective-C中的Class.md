#  深入理解Objective-C中的Class
> **OC -> C/C++ -> 汇编 -> 机器码**

在 `main.m` 中输入如下，然后在终端中使用如下命令，将 `main.m` 中代码编译成 `C/C++` 代码（在当前文件夹下生成 `main.cpp`）：
```
clang -rewrite-objc main.m -o main.cpp // -o main.cpp 可以忽略
```
在 main.cpp 中发现下边的结构体，从 objc4 库的命名习惯可推断应该是 NSObject 的底层实现:
```
// IMPL 应该是 implementation 的缩写
// NSObject_IMPL => NSObject implementation 
struct NSObject_IMPL {
    Class isa;
};
```
在 **usr/include => objc => NSObject.h** 中看到其声明：
```
OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0)
OBJC_ROOT_CLASS
OBJC_EXPORT
@interface NSObject <NSObject> {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-interface-ivars"
    Class isa  OBJC_ISA_AVAILABILITY;
#pragma clang diagnostic pop
}

// 暂时忽略前缀修饰，以及中间的消除警告的代码
// 以及 NSObject 协议里面的代理方法，以及实例方法和类方法

// 主要看 Class isa; 这个成员变量和上面 NSObject_IMPL 
// 结构体的成员变量如出一辙，这里进一步印证了 NSObject_IMPL 
// 结构体是 NSObject 的底层结构的推断。
@interface NSObject <NSObject> {
    Class isa  OBJC_ISA_AVAILABILITY;
}
```
按住 command 点击 Class 跳转到 **usr/include => objc => objc.h** 
中看到 Class：
```
/// An opaque type that represents an Objective-C class.
typedef struct objc_class *Class;
```
Class 是 `objc_class` 结构体指针，即 isa 实际上是一个指向 `objc_class` 的结构体的指针。
`objc_class` 是 Class 的底层结构。

## HHStaff HHManager

```
// HHStaff.h
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface HHStaff : NSObject {
    NSString *name;
}

- (void)doInstanceStaffWork; // 对象方法
+ (void)doClassStaffWork; // 类方法

@end

NS_ASSUME_NONNULL_END

// HHStaff.m
#import "HHStaff.h"

@implementation HHStaff

- (void)doInstanceStaffWork { // 对象方法
    NSLog(@"✳️✳️✳️ %s", __FUNCTION__);
}

+ (void)doClassStaffWork { // 类方法
    NSLog(@"✳️✳️✳️ %s", __FUNCTION__);
}

@end

// HHManager.h
#import "HHStaff.h"

NS_ASSUME_NONNULL_BEGIN

@interface HHManager : HHStaff {
    NSInteger officeNum;
}

- (void)doInstanceManagerWork;
+ (void)doClassManagerWork;

@end

NS_ASSUME_NONNULL_END

// HHManager.m
#import "HHManager.h"

@implementation HHManager

- (void)doInstanceManagerWork {
    NSLog(@"✳️✳️✳️ %s", __FUNCTION__);
}

+ (void)doClassManagerWork {
    NSLog(@"✳️✳️✳️ %s", __FUNCTION__);
}

@end

// clang -rewrite-objc main.m =》main.cpp

struct NSObject_IMPL {
    Class isa;
};

struct HHStaff_IMPL {
    struct NSObject_IMPL NSObject_IVARS;
    NSString *name;
};

struct HHManager_IMPL {
    struct HHStaff_IMPL HHStaff_IVARS;
    NSInteger officeNum;
};

// 合并继承体系, HHManager_IMPL 实际实现如下：
struct HHManager_IMPL {
    Class isa;
    NSString *name;
    NSInteger officeNum;
};
```
**isa 来自 NSObject，因为大部分类都是直接或间接继承自 NSObject 的，所以可以认为每一个对象都包含了一个 isa 指针。**
## OC 中的 3 种对象关系
+ 实例对象(instance)。实例对象在内存中存储的信息包括：isa 指针 和 类定义中的成员变量对应的值，如 `NSString* name` 成员变量的值是: `@"CHM"`。 
+ 类对象(Class)。类对象中包含的信息，成员变量信息，指的是成员变量的描述信息。如 `HHManager_IMPL` 中有：`Class isa`、`NSString* name`、`NSInteger officeNum` 三个成员变量。
+ 元类对象(meta-class)。元类对象的存储结构与类对象相似，只不过只有 `isa`、`superclass` 和 类方法有值，其他均为空。
```
HHStaff *staffA = [[HHStaff alloc] init];
HHStaff *staffB = [[HHStaff alloc] init];

NSLog(@"♻️♻️♻️ 实例对象: %p - %p", staffA, staffB);

// @protocol NSObject
// - (Class)class OBJC_SWIFT_UNAVAILABLE("use 'type(of: anObject)' instead");
// @end
Class staffClassA = [staffA class]; // 属于 NSObject 协议的代理方法

// runtime.h 中的函数
Class staffClassB = objc_getClass(object_getClassName(staffB));
Class staffClassB2 = object_getClass(staffB);

@interface NSObject <NSObject>
+ (Class)class OBJC_SWIFT_UNAVAILABLE("use 'aClass.self' instead");
@end
Class staffClassC = [HHStaff class]; // 属于 NSObject 类定义中的类方法

NSLog(@"♻️♻️♻️ 类 对象: %p - %p - %p - %p", staffClassA, staffClassB, staffClassB2, staffClassC);

Class staffMetaClassA = object_getClass(staffClassA);
Class staffMetaClassB = object_getClass(staffClassB);

NSLog(@"♻️♻️♻️ 元类对象: %p - %p", staffMetaClassA, staffMetaClassB);

// 打印结果:
♻️♻️♻️ 实例对象: 0x1039addc0 - 0x1039ace80
♻️♻️♻️ 类 对象: 0x100003890 - 0x100003890 - 0x100003890 - 0x100003890
♻️♻️♻️ 元类对象: 0x100003868 - 0x100003868
```
```
/** 
 * Returns the class of an object.
 * 返回对象的类。
 * 
 * @param obj The object you want to inspect.
 * @param obj 你想要获取其 Class 的实例对象。
 *（包括实例对象和类对象，入参是类对象时返回的是它的元类）
 * 
 * @return The class object of which object is an instance, or Nil if object is nil.
 * 入参为 Nil 时返回 nil
 */
OBJC_EXPORT Class _Nullable
object_getClass(id _Nullable obj) 
    OBJC_AVAILABLE(10.5, 2.0, 9.0, 1.0, 2.0);
```
```
/** 
 * Returns the class definition of a specified class.
 * 返回指定类的类定义。
 * 
 * @param name The name of the class to look up.
 * 要查找的类的名称。
 *
 * @return The Class object for the named class, or \c nil
 *  if the class is not registered with the Objective-C runtime.
 * 
 * @note \c objc_getClass is different from \c objc_lookUpClass in that if the class
 *  is not registered, \c objc_getClass calls the class handler callback and then checks
 *  a second time to see whether the class is registered. \c objc_lookUpClass does 
 *  not call the class handler callback.
 * 
 * @warning Earlier implementations of this function (prior to OS X v10.0)
 *  terminate the program if the class does not exist.
 */
OBJC_EXPORT Class _Nullable
objc_getClass(const char * _Nonnull name)
    OBJC_AVAILABLE(10.0, 2.0, 9.0, 1.0, 2.0);
```
由打印结果可知：实例对象可以有多个，类对象和元类对象各自只有一个。

### isa 和 superclass
通过上一小节




**参考链接:**
[深入理解 Objective-C ☞ Class](https://www.jianshu.com/p/241e8be676a9?utm_campaign=maleskine&utm_content=note&utm_medium=reader_share&utm_source=weixin)
