# iOS associated object使用及底层原理

> 话外音 C++ 的析构函数应该理解为内存释放前的清理工作，而不是内存释放，内存释放是使用的 free 函数，还有 OC 的 dealloc 也是，最终真正的释放内存函数是 free，dealloc 也可以理解为是 free 函数调用前做清理工作的。

## 前言
&emsp;使用 `Category` 为已经存在的类添加方法是我们很熟悉的常规操作，但是如果在 `Category` 中为类添加属性 `@property`，则编译器会立即给我们如下警告:
```
Property 'categoryProperty' requires method 'categoryProperty' to be defined - use @dynamic or provide a method implementation in this category.
Property 'categoryProperty' requires method 'setCategoryProperty:' to be defined - use @dynamic or provide a method implementation in this category
```
提示我们需要手动为属性添加 `setter` `gettr` 方法或者使用 `@dynamic` 在运行时实现这些方法。
**即明确的告诉我们在分类中 `@property` 并不会自动生成实例变量以及存取方法。**

不是说好的使用 `@property`，编译器会自动帮我们生成实例变量和对应的 `setter` 和 `getter` 方法吗，此机制只能在类定义中实现，因为在分类中，类的实例变量的布局已经固定，使用 `@property` 已经无法向固定的布局中添加新的实例变量，所以我们需要使用关联对象以及两个方法来模拟构成属性的三个要素。

示例代码:
```objective-c
#import "HMObject.h"

NS_ASSUME_NONNULL_BEGIN

@interface HMObject (category)
// 在分类中添加一个属性
@property (nonatomic, copy) NSString *categoryProperty;
@end

NS_ASSUME_NONNULL_END
```
```objective-c
#import "HMObject+category.h"
#import <objc/runtime.h>

@implementation HMObject (category)

- (NSString *)categoryProperty {
    // _cmd 代指当前方法的选择子，即 @selector(categoryProperty)
    return objc_getAssociatedObject(self, _cmd);
}

- (void)setCategoryProperty:(NSString *)categoryProperty {
    objc_setAssociatedObject(self,
                             @selector(categoryProperty),
                             categoryProperty,
                             OBJC_ASSOCIATION_COPY_NONATOMIC);
}

@end
```
此时我们可以使用关联对象 `Associated Object` 来手动为 `categoryProperty` 添加存取方法，接下来我们对示例代码一步一步进行分析。

## 在类定义中使用 `@property` 
&emsp;在类定义中我们使用 `@property` 为类添加属性，如果不使用 `@dynamic` 标识该属性的话，编译器会自动帮我们生成一个名字为下划线加属性名的实例变量和该属性的 `setter` 和 `getter` 方法。我们编写如下代码:
```objective-c
// .h 中如下书写
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface HMObject : NSObject

@property (nonatomic, copy) NSString *cusProperty;

@end

NS_ASSUME_NONNULL_END

// .m 中什么都不做
#import "HMObject.h"
@implementation HMObject
// @dynamic cusProperty;

@end
```
编译器会自动帮我们做如下三件事:
1. 添加实例变量 `_cusProperty`
2. 添加 `setter` 方法 `setCusProperty`
3. 添加 `getter` 方法 `cusProperty`

即如下 `HMObject.m` 代码实现：
```objective-c
#import "HMObject.h"

@implementation HMObject
//@dynamic cusProperty;
{
    NSString *_cusProperty;
}

- (void)setCusProperty:(NSString *)cusProperty {
    _cusProperty = cusProperty;
}

- (NSString *)cusProperty {
    return _cusProperty;
}

@end
```
### 验证 `@property`
下面我们通过 `LLDB` 进行验证，首先我们把 `HMObject.m` 的代码都注释掉，只留下 `HMObject.h` 中的 `cusProperty` 属性。
然后在 `main` 函数中编写如下代码：
```c++
Class cls = NSClassFromString(@"HMObject");
NSLog(@"%@", cls); // ⬅️ 这里打一个断点
```
开始验证：

> 这里我们也可以使用 `runtime` 的 `class_copyPropertyList`、`class_copyMethodList`、`class_copyIvarList` 三个函数来分别获取 `HMObject` 的属性列表、方法列表和成员变量列表来验证编译器为我们自动生成了什么内容，但是这里我们采用一种更为简单的方法，仅通过控制台打印即可验证。

