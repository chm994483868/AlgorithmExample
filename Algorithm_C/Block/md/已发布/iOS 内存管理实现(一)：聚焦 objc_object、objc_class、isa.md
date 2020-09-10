# iOS 内存管理实现(一)：聚焦 objc_object、objc_class、isa
> 2006 年，苹果发布了全新的 `Objective-C 2.0`，目前我们可以在苹果官网下载最新的 `objc4-781` 源码来阅读和调试。**在 `Obejective-C 2.0` 中类和对象等的实现进行了完全重写。** 虽然 `Objective-C 1.0` 已经被废弃了，但是相关代码并没有被完全剔除，源码内通过一些宏定义来为我们做出区分，在阅读过程中我们可以进行比较理解。

&emsp;`struct objc_object` 和 `struct objc_class` 是 `iOS(OC)` 编写面向对象代码的基石。那今天我们就来超详细的解析这两个结构体吧。

## `objc_object`
&emsp;`struct objc_object` 的定义位于 `objc4-781/Project Headers/objc-private.h/Line 82`。
在正式开始看 `objc_object` 之前我们先看下 `objc-private.h` 文件前面大概 50 行左右相关的代码。

+ **声明 `objc-privete.h` 头文件必须在其它头文件之前导入。**

&emsp;这样做是为了避免与其它地方定义的 `id` 和 `Class` 产生冲突。在 `Objective-C 1.0` 和 `2.0` 中，类和对象的结构体定义是不同的，在源码中我们能看到两处不同的 `objc_class` 和 `objc_object` 定义。

