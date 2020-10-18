#  iOS 从 libclosure-74 源码来研究 Block 实现原理(2)

> 上篇分析了关于 `Block` 的各种理论，还没有触及 `libclosure-74` 的源码细节，本篇则针对上篇结束时提及的问题从源码角度来一一解读。⛽️⛽️

## 前期准备
&emsp;我们先创建一个如下 `NSObject` 的 `category`，并且在 `Compile Sources` 中把 `category` 的 `.m` 文件的 `Compiler Flags` 标记为 `-fno-objc-arc`，其中的 `retainCountForARC` 函数可以帮助我们在 `ARC` 下调用对象的 `retainCount` 方法，然后根据返回的引用计数值来判断外部对象是否被 `block` 持有了。
```c++
// NSObject+RetainCountForARC.h 
@interface NSObject (RetainCountForARC)

- (NSUInteger)retainCountForARC;

@end

// NSObject+RetainCountForARC.m
#import "NSObject+RetainCountForARC.h"
@implementation NSObject (RetainCountForARC)

- (NSUInteger)retainCountForARC {
    return [self retainCount];
}

@end
```

## `block.h` 
&emsp;`block.h` 声明了四个函数接口，以及两个 `block` 的初始类型。
### `_Block_copy`
```c++
// Create a heap based copy of a Block or simply add a reference to an existing one.
// This must be paired with Block_release to recover memory,
// even when running under Objective-C Garbage Collection.

// 创建基于堆的 block 副本，或仅添加对现有 block 的引用。
// 使用时必须与 Block_release 配对使用来恢复内存，
// 即使在 Objective-C 垃圾回收下运行也是如此。

BLOCK_EXPORT void *_Block_copy(const void *aBlock) __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
```
### `_Block_release`
```c++
// Lose the reference, and if heap based and last reference, recover the memory
// 释放引用，如果是堆 Block 且释放的是最后一个引用，释放引用后并释放内存。
//（类似 ARC 的 release 操作，先是减少引用计数，如果减少到 0 了，则执行 dealloc）

BLOCK_EXPORT void _Block_release(const void *aBlock) __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
```
### `_Block_object_assign`
```c++
// Used by the compiler. Do not call this function yourself.
// 由编译器主动调用，不要自己调用此函数。

BLOCK_EXPORT void _Block_object_assign(void *, const void *, const int) __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
```
### `_Block_object_dispose`
```c++
// Used by the compiler. Do not call this function yourself.
// 由编译器主动调用，不要自己调用此函数。
BLOCK_EXPORT void _Block_object_dispose(const void *, const int) __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
```
### `_NSConcreteGlobalBlock/_NSConcreteStackBlock`
```c++
BLOCK_EXPORT void * _NSConcreteGlobalBlock[32] __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
BLOCK_EXPORT void * _NSConcreteStackBlock[32] __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
```
&emsp;这里只把 `Global` 和 `Stack` 类型的 `Block` 的声明放在了 `.h` 文件中，`Block` 类型除了这两种之外，还有声明在 `Block_private.h` 中的多个其它类型。而这里仅仅把 `Global` 和 `Stack` 放出来，是因为当我们构建 `block` 时，`block` 的起始类型只可能是 `Global` 和 `Stack` 类型中的一种，然后其它几种 `Block` 类型都是 `_NSConcreteStackBlock` 类型的 `block` 执行复制（调用 `_Block_copy` 函数）时根据 `block` 定义内容不同来动态指定的。（`Global` 类型的 `block`，执行复制操作直接返回它自己）
&emsp;在上一篇我们使用 `clang` 转换 `block` 定义时已经见到过，`block` 所属的类型是由 `struct __block_impl` 的 `void *isa` 指针来指向的。下面我们对 `Block` 的 `6` 种类型来进行解析。