1. 找到 `cls` 的 `bits`：
```c++
(lldb) x/5gx cls
0x1000022e8: 0x00000001000022c0 (isa) 0x00000001003ee140 (superclass)
0x1000022f8: 0x00000001003e84a0 0x0000001c00000000 (cache_t)
0x100002308: 0x0000000101850640 (bits)
```
2. 强制转换 `class_data_bits_t` 指针
```c++
(lldb) p (class_data_bits_t *)0x100002308
(class_data_bits_t *) $1 = 0x0000000100002308
```
3. 取得 `class_rw_t *`
```c++
(lldb) p $1->data()
(class_rw_t *) $2 = 0x0000000101850640
```
4. 取得 `class_ro_t *`
```c++
(lldb) p $2->ro
(const class_ro_t *) $3 = 0x0000000100002128
```
5. 打印 `ro` 内容
```c++
(lldb) p *$3
(const class_ro_t) $4 = {
  flags = 388
  instanceStart = 8
  instanceSize = 16
  reserved = 0
  ivarLayout = 0x0000000100000ee6 "\x01"
  name = 0x0000000100000edd "HMObject" // 类名
  baseMethodList = 0x0000000100002170 // 方法列表
  baseProtocols = 0x0000000000000000 // 遵循协议为空
  ivars = 0x00000001000021c0 // 成员变量
  weakIvarLayout = 0x0000000000000000
  baseProperties = 0x00000001000021e8 // 属性
  _swiftMetadataInitializer_NEVER_USE = {}
}
```
6. 打印 `ivars`
```c++
(lldb) p $4.ivars
(const ivar_list_t *const) $5 = 0x00000001000021c0
(lldb) p *$5
(const ivar_list_t) $6 = {
  entsize_list_tt<ivar_t, ivar_list_t, 0> = {
    entsizeAndFlags = 32
    count = 1 // 有 1 个成员变量
    first = {
      offset = 0x00000001000022b8
      // 看到名字为 _cusProperty 的成员变量
      name = 0x0000000100000ef6 "_cusProperty"
      type = 0x0000000100000f65 "@\"NSString\""
      alignment_raw = 3
      size = 8
    }
  }
}
```
7. 打印 `baseProperties`
```c++
(lldb) p $4.baseProperties
(property_list_t *const) $7 = 0x00000001000021e8
(lldb) p *$7
(property_list_t) $8 = {
  entsize_list_tt<property_t, property_list_t, 0> = {
    entsizeAndFlags = 16
    count = 1
    first = (name = "cusProperty", attributes = "T@\"NSString\",C,N,V_cusProperty")
  }
}
```
看到只有一个名字是 `cusProperty` 的属性，属性的 `attributes` 是：`"T@\"NSString\",C,N,V_cusProperty"`

|code|meaning|
|...|...|
|T|类型|
|C|copy|
|N|nonatomic|
|V|实例变量|

关于它的详细信息可参考 [《Objective-C Runtime Programming Guide》](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ObjCRuntimeGuide/Articles/ocrtPropertyIntrospection.html)。