&emsp;我们可在 `runtime.h` 文件中看到 `/* Types */` 处的 `objc_class` 定义的那一部分代码都被 `#if !OBJC_TYPES_DEFINED ... #endif` 所包裹，然后还有 `objc.h` 文件开头处的 `objc_object` 定义的代码也被 `#if !OBJC_TYPES_DEFINED ... #endif` 所包裹，表示两者只在 `Objective-C 1.0` 中使用。`Objective-C 2.0` 下正在使用的  `objc_object` 和 `objc_class` 定义分别位于 `objc-private.h` 和 `objc-runtime-new.h` 文件下。（2.0 下 `OBJC_TYPES_DEFINED` 宏值为 1）
```c++
/* 
 * Isolate ourselves from the definitions of id and Class in the compiler and public headers.
 * 隔离 id 和 Class 在不同的编译器和头文件中的定义。
 */
#ifdef _OBJC_OBJC_H_
#error include objc-private.h before other headers
#endif

// 作为不同版本下定义类和对象的区分
#define OBJC_TYPES_DEFINED 1

// "为同名函数定不同的参数类型，大概是 id 类型参数与 void 替换"
#undef OBJC_OLD_DISPATCH_PROTOTYPES
#define OBJC_OLD_DISPATCH_PROTOTYPES 0
```
&emsp;在 `Public Headers/objc-api.h` 文件可看到如下代码：
```c++
/* OBJC_OLD_DISPATCH_PROTOTYPES == 0 
 * enforces the rule that the dispatch functions must be cast to an appropriate function pointer type. 
 * 必须将 dispatch functions 强制转换为适当的函数指针类型。
 */
#if !defined(OBJC_OLD_DISPATCH_PROTOTYPES)
#   if __swift__
        // Existing Swift code expects IMP to be Comparable.
        // 当前 Swift 代码期望 IMP 是遵守 Comparable，可以进行比较。
        
        // Variadic IMP is comparable via OpaquePointer; non-variadic IMP isn't.
        // Variadic IMP 变量可以通过 OpaquePointer 进行比较; non-variadic IMP 不可以。
#       define OBJC_OLD_DISPATCH_PROTOTYPES 1
#   else
#       define OBJC_OLD_DISPATCH_PROTOTYPES 0
#   endif
#endif
```
在 `objc-privete.h` 中 `OBJC_OLD_DISPATCH_PROTOTYPES` 被定为 0。
示例代码一：
```c++
/// A pointer to the function of a method implementation. 
#if !OBJC_OLD_DISPATCH_PROTOTYPES
typedef void (*IMP)(void /* id, SEL, ... */ ); 
#else
typedef id _Nullable (*IMP)(id _Nonnull, SEL _Nonnull, ...); 
#endif
```
示例代码 二：
```c++
#if !OBJC_OLD_DISPATCH_PROTOTYPES
extern void _objc_msgForward_impcache(void);
#else
extern id _objc_msgForward_impcache(id, SEL, ...);
#endif
```
看到同名函数，返回值 `void` 和 `id _Nullable (id 可空)` 做了替换，参数 `void` 和 `id, SEL` 做了替换，全局搜索可发现此行为只针对调度函数。
+ `Public Header/runtime.h` 中的 `Objective-C 1.0` 下的 `objc_class` 定义
```c++
/* Types */

#if !OBJC_TYPES_DEFINED

/// An opaque type that represents a method in a class definition.
typedef struct objc_method *Method;

/// An opaque type that represents an instance variable.
typedef struct objc_ivar *Ivar;

/// An opaque type that represents a category.
typedef struct objc_category *Category;

/// An opaque type that represents an Objective-C declared property.
typedef struct objc_property *objc_property_t;

struct objc_class {
    // 指向该类的元类（metaclass）指针，（元类不可能为空，根元类的 isa 指向自己）
    Class _Nonnull isa  OBJC_ISA_AVAILABILITY;

#if !__OBJC2__
    // 指向父类的指针，（可空，根类的 super_class 指向 nil，根元类的 super_class 指向根类）
    Class _Nullable super_class                              OBJC2_UNAVAILABLE;
    // 类名
    const char * _Nonnull name                               OBJC2_UNAVAILABLE;
    // 类的版本信息
    long version                                             OBJC2_UNAVAILABLE;
    // 类信息，供运行时使用的一些标记位
    long info                                                OBJC2_UNAVAILABLE;
    // 该类的实例对象的大小
    long instance_size                                       OBJC2_UNAVAILABLE;
    // 指向该类成员变量列表的指针
    struct objc_ivar_list * _Nullable ivars                  OBJC2_UNAVAILABLE;
    // 指向该类方法（函数指针）列表指针的指针
    struct objc_method_list * _Nullable * _Nullable methodLists                    OBJC2_UNAVAILABLE;
    // 指向方法调用缓存的指针
    struct objc_cache * _Nonnull cache                       OBJC2_UNAVAILABLE;
    // 指向该类实现的协议列表的指针
    struct objc_protocol_list * _Nullable protocols          OBJC2_UNAVAILABLE;
#endif

} OBJC2_UNAVAILABLE;
/* Use `Class` instead of `struct objc_class *` */
```
+ `Public Headers/objc.h` 中的 `Objective-C 1.0` 下的 `objc_object` 定义
```c++
#if !OBJC_TYPES_DEFINED
/// An opaque type that represents an Objective-C class.
typedef struct objc_class *Class;

/// Represents an instance of a class.
struct objc_object {
    Class _Nonnull isa  OBJC_ISA_AVAILABILITY;
};

/// A pointer to an instance of a class.
typedef struct objc_object *id;
#endif
```
+ `OBJC_ISA_AVAILABILITY` 
&emsp;在 `Public Headers/objc-api.h` 中的宏定义。
```c++
/* OBJC_ISA_AVAILABILITY: `isa` will be deprecated or unavailable in the future
 * isa 在未来将被弃用或者不可用。
*/
#if !defined(OBJC_ISA_AVAILABILITY)
#   if __OBJC2__
#       define OBJC_ISA_AVAILABILITY  __attribute__((deprecated))
#   else
#       define OBJC_ISA_AVAILABILITY  /* still available */
#   endif
#endif
```
表明在 `Objective-C 1.0` 中类型为 `Class` 的 `isa` 将在 `2.0` 中被弃用。在 `2.0` 中 `isa` 转变为 `union isa_t isa`，下面会详细分析。
+ `OBJC2_UNAVAILABLE`
&emsp;在 `Public Headers/objc-api.h` 中的宏定义。
```c++
/* OBJC2_UNAVAILABLE: unavailable in objc 2.0, deprecated in Leopard */
#if !defined(OBJC2_UNAVAILABLE)
#   if __OBJC2__
#       define OBJC2_UNAVAILABLE UNAVAILABLE_ATTRIBUTE
#   else
        /* plain C code also falls here, but this is close enough */
#       define OBJC2_UNAVAILABLE                                       \
            __OSX_DEPRECATED(10.5, 10.5, "not available in __OBJC2__") \
            __IOS_DEPRECATED(2.0, 2.0, "not available in __OBJC2__")   \
            __TVOS_UNAVAILABLE __WATCHOS_UNAVAILABLE __BRIDGEOS_UNAVAILABLE
#   endif
#endif
```
表明在 `Objective-C 2.0` 中不可用，在 `macOS 10.5 iOS 2.0` 以及 `TVOS/WATCHOS/BRIDGEOS` 不可用。
+ `SEL`
&emsp;在 `Public Headers/objc.h` 文件中定义的一个指向 `struct objc_selector` 的指针。在 `objc4-781` 中查找不到 `objc_selector` 的具体定义，那这个 `objc_selector` 结构体具体是什么取决与使用 `GNU` 还是苹果的运行时， 在 `macOS` 中 `SEL` 其实被映射为一个 `C` 字符串，一个保存方法名字的字符串，它并不指向一个具体的方法实现（`IMP` 类型才是）。
`@selector(abc)` 返回的类型是 `SEL`，它作用是找到名字为 `abc` 的方法，对于所有的类，只要方法名是相同的，产生的 `selector` 都是一样的。简而言之，你可以理解 `@selector()` 就是取指定名字的函数在类中的编号，它的行为基本可以等同 `C` 语言的中函数指针，只不过 `C` 语言中，可以把函数名直接赋给一个函数指针，而 `Objective-C` 的类不能直接应用函数指针，这样只能做一个 `@selector` 语法来取。
```c++
/// An opaque type that represents a method selector.
typedef struct objc_selector *SEL;
```
+ `IMP`
&emsp;在 `Public Headers/objc.h` 文件中定义的一个函数指针，指向方法调用时对应的函数实现。
```c++
// A pointer to the function of a method implementation. 
// 指向方法实现的指针。
#if !OBJC_OLD_DISPATCH_PROTOTYPES
typedef void (*IMP)(void /* id, SEL, ... */ ); 
#else
typedef id _Nullable (*IMP)(id _Nonnull, SEL _Nonnull, ...); 
#endif
```
`OBJC_OLD_DISPATCH_PROTOTYPES` 默认为 0，在 `__swift__` 为真时是 1，则会进行严格的参数匹配。
+ `Method`
&emsp;在 `Objective-C 1.0` 下：
```c++
#if !OBJC_TYPES_DEFINED

// An opaque type that represents a method in a class definition.
// 表示类定义中的方法
typedef struct objc_method *Method;

#endif
```

