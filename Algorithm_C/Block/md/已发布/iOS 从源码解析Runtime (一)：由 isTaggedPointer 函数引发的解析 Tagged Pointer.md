# iOS 从源码解析Runtime (一)：由 isTaggedPointer 函数引发的解析 Tagged Pointer

> 本来第一篇是 《iOS 从源码解析Runtime (二)：聚焦 objc_object、objc_class、isa》，但是当分析到 `struct objc_object` 的第一个函数 `Class ISA()` 的第一行实现代码时又看到了 `ASSERT(!isTaggedPointer())` 觉的还是有必要再深入总结一下 `Tagged Pointer` 了。

## `Tagged Pointer` 介绍
> `Tagged Pointer` 是苹果为了在 64 位架构的处理器下节省内存和提高执行效率而提出的概念。

&emsp;2013 年 9 月，苹果首次在 `iOS` 平台推出了搭载 `64` 位架构处理器的 `iPhone`（`iPhone 5s`），为了节省内存和提高执行效率，提出了 `Tagged Pointer` 概念。
指针变量的长度与地址总线有关，所以从 `32` 位系统架构切换到 `64` 位系统架构后，指针变量的长度也会由 `32` 位增加到 `64` 位。如果不考虑其它因素，`64` 位指针可表示的地址长度可达到 `2^64` 字节，以目前的设备内存容量来看的话 `64` 位指针变量的空间内会有很多空位。

&emsp;如果没有 `Tagged Pointer`，在 `32` 位机器上存储一个 `int` 类型的 `NSNumber` 对象的时候，需要系统在堆区为其分配 `8` 个字节（对象的 `isa` 指针 + 存储的值）空间，而到了 `64` 位机器，就会变成 `16` 个字节（对象的 `isa` 指针 + 存储的值），然后再加上必要的指针变量在栈区的空间（ `4` 字节/ `8` 字节），而如果此时 `NSNumber` 对象中仅存储了一个较小的数字或者 `NSString` 对象中仅存储了一两个字符，那也太浪费存储空间了，特别是从 `32` 位切到  `64` 位即使逻辑上没有任何改变的情况下，`NSNumber`、`NSString` 这类对象的内存占用也会直接翻一倍，再加上对其堆区的读写、引用计数维护、销毁的清理工作等，这些都给程序增加了额外的逻辑，造成效率上的损失。

所以苹果采取了一种方式将一些比较小的数据直接存储在指针变量中，这些指针变量就称为 `Tagged Pointer`。



## 参考链接
**参考链接:🔗**
+ [Objective-C 的 Tagged Pointer 实现](https://www.jianshu.com/p/58d00e910b1e)
+ [译】采用Tagged Pointer的字符串](http://www.cocoachina.com/articles/13449)
+ [TaggedPointer](https://www.jianshu.com/p/01153d2b28eb?utm_campaign=maleskine&utm_content=note&utm_medium=seo_notes&utm_source=recommendation)
+ [深入理解 Tagged Pointer](https://www.infoq.cn/article/deep-understanding-of-tagged-pointer)