8. 打印 `baseMethodList`
```c++
(lldb) p $4.baseMethodList
(method_list_t *const) $9 = 0x0000000100002170
(lldb) p *$9
(method_list_t) $10 = {
  entsize_list_tt<method_t, method_list_t, 3> = {
    entsizeAndFlags = 26
    count = 3 // 有 3 个 method
    first = {
      // 第一个正是 cusProperty 的 getter 函数
      name = "cusProperty"
      types = 0x0000000100000f79 "@16@0:8"
      imp = 0x0000000100000c30 (KCObjcTest`-[HMObject cusProperty])
    }
  }
}
```
看到方法的 `TypeEncoding` 如下:
`types = 0x0000000100000f79 "@16@0:8"`
从左向右分别表示的含义是: `@` 表示返回类型是 `OC` 对象，16 表示所有参数总长度，再往后 `@` 表示第一个参数的类型，对应函数调用的 `self` 类型，0 表示从第 0 位开始，分隔号 : 表示第二个参数类型，对应 `SEL`，8 表示从第 8 位开始，因为前面的一个参数 `self` 占 8 个字节。下面开始是自定义参数，因为 `getter` 函数没有自定义函数，所以只有 `self` 和 `SEL` 参数就结束了。
对应的函数原型正是 `objc_msgSend` 函数:
```c++
void
objc_msgSend(void /* id self, SEL op, ... */ )
```
9. 打印剩下的两个 `method`
```c++
(lldb) p $10.get(1)
(method_t) $11 = {
  name = "setCusProperty:"
  types = 0x0000000100000f81 "v24@0:8@16"
  imp = 0x0000000100000c60 (KCObjcTest`-[HMObject setCusProperty:])
}
(lldb) p $10.get(2)
(method_t) $12 = {
  name = ".cxx_destruct"
  types = 0x0000000100000f71 "v16@0:8"
  imp = 0x0000000100000c00 (KCObjcTest`-[HMObject .cxx_destruct])
}
```
看到一个是 `cusProperty` 的 `setter` 函数，一个是 `C++` 的析构函数。

为了做出对比，我们注释掉  `HMObject.h` 中的 `cusProperty` 属性，然后重走上面的流程，可打印出如下信息:
```c++
(lldb) x/5gx cls
0x100002240: 0x0000000100002218 0x00000001003ee140
0x100002250: 0x00000001003e84a0 0x0000001000000000
0x100002260: 0x00000001006696c0
(lldb) p (class_data_bits_t *)0x100002260
(class_data_bits_t *) $1 = 0x0000000100002260
(lldb) p $1->data()
(class_rw_t *) $2 = 0x00000001006696c0
(lldb) p $2->ro
(const class_ro_t *) $3 = 0x0000000100002118
(lldb) p *$3
(const class_ro_t) $4 = {
  flags = 128
  instanceStart = 8
  instanceSize = 8
  reserved = 0
  ivarLayout = 0x0000000000000000
  name = 0x0000000100000f22 "HMObject"
  baseMethodList = 0x0000000000000000
  baseProtocols = 0x0000000000000000
  ivars = 0x0000000000000000
  weakIvarLayout = 0x0000000000000000
  baseProperties = 0x0000000000000000
  _swiftMetadataInitializer_NEVER_USE = {}
}
(lldb) 
```
可看到 `ivars`、`baseProperties` 和 `baseMethodList` 都是 `0x0000000000000000`，即编译器没有为 `HMObject` 生成属性、成员变量和函数。
至此 `@property` 的作用可得到完整证明。

`@property` 能够为我们自动生成实例变量以及存取方法，而这三者构成了属性这个类似于语法糖的概念，为我们提供了更便利的点语法来访问属性：

`self.property` 等价于 `[self property];`
`self.property = value;` 等价于 `[self setProperty:value];`

习惯于 `C/C++` 结构体和结构体指针取结构体成员变量时使用 `.` 和 `->`。初见 `OC` 的点语法时有一丝疑问，`self` 明明是一个指针，访问它的成员变量时为什么用 `.` 呢？如果按 `C/C++` 的规则，不是应该使用 `self->_property` 吗？

这里我们应与 `C/C++` 的点语法做出区别理解，`OC` 中点语法是用来帮助我们便捷访问属性的，在类内部我们可以使用 `_proerty`、`self->_propery` 和 `self.property` 三种方式访问同一个成员变量，区别在于使用 `self.property` 时是通过调用 `property` 的 `setter` 和 `getter` 来读取成员变量，而前两种则是直接读取，因此当我们重写属性的 `setter` 和 `getter` 并在内部做一些自定义操作时，我们一定要记得使用 `self.property` 来访问属性。

##  `Associated Object` 过程
&emsp;我们使用 `objc_setAssociatedObject` 和 `objc_getAssociatedObject` 来分别模拟属性的存取方法，而使用关联对象模拟实例变量。

### `const void *key`
&emsp;存取函数中的参数 `key` 我们都使用了 `@selector(categoryProperty)`，其实也可以使用静态指针 `static void *` 类型的参数来代替，不过这里强烈建议使用 `@selector(categoryProperty)` 作为 `key` 传入，因为这种方法省略了声明参数的代码，并且能很好地保证 `key` 的唯一性。
 
### `objc_AssociationPolicy policy`
`policy` 代表关联策略:
```c++
/**
 * Policies related to associative references.
 * These are options to objc_setAssociatedObject()
 */
typedef OBJC_ENUM(uintptr_t, objc_AssociationPolicy) {
    /**< Specifies a weak reference to the associated object. */
    OBJC_ASSOCIATION_ASSIGN = 0,    
    
    /**< Specifies a strong reference to the associated object. 
    *   The association is not made atomically. */
    OBJC_ASSOCIATION_RETAIN_NONATOMIC = 1, 
    
    /**< Specifies that the associated object is copied. 
    *   The association is not made atomically. */
    OBJC_ASSOCIATION_COPY_NONATOMIC = 3,
    
    /**< Specifies a strong reference to the associated object.
    *   The association is made atomically. */
    OBJC_ASSOCIATION_RETAIN = 01401,
    
    /**< Specifies that the associated object is copied.
    *   The association is made atomically. */
    OBJC_ASSOCIATION_COPY = 01403          
};
```
注释已经解释的很清楚了，即不同的策略对应不同的修饰符:
| objc_AssociationPolicy | 修饰符 |
| ... | ... |
| OBJC_ASSOCIATION_ASSIGN | assign |
| OBJC_ASSOCIATION_RETAIN_NONATOMIC | nonatomic、strong |
| OBJC_ASSOCIATION_COPY_NONATOMIC | nonatomic、copy |
| OBJC_ASSOCIATION_RETAIN | atomic, strong |
| OBJC_ASSOCIATION_COPY | atomic, copy |

### ``

## 参考链接
**参考链接:🔗**
+ [关联对象 AssociatedObject 完全解析](https://draveness.me/ao/)
+ [iOS_@property 属性的本质是什么?](https://www.jianshu.com/p/7ddefcfba3cb)
+ [C++11的6种内存序总结__std::memory_order_acquire_等](https://blog.csdn.net/mw_nice/article/details/84861651)