+ `ASSERT(x)`
&emsp;在 `release` 模式下不会执行断言，但是保证 `ASSERT(x)` 可编译。
```c++
// An assert that's disabled for release builds but still ensures the expression compiles.
#ifdef NDEBUG
#define ASSERT(x) (void)sizeof(!(x))
#else
#define ASSERT(x) assert(x)
#endif
```
+ `__OBJC__`
> `__OBJC__`
  This macro is defined, with value 1, when the Objective-C compiler is in use. You can use `__OBJC__` to test whether a header is compiled by a C compiler or a Objective-C compiler. 

`__OBJC__` 在 `Objective-C` 编译器中被预定义为 1，我们可以使用该宏来判断头文件是通过 `C` 编译器还是 `Objective-C` 编译器进行编译。
+ `__OBJC2__`
&emsp;定义在 `Project Headers/objc-config.h` 中：
```c++
// Define __OBJC2__ for the benefit of our asm files.
#ifndef __OBJC2__
#   if TARGET_OS_OSX  &&  !TARGET_OS_IOSMAC  &&  __i386__
        // old ABI
#   else
#       define __OBJC2__ 1
#   endif
#endif
```
&emsp;下面接着看 `objc_object`。`objc_object` 仅有一个 `isa_t isa` 成员变量。
```c++
struct objc_object {
private:
    isa_t isa;
public:
    ...
};
```

