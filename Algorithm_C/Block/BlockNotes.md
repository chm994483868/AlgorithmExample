#  Blocks
> 主要介绍 OS X Snow Leopard(10.6) 和 iOS 4 引入的 C 语言扩充功能 “Blocks” 。究竟是如何扩充 C 语言的，扩充之后又有哪些优点呢？下面通过其实现来一步一步探究。
## 什么是 Blocks 
Blocks 是 C 语言的扩充功能。可以用一句话来表示 Blocks 的扩充功能：带有自动变量（局部变量）的匿名函数。(对于程序员而言，命名就是工作的本质。)
> 回顾下 C 语言的函数中可能使用的变量：
> + 自动变量（局部变量）
> + 函数的参数
> + 静态变量（静态局部变量）
> + 静态全局变量
> + 全局变量
Block 类型变量与一般的 C 语言变量完全相同，可作为以下用途使用：
+ 自动变量
+ 函数参数
+ 静态变量
+ 静态全局变量
+ 全局变量

使用 Block  语法将 Block 赋值为 Block 类型变量。
```
int (^blk)(int) = ^(int count) { return count + 1; };
```
```
// 返回值是 Block 类型的函数
void (^ func() )(int) {
    return ^(int count) { return count + 1; }; 
}
```
将赋值给 Block 类型变量中的 Bolock 方法像 C  语言通常的函数调用那样使用，这种方法与使用函数指针类型变量调用函数的方法几乎完全相同。
Block 类型变量可完全像通常的 C 语言变量一样使用，因此也可以使用指向 Block 类型变量的指针，即 Block 的指针类型变量。
```
typedef int (^blk_t)(int);
blk_t blk = ^(int count) { return count + 1; };
blk_t* blkPtr = &blk;
(*blkPrt)(10);
```
“带有自动变量值” 在 Blocks 中表现为 “截获自动变量值”。


