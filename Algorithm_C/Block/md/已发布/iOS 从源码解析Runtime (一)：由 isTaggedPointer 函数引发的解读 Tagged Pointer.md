# iOS 从源码解析Runtime (一)：由 isTaggedPointer 函数引发的解读 Tagged Pointer

> 本来第一篇是 《iOS 从源码解析Runtime (二)：聚焦 objc_object、objc_class、isa》，但是当分析到 `struct objc_object` 的第一个函数 `Class ISA()` 的第一行实现代码时又看到了 `ASSERT(!isTaggedPointer())` 觉的还是有必要再深入总结一下 `Tagged Pointer` 了。

## `Tagged Pointer` 介绍
> `Tagged Pointer` 是苹果为了在 `64` 位架构的处理器下节省内存占用和提高运行效率而提出的概念。它的本质是把一些占用内存较小的对象直接放在指针内部，不再为对象在堆区开辟空间。

&emsp;2013 年 9 月，苹果首次在 `iOS` 平台推出了搭载 `64` 位架构处理器的 `iPhone`（`iPhone 5s`），为了节省内存和提高运行效率，提出了 `Tagged Pointer` 概念。下面我们逐步分析 `Tagged Pointer` 的优点以及结合源码分析它的实现。在 `objc4-781: Private Headers/objc-internal.h Line 245` 定义了 `OBJC_HAVE_TAGGED_POINTERS` 宏，表示在 `__LP64__` 环境中支持 `Tagged Pointer`。
```c++
// Tagged pointer objects.
#if __LP64__
#define OBJC_HAVE_TAGGED_POINTERS 1
#endif
```

> 指针变量的长度与地址总线有关。从 `32` 位系统架构切换到 `64` 位系统架构后，指针变量的长度也会由 `32` 位增加到 `64` 位。如果不考虑其它因素，`64` 位指针可表示的地址长度可达到 `2^64` 字节即 `2^34 TB`，以目前的设备的内存来看，使用 `8` 个字节存储一个地址数据，其实有很多位都是空余的。（例如，在 `iPhone` 真机下，在堆区创建一个 `NSObject` 对象，打印的它的地址，看到只占用了 `36` 位，剩下 `28` 位都是零。） 

## `Tagged Pointer` 内存占用
```c++
#if __LP64__ || 0 || NS_BUILD_32_LIKE_64
// 在 64 位环境中，
// NSInteger 和 NSUInteger 占 8 个字节
typedef long NSInteger;
typedef unsigned long NSUInteger;
#else
// 在 32 位环境中，
// NSInteger 和 NSUInteger 占 4 个字节
typedef int NSInteger;
typedef unsigned int NSUInteger;
#endif
```
```objective-c
// NSValue 继承自 NSObject
// NSNumber 继承自 NSValue
@interface NSNumber : NSValue
...
@end

@interface NSValue : NSObject <NSCopying, NSSecureCoding>
...
@end

// NSString 继承自 NSObject
@interface NSString : NSObject <NSCopying, NSMutableCopying, NSSecureCoding>
...
@end
```
在 `Project Headers/objc-runtime-new.h Line 1651`，`CF` 要求所有对象至少为 `16` 个字节。
```c++
size_t instanceSize(size_t extraBytes) const {
    if (fastpath(cache.hasFastInstanceSize(extraBytes))) {
        return cache.fastInstanceSize(extraBytes);
    }

    size_t size = alignedInstanceSize() + extraBytes;
    // CF requires all objects be at least 16 bytes.
    if (size < 16) size = 16;
    return size;
}
```
&emsp;如果没有 `Tagged Pointer`，在 `32` 位环境中存储一个 `NSInteger` 类型的 `NSNumber` 对象的时候，需要系统在堆区为其分配 `8` 个字节（对象的 `isa` 指针 `4` 字节 + 存储的值 `4` 字节）空间，而到了 `64` 位环境，就会变成 `16` 个字节（（理想状态）对象的 `isa` 指针 `8` 字节 + 存储的值 `8` 字节），然后再加上必要的指针变量在栈区的空间（`32` 位占 `4` 字节/ `64` 位占 `8` 字节），而如果此时 `NSNumber` 对象中仅存储了一个较小的数字，从 `32` 位切到  `64` 位环境即使在逻辑上没有任何改变的情况下，`NSNumber` 这类对象的内存占用也会直接翻一倍。
（在 `64` 位 `iOS` 真机环境下，`NSNumber` 对象中存放 `NSIntegerMax` 时，使用 `malloc_size` 函数，返回 `32`，即系统会为其开辟 `32` 字节的空间，一个 `NSObject` 对象系统会为其开辟 `16` 字节的空间。）