## `isa`
```c++
#include "isa.h"
union isa_t {
    // 构造函数
    isa_t() { }
    // 初始化列表，初始 bits 的值
    isa_t(uintptr_t value) : bits(value) { }

    Class cls;
    // typedef unsigned long uintptr_t;
    uintptr_t bits;
    
    // ISA_BITFIELD 在 x86_64/arm64 下有不同的定义
#if defined(ISA_BITFIELD)
    struct {
        ISA_BITFIELD;  // defined in isa.h
    };
#endif
};
```
&emsp;`isa` 的类型是 `union isa_t`，它有两个成员变量 `Class cls` 和 `uintptr_t bits`，其中 `bits` 采用位域的机制来保存更多信息。`ISA_BITFIELD` 宏定义位于 `isa.h` 文件，一起来看下。

### 苹果设备架构梳理
&emsp;在进行 `isa.h` 之前我们首先对苹果设备的架构做一下梳理。
+ `armv6/armv7/armv7s/arm64(armv8)/arm64e(armv8)` 是 `iPhone` 的 `ARM` 处理器的指令集。（`A` 系列 `CPU` 芯片）
+ `i386/x86_64` 是 `Mac` 的 `intel` 处理器的指令集。（同时也是我们在 `Xcode` 中使用的手机模拟器的指令集）

> `i386` 架构是 `intel` 通用微处理器 `32` 位处理器。
  `x86_64` 架构是 `x86` 架构的 `64` 位处理器。 
  `armv6/armv7/armv7s` 架构是 `32` 位处理器。
  `arm64(armv8)/arm64e(armv8)` 架构是 `64` 位处理器。（即 `iPhone 5s` 开始全部转向 `64` 位（`__LP64__`））

### `isa.h` 文件
&emsp; `isa.h` - `C` 和 `assembly` 代码的 `isa` 字段的定义。

#### `SUPPORT_PACKED_ISA`
```c++
// Define SUPPORT_PACKED_ISA=1 on platforms that store the class in
the isa field as a maskable pointer with other data around it.
// SUPPORT_PACKED_ISA = 1 的平台将类信息与其他数据一起存储在 isa 的字段中，
// 把 isa 作为一个 maskable pointer。
#if (!__LP64__  ||  TARGET_OS_WIN32  || (TARGET_OS_SIMULATOR && !TARGET_OS_IOSMAC))
#   define SUPPORT_PACKED_ISA 0
#else
#   define SUPPORT_PACKED_ISA 1
#endif
```
&emsp;表示平台是否支持在 `isa` 指针中插入 `Class` 之外的信息。如果支持就会将 `Class` 信息放入 `isa_t` 中定义的 `bits` 位域的某些位中，并在其它位中放一些其它信息。如果不支持的话，那么不会使用 `isa_t` 内定义的 `bits`，这时只使用 `cls`（`class` 指针） 成员变量。

下列平台下不支持：
1. 非 64 位处理器，即 32 位处理器。
2. `os` 是 `win32`。
3. 在模拟器中且 `!TARGET_OS_IOSMAC`。（ `TARGET_OS_IOSMAC` 不知道是什么平台）