#### `Block 类型`
&emsp;在 `libclosure-74/Block_private.h Line 499` 中声明了所有 `Block` 类型：
```c++
// the raw data space for runtime classes for blocks class+meta used for stack, malloc, and collectable based blocks.
// block 的 class + meta 的数据空间。

BLOCK_EXPORT void * _NSConcreteMallocBlock[32] __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
BLOCK_EXPORT void * _NSConcreteAutoBlock[32] __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
BLOCK_EXPORT void * _NSConcreteFinalizingBlock[32] __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
BLOCK_EXPORT void * _NSConcreteWeakBlockVariable[32] __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_3_2);
// declared in Block.h
// BLOCK_EXPORT void * _NSConcreteGlobalBlock[32];
// BLOCK_EXPORT void * _NSConcreteStackBlock[32];
```
&emsp;其中 `_NSConcreteGlobalBlock`、`_NSConcreteStackBlock`、`_NSConcreteMallocBlock` 是三种最常见的类型，下面我们也会仔细分析这三种类型，另外的 `_NSConcreteFinalizingBlock`、`_NSConcreteAutoBlock`、`_NSConcreteWeakBlockVariable` 三种类型只在 `GC` 环境下使用。

+ 以下情况 `block` 会初始化为 `_NSConcreteGlobalBlock`：（要求此 `block` 以及其内部嵌套的 `block` 符合以下条件）
1. `MRC` 或者 `ARC` 下，未截获任何外部变量时。
```c++
{
    LGPerson *person = [[LGPerson alloc] init];
    
    // 内层 block 根据外层 block 输入的指数计算出 2 的幂并返回
    NSLog(@"%@", ^(int exponent){ return ^{ return pow(2, exponent); }(); });
    
    // 打印 🖨️：
    // <__NSGlobalBlock__: 0x106763048>
    
    // 内部嵌套一个 global Block，如果内部嵌套了一个 stack block 的话，外层 block 的类型也会是 stack block
    // 如 MRC 下：（MRC 下 block 截获外部的自动变量时类型时 Stack Block，ARC 下则是 Malloc Block）
    NSLog(@"%@", ^(int exponent){ ^{ NSLog(@"%@", person); }; });
    
    // 打印 🖨️：
    // MRC 下：<__NSStackBlock__: 0x7ffee5a230f0>
    // ARC 下：<__NSMallocBlock__: 0x600001cd7120>
    
    // 这里即使发生了赋值操作，因为右侧是 Global block，所以 blk 依然是 Global block
    double (^blk)(int exponent) = ^(int exponent){ return ^{ return pow(2, exponent); }(); };
    NSLog(@"%@", blk);
    NSLog(@"%f", blk(4));
    
    // 打印 🖨️：
    // <__NSGlobalBlock__: 0x106763088>
    // 16.000000
    
    NSLog(@"🍎🍎🍎:  %ld", [person retainCountForARC]);
    
    // 打印 🖨️：
    // MRC 下： 🍎🍎🍎:  1
    // ARC 下： 🍎🍎🍎:  2 // 被 block 持有 1 次
}

// 打印:
// ARC 下：🍀🍀🍀 LGPerson dealloc
// MRC 需要自己主动调用一次 release 操作
```
2. `MRC` 或 `ARC` 下仅截获 **全局变量**、**静态全局变量**、**静态局部变量** 时。
```c++
static int static_val = 14;
NSLog(@"%@", ^{ ^{ NSLog(@"%d", global_static_val); }; });
NSLog(@"%@", ^{ ^{ NSLog(@"%d", global_val); }; });
NSLog(@"%@", ^{ ^{ NSLog(@"%d", static_val); }; });
// 打印：
<__NSGlobalBlock__: 0x10bccc040>
<__NSGlobalBlock__: 0x10bccc060>
<__NSGlobalBlock__: 0x10bccc080>
```
+ 以下情况 `block` 会初始化为 `_NSConcreteStackBlock`：
1. `MRC` 下截获 **外部局部变量** 时。
```c++
{    
    LGPerson *person = [[LGPerson alloc] init];
    NSLog(@"%@", ^{ NSLog(@"%@", person);});
    NSLog(@"🍎🍎🍎:  %ld", [person retainCountForARC]);
}
// MRC 下打印：
// <__NSStackBlock__: 0x7ffee0c510f0>
// 🍎🍎🍎:  1 // MRC 下栈区 block 不持有外部局部变量

// ARC 下打印：
// <__NSMallocBlock__: 0x6000022126d0>
// 🍎🍎🍎:  2 // ARC 下堆区 block 持有外部局部变量
// 🍀🍀🍀 LGPerson dealloc
```
+ 以下情况 `block` 为转化为 `_NSConcreteMallocBlock`：
1. `MRC` 下 `_NSConcreteStackBlock` 调用 `copy` 函数（`_Block_copy`），`block` 的 `isa` 会被转换为 `_NSConcreteMallocBlock`（`result->isa = _NSConcreteMallocBlock;`）。
```c++
{
    LGPerson *person = [[LGPerson alloc] init];
    void (^blk)(void) = [^{ NSLog(@"%@", person);} copy];
    NSLog(@"%@", blk);
    NSLog(@"🍎🍎🍎:  %ld", [person retainCountForARC]);
}
// MRC 下打印：
// <__NSMallocBlock__: 0x600002822940>
// 🍎🍎🍎:  2 // 栈区 block 被复制到堆区时 持有 person 对象

// ARC 下打印：（即使不调用 copy 函数，也是同样的打印）
// <__NSMallocBlock__: 0x60000227f8a0>
// 🍎🍎🍎:  3 // 1): person 持有。 2): 等号右边 block 持有。 3): 等号左边 block 持有。
// 🍀🍀🍀 LGPerson dealloc
```
+ 在 `GC` 环境下，当 `block` 被复制时，如果 `block` 有 `ctors & dtors` 时，则会转换为 `_NSConcreteFinalizingBlock` 类型，反之，则会转换为 `_NSConcreteAutoBlock` 类型。
+ 在 `GC` 环境下，当对象被 `__weak __block` 修饰，且从栈复制到堆时，`block` 会被标记为 `_NSConcreteWeakBlockVariable` 类型。