+ 在 `64` 位环境下，非 `Tagged Pointer` 时，`NSNumber` 对象在堆区占用 `16` 字节（`NSObject` 对象是 `16` 字节，`NSNumber` 对象实际占用 `32` 字节） + 指针变量在栈区占用 `8` 字节空间，一共 `24` 字节空间。
+ 在 `64` 位环境下，使用 `Tagged Pointer` 时，`NSNumber` 对象在堆区占用 `0` 字节 + 指针变量在栈区占用 `8` 字节空间，一共 `8` 字节空间。

**`Tagged Pointer` 减少了至少一半的内存占用。**

示例代码:
```objective-c
NSObject *objc = [[NSObject alloc] init];
NSNumber *number = [[[NSNumber alloc] initWithInt:1] copy];
// NSNumber *number = [[NSNumber alloc] initWithLong:NSIntegerMax];
NSLog(@"objc pointer: %zu malloc: %zu CLASS: %@ ADDRESS: %p", sizeof(objc), malloc_size(CFBridgingRetain(objc)), object_getClass(objc), objc);
NSLog(@"number pointer: %zu malloc: %zu CLASS: %@ ADDRESS: %p", sizeof(number), malloc_size(CFBridgingRetain(number)), object_getClass(number), number);

// 控制台打印:
objc pointer: 8 malloc: 16 CLASS: NSObject ADDRESS: 0x282f2c6e0
number pointer: 8 malloc: 0 CLASS: __NSCFNumber ADDRESS: 0xddb739a2fdf961f7
number pointer: 8 malloc: 32 CLASS: __NSCFNumber ADDRESS: 0x282d23da0
```