#### `SUPPORT_INDEXED_ISA`
```c++
// Define SUPPORT_INDEXED_ISA = 1 on platforms that store the class in the isa field as an index into a class table.
// Note, keep this in sync with any .s files which also define it.
// Be sure to edit objc-abi.h as well.
// SUPPORT_INDEXED_ISA = 1 的平台将类信息保存在 isa 中并把 isa 作为一个在 class table 中的索引。
// 注意，与任何定义它的 .s 文件保持同步。确保同时编辑 objc-abi.h 文件。
#if __ARM_ARCH_7K__ >= 2  ||  (__arm64__ && !__LP64__)
#   define SUPPORT_INDEXED_ISA 1
#else
#   define SUPPORT_INDEXED_ISA 0
#endif
```
&emsp;表示在 `isa` 中存放的 `Class` 信息是 `Class` 的地址，并把 `isa` 作为一个在 `class table` 中的索引。仅限于 `armv7k` 或 `arm64_32`。已知自 `iPhone 5s` 以后苹果手机全部转向 `64` 位处理器 `arm64` 架构。

#### `SUPPORT_NONPOINTER_ISA`
```c++
// Define SUPPORT_NONPOINTER_ISA = 1 on any platform that may store something in the isa field that is not a raw pointer.
// SUPPORT_NONPOINTER_ISA = 1 的平台表示可以在 isa 的字段中保存非原始指针的内容。
#if !SUPPORT_INDEXED_ISA  &&  !SUPPORT_PACKED_ISA
#   define SUPPORT_NONPOINTER_ISA 0
#else
#   define SUPPORT_NONPOINTER_ISA 1
#endif
```
&emsp;标记是否支持优化的 `isa` 指针（`isa` 中除 `Class` 指针外，可以保存更多信息）。那如何判断是否支持优化的 `isa` 指针呢？
1. 首先只要支持 `SUPPORT_PACKED_ISA` 或 `SUPPORT_INDEXED_ISA` 任何一个的情况下都是支持 `SUPPORT_NONPOINTER_ISA` 的。 
2. 已知在自 `iPhone 5s` `arm64` 架构以后的 `iPhone` 中 `SUPPORT_PACKED_ISA` 都为 1，`SUPPORT_INDEXED_ISA` 为 0，则其 `SUPPORT_NONPOINTER_ISA` 也为 1。
3. 在 `Edit Scheme... -> Run -> Environment Variables` 中添加 `OBJC_DISABLE_NONPOINTER_ISA`。(在 `objc-env.h` 文件我们可以看到 `OPTION( DisableNonpointerIsa, OBJC_DISABLE_NONPOINTER_ISA, "disable non-pointer isa fields")` )

**注意：**
&emsp;即使在 `64` 位环境下，优化的 `isa` 指针并不是就一定会存储引用计数，毕竟用 19 位（在 `iOS` 系统中）保存引用计数并不一定够，且这 19 位保存的是引用计数减 1。

那么看完以上 3 个宏定义，`isa.h` 文件中的宏定义我们就自然理解分为 3 大块了（由于我们的 `objc4-781` 是在 `Mac` 下运行的，那自然我们会更关注 `x84_64` 一些）。

#### `ISA_BITFIELD`
&emsp;`isa_t isa` 不同位段下保存的信息不同。

##### `__arm64__` 下
```c++
#   define ISA_MASK        0x0000000ffffffff8ULL
#   define ISA_MAGIC_MASK  0x000003f000000001ULL
#   define ISA_MAGIC_VALUE 0x000001a000000001ULL
#   define ISA_BITFIELD                                                      \
      // 表示 isa 中只是存放的 Class cls 指针还是包含更多信息的 bits
      uintptr_t nonpointer        : 1;                                       \
      // 标记该对象是否有关联对象，如果没有的话对象能更快的销毁，
      // 如果有的话销毁前会调用 _object_remove_assocations 函数根据关联策略循环释放每个关联对象
      uintptr_t has_assoc         : 1;                                       \
      // 标记该对象所属类是否有 C++ 析构函数，如果没有的话对象能更快销毁，
      // 如果有的话对象销毁前会调用 object_cxxDestruct 函数去执行该类的析构函数
      uintptr_t has_cxx_dtor      : 1;                                       \
      // isa & ISA_MASK 得出该对象所属的类
      uintptr_t shiftcls          : 33; /*MACH_VM_MAX_ADDRESS 0x1000000000*/ \
      // 用于调试器判断当前对象是真的对象还是没有初始化的空间
      uintptr_t magic             : 6;                                       \
      // 标记该对象是否有弱引用，如果没有的话对象能更快销毁，
      // 如果有的话对象销毁前会调用 weak_clear_no_lock 函数把该对象的弱引用置为 nil，
      // 并调用 weak_entry_remove 把对象的 entry 从 weak_table 移除
      uintptr_t weakly_referenced : 1;                                       \
      // 标记该对象是否正在执行销毁
      uintptr_t deallocating      : 1;                                       \
      // 标记该对象的引用计数是否保存在 refcnts 中
      uintptr_t has_sidetable_rc  : 1;                                       \
      // 保存该对象的引用计数 -1 的值
      uintptr_t extra_rc          : 19
#   define RC_ONE   (1ULL<<45)
#   define RC_HALF  (1ULL<<18)
```