## `Block_private.h`
&emsp;`Block_private.h` 中定义了 `struct Block_byref`，作为 `block` 的数据结构，对应上篇我们使用 `clang` 转换 `block` 得到的 `struct __main_block_impl_0`。定义了 `struct Block_byref`，作为 `__block` 变量的数据结构，对应上篇我们使用 `clang` 转换 `__block` 变量得到的 `struct __block_byref_val_0`。同时还定义了一些函数接口，下面进行详细分析。

### `Block_layour->flags`
&emsp;`struct Block_layout` 的成员变量 `flags` 类型是 `volatile int32_t` 共 `32` 位，下面的枚举值对应 `flags` 的某些位来标记 `Block_layout` 是否正在释放、引用计数的掩码、`block` 的类型、以及 `block` 的 `DESCRIPTOR` 信息等。
```c++
// Values for Block_layout->flags to describe block objects.
// 用于描述 block 对象的 Block_layout->flags 的值。

enum {
    BLOCK_DEALLOCATING =      (0x0001),  // runtime 用于运行时，标记 block 正在进行释放，
                                                    // 转化为二进制共 16 位，前 15 位是 0，最后一位是 1
                                                    
    BLOCK_REFCOUNT_MASK =     (0xfffe),  // runtime 用于运行时，block 引用计数的掩码
                                                    // 转化为二进制共 16 位，前 15 位是 1，最后一位是 0
                                                    
    // 16 位到 23 位 未使用
    
    BLOCK_NEEDS_FREE =        (1 << 24), // runtime 用于运行时，1 左移 24 位，第 24 位是 1，标识 block 是 堆 Block（24 位是 1）
    
    // 用于编译时，标识 block 有 copy dispose 助手
    
    // 判断 Block 是否有 copy_dispose 助手 即 description2 中的 copy 和 dispose 函数
    // 对应上篇 clang 转化中的 static struct __main_block_desc_0 中的
    // void (*copy)(struct __main_block_impl_0*, struct __main_block_impl_0*) 和
    // void (*dispose)(struct __main_block_impl_0*);
    
    BLOCK_HAS_COPY_DISPOSE =  (1 << 25), // compiler（25 位是 1）
    
    BLOCK_HAS_CTOR =          (1 << 26), // compiler: helpers have C++ code 标记 block 有 ctors & dtors（26 位是 1）
    BLOCK_IS_GC =             (1 << 27), // runtime 用于运行时，标记 block 是否处于 GC 环境（27 位是 1）
    
    BLOCK_IS_GLOBAL =         (1 << 28), // compiler 用于编译时，1 左移 28 位，第 28 位是 1，标识 block 是 全局 Block（28 位是 1）
    
    BLOCK_USE_STRET =         (1 << 29), // compiler: undefined if !BLOCK_HAS_SIGNATURE（29 位是 1）
    
    BLOCK_HAS_SIGNATURE  =    (1 << 30), // compiler 用于编译时，标记 block 有签名，可用于 block hook（30 位是 1）
    BLOCK_HAS_EXTENDED_LAYOUT=(1 << 31)  // compiler 用于编译时，标记 block 是否有延展布局（31 位是 1）
};
```
### `BLOCK_DESCRIPTOR`
&emsp;此处三个结构体各自包含一些描述信息或者功能，不同的 `block` 会包含其中一些或者全部，当包含某些描述信息或者功能时，它们会被追加到我们上篇看到的 `static struct __main_block_desc_0` 中，其中 `struct Block_descriptor_1` 所包含的 `reserved`（保留字段） 和 `size`（`block` 结构体所占用内存大小，如 `sizeof(struct __main_block_impl_0)`），它们两个是所有 `block` 都会包含的，然后像是 `struct Block_descriptor_2` 中的 `copy` 和 `dispose` 则是在 `block` 中截获外部对象类型或者 `__block` 变量时才会有的，且它们都是在编译时根据 `block` 的定义来确定的。
```c++
#define BLOCK_DESCRIPTOR_1 1

// block 结构体的默认描述信息
struct Block_descriptor_1 {
    uintptr_t reserved; // 保留字段
    uintptr_t size; // block 结构体的大小
};

#define BLOCK_DESCRIPTOR_2 1

// block 结构体包含 copy 和 dispose 函数
struct Block_descriptor_2 {
    // requires BLOCK_HAS_COPY_DISPOSE
    // Block_layout->flags & BLOCK_HAS_COPY_DISPOSE == 1
    
    // typedef void(*BlockCopyFunction)(void *, const void *);
    // typedef void(*BlockDisposeFunction)(const void *);
    
    BlockCopyFunction copy;
    BlockDisposeFunction dispose;
};

#define BLOCK_DESCRIPTOR_3 1

// block 结构体存在 signature 和 layout
struct Block_descriptor_3 {
    // requires BLOCK_HAS_SIGNATURE
    // Block_layout->flags & BLOCK_HAS_SIGNATURE == 1
    
    const char *signature;
    const char *layout;     // contents depend on BLOCK_HAS_EXTENDED_LAYOUT
};
```
### `struct Block_layout`
&emsp;`block` 的本质正是 `struct Block_layout`。
```c++
struct Block_layout {
    void *isa; // block 所属类型
    volatile int32_t flags; // contains ref count 包含引用计数等一些信息
    int32_t reserved; // block 的保留信息
    
    // typedef void(*BlockInvokeFunction)(void *, ...);
    BlockInvokeFunction invoke; // 函数指针，指向 block 要执行的函数（即 block 定义中花括号中的表达式）
    
    // 上篇中我们看到上面四个字段被综合放在了 struct __block_impl 中
    
    // block 附加描述信息，默认所有 block 都包含 Block_descriptor_1 中的内容
    struct Block_descriptor_1 *descriptor;
    
    // 主要保存了内存 size 大小以及 copy 和 dispose 函数的指针及签名和 layout 等信息，
    // 通过源码可发现，layout 中只包含了 Block_descriptor_1，
    // 并未包含 Block_descriptor_2 和 Block_descriptor_3，
    // 这是因为在捕获不同类型变量或者没用到外部变量时，编译器会改变结构体的结构，
    // 按需添加 Block_descriptor_2 和 Block_descriptor_3，
    // 所以才需要 BLOCK_HAS_COPY_DISPOSE 和 BLOCK_HAS_SIGNATURE 等枚举来判断
    
    // imported variables
    // capture 的外部变量，
    // 如果 block 表达式中截获了外部变量，block 结构体中就会有添加相应的成员变量
    // 如果是 __block 变量则添加对应结构体类型为其成员变量，
    // 非 __block 变量则是直接添加对应类型的成员变量。
    // 同时在 Block 的结构体初始化时将使用截获的值或者指针来初始化对应的成员变量。
};
```
### `Block_byref->flags`
&emsp;`struct Block_byref` 的成员变量 `flags` 类型是 `volatile int32_t` 共 `32` 位，下面的枚举值对应 `flags` 的某些位来标记 `Block_byref` 是否需要释放、是否有 `copy` 和 `dispose`、`Layout` 的掩码、`__block` 修饰的变量类型等等。
```c++
// Values for Block_byref->flags to describe __block variables.
// 用于描述 __block 变量的 Block_byref->flags 的值。

// 变量在被 __block 修饰时由编译器来生成 struct Block_byref 实例。 
enum {
    // Byref refcount must use the same bits as Block_layout's refcount.
    // Byref refcount 必须使用与 Block_layout 的 refcount 相同的位
    
    // BLOCK_DEALLOCATING =      (0x0001),  // runtime
    // BLOCK_REFCOUNT_MASK =     (0xfffe),  // runtime

    BLOCK_BYREF_LAYOUT_MASK =       (0xf << 28), // compiler 掩码 0b1111 左移 28 位
    BLOCK_BYREF_LAYOUT_EXTENDED =   (  1 << 28), // compiler
    BLOCK_BYREF_LAYOUT_NON_OBJECT = (  2 << 28), // compiler
    BLOCK_BYREF_LAYOUT_STRONG =     (  3 << 28), // compiler
    BLOCK_BYREF_LAYOUT_WEAK =       (  4 << 28), // compiler
    BLOCK_BYREF_LAYOUT_UNRETAINED = (  5 << 28), // compiler

    BLOCK_BYREF_IS_GC =             (  1 << 27), // runtime 用于运行时，表示当前时 GC 环境

    BLOCK_BYREF_HAS_COPY_DISPOSE =  (  1 << 25), // compiler 用于编译时，表示 Block_byref 含有 copy 和 dispose
    BLOCK_BYREF_NEEDS_FREE =        (  1 << 24), // runtime 用于运行时，表示是否需要进行释放操作
};
```
### `struct Block_byref`
&emsp;`__block` 变量的本质正是 `struct Block_byref`。
```c++
struct Block_byref {
    void *isa; // 指向父类，一般直接指向 0
    
    // __block 结构体实例在栈中时指向自己，
    // 截获 __block 变量的 栈区 block 执行 copy 后，
    // 栈中 __block 结构体实例的 __forwarding 指向堆中的 byref（__block 变量），
    // 堆中 __block 结构体实例的 __forwarding 指向自己
    
    struct Block_byref *forwarding; // 指向自己的指针 
    
    volatile int32_t flags; // contains ref count
    uint32_t size; // __block 结构体所占内存大小
};
```
### `struct Block_byref_2`
&emsp;这里有点像上面的 `block` 结构体的描述信息，根据 `block` 的定义追加不同的描述。这里是当 `__block` 修饰的不同类型时也是追加不同的功能，比如下面的 `Keep` 和 `Destroy` 操作，当 `__block` 修饰的是对象类型时就会追加到 `__block` 的结构体中，而如果修饰的是基本类型的话则不会添加这两个功能。
```c++
struct Block_byref_2 {
    // requires BLOCK_BYREF_HAS_COPY_DISPOSE
    // Block_byref->flags & BLOCK_BYREF_HAS_COPY_DISPOSE == 1
    
    // typedef void(*BlockByrefKeepFunction)(struct Block_byref*, struct Block_byref*);
    // typedef void(*BlockByrefDestroyFunction)(struct Block_byref *);
    
    // 在 ARC 下当截获 __block 对象类型变量的栈区 block 被复制到堆区时，__block 中的对象类型值引用计数会 + 1
    BlockByrefKeepFunction byref_keep;
    
    // 销毁 __block 变量
    BlockByrefDestroyFunction byref_destroy;
};
```
### `struct Block_byref_3`
&emsp;包含有布局扩展，当 `__block` 修饰不同类型的变量时，对应类型的成员变量会追加在 `struct Block_byref` 中。
```c++
struct Block_byref_3 {
    // requires BLOCK_BYREF_LAYOUT_EXTENDED
    // Block_byref->flags & BLOCK_BYREF_LAYOUT_EXTENDED == 1
    
    const char *layout;
};
```
### `Block 截获的外部变量类型`
&emsp;以下枚举值标识 `Block` 截获不同类型的外部变量。
```c++
// Runtime support functions used by compiler when generating copy/dispose helpers.
// 当编译器生成 copy/dispose helpers 时 Runtime 支持的函数.

// Values for _Block_object_assign() and _Block_object_dispose() parameters
// 作为 _Block_object_assign() 和 _Block_object_dispose() 函数的参数.

enum {
    // see function implementation for a more complete description of these fields and combinations.
    // 有关这些字段及其组合的更完整说明，请参见函数实现。
    
    // 0b11
    // 对象类型 
    BLOCK_FIELD_IS_OBJECT   =  3,  // id, NSObject, __attribute__((NSObject)), block, ...
    
    // 0b111
    // block 变量 
    BLOCK_FIELD_IS_BLOCK    =  7,  // a block variable
    
    // 0b1000
    // __block 说明符生成的结构体，持有 __block 变量的堆栈结构 
    BLOCK_FIELD_IS_BYREF    =  8,  // the on stack structure holding the __block variable
    
    // 0b10000
    // 被 __weak 修饰过的弱引用，只在 Block_byref 管理内部对象内存时使用
    // 也就是 __block __weak id; 仅使用 __weak 时，还是 BLOCK_FIELD_IS_OBJECT，
    // 即如果是对象类型，有没有添加 __weak 修饰都是一样的，都会生成 copy 助手
    BLOCK_FIELD_IS_WEAK     = 16,  // declared __weak, only used in byref copy helpers
    
    // 0b1000 0000
    // 在处理 Block_byref 内部对象内存的时候会加一个额外标记，配合上面的枚举一起使用
    BLOCK_BYREF_CALLER      = 128, // called from __block (byref) copy/dispose support routines.
};

enum {
    // 上述情况的整合，以下的任一中情况下编译器都会生成 copy_dispose 助手（即 copy 和 dispose 函数）
    
    BLOCK_ALL_COPY_DISPOSE_FLAGS = 
        BLOCK_FIELD_IS_OBJECT | BLOCK_FIELD_IS_BLOCK | BLOCK_FIELD_IS_BYREF |
        BLOCK_FIELD_IS_WEAK | BLOCK_BYREF_CALLER
};
```
## `runtime.cpp`
&emsp;`Block` 的核心内容的实现。
### `latching_incr_int`
```c++
// 传实参 &aBlock->flags 过来，
// 增加 Block 的引用计数
static int32_t latching_incr_int(volatile int32_t *where) {
    while (1) {
        int32_t old_value = *where;
        // 如果 flags 含有 BLOCK_REFCOUNT_MASK 证明其引用计数达到最大，
        // 直接返回，需要三万多个指针指向，正常情况下不会出现。
        // BLOCK_REFCOUNT_MASK =     (0xfffe)
        // 0x1111 1111 1111 1110 // 10 进制 == 65534 // 以 2 为单位，每次递增 2
        if ((old_value & BLOCK_REFCOUNT_MASK) == BLOCK_REFCOUNT_MASK) {
            return BLOCK_REFCOUNT_MASK;
        }
        
        // 做一次原子性判断其值当前是否被其他线程改动，
        // 如果被改动就进入下一次循环直到改动结束后赋值。
        // OSAtomicCompareAndSwapInt 的作用就是在 where 取值与 old_value 相同时，
        // 将 old_value + 2 赋给 where
        // 注: Block 的引用计数以 flags 的后 16 位代表，
        // 以 2 为单位，每次递增 2，1 为 BLOCK_DEALLOCATING，表示正在释放占用。
        // 2 二进制表示是 0x10
        if (OSAtomicCompareAndSwapInt(old_value, old_value+2, where)) {
            return old_value+2;
        }
    }
}
```


## 参考链接
**参考链接:🔗**
+ [libclosure-74](https://opensource.apple.com/source/libclosure/libclosure-74/)
+ [深入理解Block之Block的类型](https://www.jianshu.com/p/0855b68d1c1d)
+ [深入研究Block捕获外部变量和__block实现原理](https://www.jianshu.com/p/ee9756f3d5f6)
+ [【iOS】Block Hook概念+BlockHook第三方库分析（基本原理已完结，补充libffi方法解释）](https://blog.csdn.net/qq_32792839/article/details/99842250)