## 如何判断指针变量是 `Tagged Pointer`
### `isTaggedPointer`
&emsp;定义于 `Project Headers/objc-object.h Line 100` 的 `isTaggedPointer` 函数，用来判断一个指针变量是否是 `Tagged Pointer` 即判断一个对象是否可使用 `Tagged Pointer` 技术直接保存在指针变量的内存中。
```c++
inline bool 
objc_object::isTaggedPointer() 
{
    return _objc_isTaggedPointer(this);
}
```
### `_objc_isTaggedPointer`
&emsp;`_objc_isTaggedPointer` 是定义于 `Private Headers/objc-internal.h Line 442` 的一个返回 `bool` 的静态内联函数。
```c++
// Return true if ptr is a tagged pointer object.
// 如果 ptr 是一个 Tagged Pointer 返回 true。
// Does not check the validity of ptr's class.
// 不检查 ptr 的 class 的有效性，这里只针对指针值的最高位或最低位是 1 还是 0。
static inline bool 
_objc_isTaggedPointer(const void * _Nullable ptr)
{
    // typedef unsigned long uintptr_t;
    
    // 直接把指针值强制转化为 unsigned long
    // 和 _OBJC_TAG_MASK 做与操作
    return ((uintptr_t)ptr & _OBJC_TAG_MASK) == _OBJC_TAG_MASK;
}
```
### `OBJC_MSB_TAGGED_POINTERS`
&emsp;`OBJC_MSB_TAGGED_POINTERS` 表示不同平台下字符串是低位优先排序（`LSD`）还是高位优先排序（`MSD`）。具体细节可参考:[《字符串低位优先排序(LSD)和高位优先排序(MSD)原理及C++实现》](https://blog.csdn.net/weixin_41427400/article/details/79851043)
```c++
#if (TARGET_OS_OSX || TARGET_OS_IOSMAC) && __x86_64__
    // 64-bit Mac - tag bit is LSB
    // 在 64 位 Mac 下采用字符串低位优先排序（LSD）
#   define OBJC_MSB_TAGGED_POINTERS 0
#else
    // Everything else - tag bit is MSB
    // 其他情况下，都是采用字符串高位优先排序 (MSB)
#   define OBJC_MSB_TAGGED_POINTERS 1
#endif
```
### `_OBJC_TAG_MASK`
&emsp;`_OBJC_TAG_MASK` 表示在 `字符串高位优先排序的平台下` 指针变量的第 `64` 位标记该指针为 `Tagged Pointer`，在 `字符串低位优先排序的平台下` 指针变量的第 `1` 位标记该指针为 `Tagged Pointer`。 
```c++
#if OBJC_MSB_TAGGED_POINTERS
#   define _OBJC_TAG_MASK (1UL<<63)
...
#else
#   define _OBJC_TAG_MASK 1UL
...
#endif
```
&emsp;在 `iOS` 真机上判断是否是 `Tagged Pointer` 直接看指针的第 `64` 个比特位是否是 `1`，在 `x86_64` 架构的 `Mac` 下看指针的第 `1` 个比特位是否是 `1`。

## 为何可通过设定最高位或最低位来标识 `Tagged Pointer`

&emsp;这是因为在分配内存的时候，都是按 `2` 的整数倍来分配的，这样分配出来的正常内存地址末位不可能为 `1` ，这样通过将最低标识为 `1` ，就可以和其他正常指针做出区分。那么为什么最高位为 `1` ，也可以标识呢 `？` 这是因为 `64` 位操作系统，设备一般没有那么大的内存，所以内存地址一般只有 `48` 个左右有效位（`64` 位 `iOS` 堆区地址使用了 `36` 位有效位），也就是说高位的 `16` 位左右都为 `0`，所以可以通过最高位标识为 `1` 来表示 `Tagged Pointer`。那么既然 `1` 位就可以标识 `Tagged Pointer` 了其他的信息是干嘛的呢？我们可以想象，要有一些 `bit` 位来表示这个指针对应的类型，不然拿到一个 `Tagged Pointer` 的时候我们不知道类型，就无法解析成对应的值。

## 如何从 `Tagged Pointer` 获取变量类型 `objc_tag_index_t`
&emsp;接着上面 `OBJC_HAVE_TAGGED_POINTERS` 宏定义继续往下看的话，看到枚举 `objc_tag_index_t`，


// tagged pointer









// 再加上对其堆区的读写、引用计数维护、销毁的清理工作等，这些都给程序增加了额外的逻辑，造成效率上的损失。
// 所以苹果采取了一种方式将一些比较小的数据直接存储在指针变量中，这些指针变量就称为 `Tagged Pointer`。
// 题纲
// 1. 减少一半的内存占用的证明：与 32 位时比较/与普通对象比较
// 2. 3 倍的访问速度提升的证明：与普通对象比较
// 3. 100 倍的创建和销毁速度提升的证明：与普通对象比较

// 如何识别 Tagged Pointer
// Tagged Pointer 中读出类型
// 

## 参考链接
**参考链接:🔗**
+ [Objective-C 的 Tagged Pointer 实现](https://www.jianshu.com/p/58d00e910b1e)
+ [译】采用Tagged Pointer的字符串](http://www.cocoachina.com/articles/13449)
+ [TaggedPointer](https://www.jianshu.com/p/01153d2b28eb?utm_campaign=maleskine&utm_content=note&utm_medium=seo_notes&utm_source=recommendation)
+ [深入理解 Tagged Pointer](https://www.infoq.cn/article/deep-understanding-of-tagged-pointer)