##### `__x86_64__` 下
```c++
#   define ISA_MASK        0x00007ffffffffff8ULL
#   define ISA_MAGIC_MASK  0x001f800000000001ULL
#   define ISA_MAGIC_VALUE 0x001d800000000001ULL
#   define ISA_BITFIELD                                                        \
      uintptr_t nonpointer        : 1;                                         \
      uintptr_t has_assoc         : 1;                                         \
      uintptr_t has_cxx_dtor      : 1;                                         \
      uintptr_t shiftcls          : 44; /*MACH_VM_MAX_ADDRESS 0x7fffffe00000*/ \
      uintptr_t magic             : 6;                                         \
      uintptr_t weakly_referenced : 1;                                         \
      uintptr_t deallocating      : 1;                                         \
      uintptr_t has_sidetable_rc  : 1;                                         \
      uintptr_t extra_rc          : 8
#   define RC_ONE   (1ULL<<56)
#   define RC_HALF  (1ULL<<7)
```

##### `armv7k or arm64_32` 下
```c++
#   define ISA_INDEX_IS_NPI_BIT  0
#   define ISA_INDEX_IS_NPI_MASK 0x00000001
#   define ISA_INDEX_MASK        0x0001FFFC
#   define ISA_INDEX_SHIFT       2
#   define ISA_INDEX_BITS        15
#   define ISA_INDEX_COUNT       (1 << ISA_INDEX_BITS)
#   define ISA_INDEX_MAGIC_MASK  0x001E0001
#   define ISA_INDEX_MAGIC_VALUE 0x001C0001
#   define ISA_BITFIELD                         \
      uintptr_t nonpointer        : 1;          \
      uintptr_t has_assoc         : 1;          \
      uintptr_t indexcls          : 15;         \
      uintptr_t magic             : 4;          \
      uintptr_t has_cxx_dtor      : 1;          \
      uintptr_t weakly_referenced : 1;          \
      uintptr_t deallocating      : 1;          \
      uintptr_t has_sidetable_rc  : 1;          \
      uintptr_t extra_rc          : 7
#   define RC_ONE   (1ULL<<25)
#   define RC_HALF  (1ULL<<6)
```

## 参考链接
**参考链接:🔗**
+ [苹果架构分类](https://www.jianshu.com/p/63420dfb217c)
+ [OC内存管理-引用计数器](https://www.neroxie.com/2018/12/02/OC内存管理-引用计数器/#more)
+ [Objective-C 1.0 中类与对象的定义](https://kangzubin.com/objc1.0-class-object/)
+ [操作系统内存管理(思维导图详解)](https://blog.csdn.net/hguisu/article/details/5713164)
+ [TaggedPointer](https://www.jianshu.com/p/01153d2b28eb?utm_campaign=maleskine&utm_content=note&utm_medium=seo_notes&utm_source=recommendation)
+ [内存管理](https://www.jianshu.com/p/8d742a44f0da)
+ [Object-C 中的Selector 概念](https://www.cnblogs.com/geek6/p/4106199.html)
