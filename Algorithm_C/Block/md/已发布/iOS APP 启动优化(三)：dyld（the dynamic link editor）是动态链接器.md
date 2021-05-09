# iOS APP 启动优化(三)：dyld（the dynamic link editor）是动态链接器


## 静态库与动态库

&emsp;TARGETS -> Build Phases -> Link Binary With Libraries -> (Add/Add Other...) 中我们可以添加多个系统库或我们自己的库，其中便可包含静态库和动态库。

&emsp;静态库通常以 .a .lib 或者 .framework 结尾，动态库以 .dylib .tbd .so .framework 结尾。（等等，.framework 可能是静态库也可能是动态库，后面我们会详细分析。）链接时，静态库会被完整的复制到可执行文件中，被多次使用就会有多份冗余拷贝，系统动态库链接时不复制，程序运行时由系统动态加载到内存中，供程序调用，系统只加载一次，多个程序共用，节省内存。

&emsp;Shift + command + n 创建 new project，在 Framework & library 中，Framework 选项默认是创建 Dynamic Library（动态库），Static Library 选项默认是创建 Static Library（静态库），创建完成的 Mach-O Type 的值告诉了我们他们对应的类型。 当然我们也能直接切换不同的 Mach-0 Type，如 Static Library 和 Dynamic Library 进行切换。而且从 Products 中看到默认情况下动态库是 .framework 后缀，静态库是 .a 后缀，同时还看到动态库是需要进行签名的，而静态库则不需要。

![截屏2021-05-09 11.00.43.png](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/bb53b5c4153b4ff6a47032975aabd997~tplv-k3u1fbpfcp-watermark.image)

![截屏2021-05-09 10.59.23.png](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/f48dc63bdc744453afeee353a127a7b7~tplv-k3u1fbpfcp-watermark.image)

![截屏2021-05-09 11.00.26.png](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/3ee1769190f44ad2a8a58db9692cfeae~tplv-k3u1fbpfcp-watermark.image)

> &emsp;如果我们创建的 framework 是动态库，那么我们直接在工程里使用的时候会报错：Reason: Image Not Found。需要在工程的 General 里的 Frameworks, Libraries, and Embedded Content 添加这个动态库并设置其 Embed 为 Embed & Sign 才能使用。
因为我们创建的这个动态库其实也不能给其他程序使用的，而你的 App Extension 和 APP 之间是需要使用这个动态库的。这个动态库可以 App Extension 和 APP 之间共用一份（App 和 Extension 的 Bundle 是共享的），因此苹果又把这种 Framework 称为 Embedded Framework，而我把这个动态库称为伪动态库。[iOS里的动态库和静态库](https://www.jianshu.com/p/42891fb90304)

```c++
dyld: Library not loaded: @rpath/DYLIB.framework/DYLIB
  Referenced from: /Users/hmc/Library/Developer/CoreSimulator/Devices/4E072E27-E586-4E81-A693-A02A3ED83DEC/data/Containers/Bundle/Application/1208BD23-B788-4BF7-A4CE-49FBA99BA330/Test_ipa_Simple.app/Test_ipa_Simple
  Reason: image not found
```

&emsp;我们创建的动态库和系统的动态库有什么区别呢？我们创建的动态库是在我们自己应用的 .app 目录里面，只能自己的 App Extension 和 APP 使用。而系统的动态库是在系统目录里面，所有的程序都能使用。（我们在模拟器上运行的时候用 `NSBundle *bundel = [[NSBundle mainBundle] bundlePath];` 就能得到 .app 的路径，在第一篇中我们有详细讲解 .ipa 和 .app 目录中的内容，这里不再展开。）






## LLDB 常用命令

1. p po p/x p/o p/t p/d p/c
2. expression 修改参数
3. call 
4. x x/4gx x/4xg
5. image list
6. image lookup --address+地址
7. thread list
8. thread backtrace（bt）bt all
9. thread return frame variable
10. register read register read/x

## clang 

&emsp;clang:Clang 是一个 C++ 编写、基于 LLVM、发布于 LLVM BSD 许可证下的 C/C++/Objective-C/ Objective-C++ 编译器。它与 GNU C 语言规范几乎完全兼容(当然，也有部分不兼容的内容， 包括编译命令选项也会有点差异)，并在此基础上增加了额外的语法特性，比如 C 函数重载 (通过 \_ attribute_((overloadable)) 来修饰函数)，其目标(之一)就是超越 GCC。

```c++
// main.m 代码如下：

__attribute__((constructor)) void main_front() {
    printf("🦁🦁🦁 %s 执行 \n", __func__);
}

__attribute__((destructor)) void main_back() {
    printf("🦁🦁🦁 %s 执行 \n", __func__);
}

int main(int argc, char * argv[]) {
    NSLog(@"🦁🦁🦁 %s 执行", __func__);
    
//    NSString * appDelegateClassName;
//    @autoreleasepool {
//        // Setup code that might create autoreleased objects goes here.
//        appDelegateClassName = NSStringFromClass([AppDelegate class]);
//    }
//    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
    
    return 0;
}

// ViewController.m 代码如下：

@implementation ViewController

+ (void)load {
    NSLog(@"🦁🦁🦁 %s 执行", __func__);
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}

@end

// 运行后控制台打印如下：

2021-05-07 14:46:45.238651+0800 Test_ipa_Simple[43277:456220] 🦁🦁🦁 +[ViewController load] 执行
🦁🦁🦁 main_front 执行 
2021-05-07 14:46:45.242218+0800 Test_ipa_Simple[43277:456220] 🦁🦁🦁 main 执行
🦁🦁🦁 main_back 执行 
```
&emsp;\_\_attribute__ 可以设置函数属性(Function Attribute)、变量属性(Variable Attribute)和类型属性(Type Attribute)。\_\_attribute__ 前后都有两个下划线，并且后面会紧跟一对原括弧，括弧里面是相应的 \_\_attribute__ 参数，\_\_attribute__ 语法格式为：`__attribute__ ( ( attribute-list ) )`。

&emsp;若函数被设定为 constructor 属性，则该函数会在 main 函数执行之前被自动的执行。类似的，若函数被设定为 destructor 属性，则该函数会在 main 函数执行之后或者 exit 被调用后被自动的执行。
















## 参考链接
**参考链接:🔗**
+ [OC底层原理之-App启动过程（dyld加载流程）](https://juejin.cn/post/6876773824491159565)
+ [iOS里的动态库和静态库](https://www.jianshu.com/p/42891fb90304)

