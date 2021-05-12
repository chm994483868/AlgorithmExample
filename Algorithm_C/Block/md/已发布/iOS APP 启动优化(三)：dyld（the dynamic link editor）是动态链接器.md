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

&emsp;这里继续依我们的 Test_ipa_Simple 为例，并把上面我们自己构建的动态库 `DYLIB` 和 静态库 `STATICLIB` 导入  Test_ipa_Simple 中，直接运行的话会报如下找不到 `DYLIB.framework` 我们把其 Embed 置为 Embed & Sign 即可正常运行，如果报找不到 STATICLIB 的话，则是在 Build Settings 的 Library Search Paths 和 Header Search Paths 中正确的导入 STATICLIB 及 .h 的路径。（同时为了作为对比，我们在 Build Phases -> Link Binary With Libraries 中导入 `WebKit.framework`。）

```c++
dyld: Library not loaded: @rpath/DYLIB.framework/DYLIB
  Referenced from: /Users/hmc/Library/Developer/CoreSimulator/Devices/4E072E27-E586-4E81-A693-A02A3ED83DEC/data/Containers/Bundle/Application/1208BD23-B788-4BF7-A4CE-49FBA99BA330/Test_ipa_Simple.app/Test_ipa_Simple
  Reason: image not found
```

```c++
hmc@bogon Test_ipa_Simple.app % file Test_ipa_Simple 
Test_ipa_Simple: Mach-O 64-bit executable arm64
```

```c++
hmc@bogon DYLIB.framework % file DYLIB 
DYLIB: Mach-O 64-bit dynamically linked shared library arm64
```

```c++
hmc@bogon Debug-iphoneos % file libSTATICLIB.a 
libSTATICLIB.a: current ar archive random library
```

&emsp;我们创建的动态库和系统的动态库有什么区别呢？

1. 我们导入到项目中的我们自己创建的动态库是在我们自己应用的 .app 目录里面，只能自己的 App Extension 和 APP 使用。
2. 我们导入到项目中的系统的动态库是在系统目录里面，所有的程序都能使用。

&emsp;（我们在模拟器上运行的时候用 `NSBundle *bundel = [[NSBundle mainBundle] bundlePath];` 就能得到 .app 的路径，在第一篇中我们有详细讲解 .ipa 和 .app 目录中的内容，这里不再展开。）

&emsp;我们自己创建的动态库就在 .app 目录下的 Framework 文件夹里，对 Test_ipa_Simple 进行 Archive，导出并解压 Test_ipa_Simple.ipa，进入 Test_ipa_Simple.app 文件夹:

![截屏2021-05-09 14.21.36.png](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/8f1d41b6bb554260abdf9017440e98ad~tplv-k3u1fbpfcp-watermark.image)

&emsp;下面我们可以通过 MachOView 来验证一下 Test_ipa_Simple.app 文件夹中的 Test_ipa_Simple 可执行文件中的动态库（WebKit 和 DYLID）的链接地址。（@rpth 表示的其实就是 .app 下的 Framework 文件夹。）

![截屏2021-05-09 14.47.22.png](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/89315ecbe30b4da4b1143391269bc6a7~tplv-k3u1fbpfcp-watermark.image)

![截屏2021-05-09 14.47.32.png](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/64864747351f417fba7af6b28920b532~tplv-k3u1fbpfcp-watermark.image)

&emsp;系统在加载动态库时，会检查 framework 的签名，签名中必须包含 Team Identifier 并且 framework 和 host app 的 Team Identifier 必须一致。可以使用 `codesign -dv Test_ipa_Simple.app` 和 `codesign -dv DYLIB.framework` 来进行验证。

+ .framework 为什么既是静态库又是动态库 ？

> &emsp;系统的 .framework 是动态库，我们自己建立的.framework 一般都是静态库。但是现在你用 xcode 创建 Framework 的时候默认是动态库（Mach-O Type 默认是 Dynamic Library），一般打包成 SDK 给别人用的话都使用的是静态库，可以修改 Build Settings 的 Mach-O Type 为 Static Library。

+ 什么是 framework ?

> &emsp;Framework 是 Cocoa/Cocoa Touch 程序中使用的一种资源打包方式，可以将代码文件、头文件、资源文件、说明文档等集中在一起，方便开发者使用。一般如果是静态 Framework 的话，资源打包进 Framework 是读取不了的。静态 Framework 和 .a 文件都是编译进可执行文件里面的。只有动态 Framework 能在 .app 下面的 Framework 文件夹下看到，并读取 .framework 里的资源文件。
>
> &emsp;Cocoa/Cocoa Touch 开发框架本身提供了大量的 Framework，比如 Foundation.framework / UIKit.framework / AppKit.framework 等。需要注意的是，这些 framework 无一例外都是动态库。
>
> &emsp;平时我们用的第三方 SDK 的 framework 都是静态库，真正的动态库是上不了 AppStore 的(iOS 8 之后能上 AppStore，因为有个 App Extension，需要动态库支持)。

&emsp;我们用 use_frameworks! 生成的 pod 里面，pods 这个 PROJECT 下面会为每一个 pod 生成一个 target，比如有一个 pod 叫做 AFNetworking，那么就会有一个叫 AFNetworking 的 target，最后这个 target 生成的就是 AFNetworking.framework。

### 关于 use_frameworks!

&emsp;在使用 CocoaPods 的时候在 Podfile 里加入 use_frameworks! ，那么在编译的时候就会默认生成动态库，我们能看到每个源码 Pod 都会在 Pods 工程下面生成一个对应的动态库 Framework 的 target，我们能在这个 target 的 Build Settings -> Mach-O Type 看到默认设置是 Dynamic Library，也就是会生成一个动态 Framework，我们能在 Products 下面看到每一个 Pod 对应生成的动态库。

![截屏2021-05-10 08.32.00.png](https://p9-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/7e4bca1afb7844c5b81bac837ab20687~tplv-k3u1fbpfcp-watermark.image)

&emsp;这些生成的动态库将链接到主项目给主工程使用，但是我们上面说过动态库需要在主工程 target 的 General -> Frameworks, Libraries, and Embedded Content 添加这个动态库并设置其 Embed 为 Embed & Sign 才能使用，而我们并没有在 Frameworks, Libraries, and Embedded Content 中看到这些动态库。那这是怎么回事呢，其实是 cocoapods 已经执行了脚本把这些动态库嵌入到了 .app 的 Framework 目录下，相当于在 Frameworks, Libraries, and Embedded Content 加入了这些动态库，我们能在主工程 target 的 Build Phase -> [CP]Embed Pods Frameworks 里看到执行的脚本。（"${PODS_ROOT}/Target Support Files/Pods-Test_ipa_Simple/Pods-Test_ipa_Simple-frameworks.sh"）

![截屏2021-05-10 08.22.43.png](https://p1-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/b3c70781d38645aba9736a352ce5b513~tplv-k3u1fbpfcp-watermark.image)

&emsp;所以 Pod 默认是生成动态库，然后嵌入到 .app 下面的 Framework 文件夹里。我们去 Pods 工程的 target 里把 Build Settings -> Mach-O Type 设置为 Static Library。那么生成的就是静态库，但是 cocoapods 也会把它嵌入到 .app 的 Framework 目录下，而因为它是静态库，所以会报错：unrecognized selector sent to instanceunrecognized selector sent to instance 。[iOS里的动态库和静态库](https://www.jianshu.com/p/42891fb90304)

## 一组函数的执行顺序

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

&emsp;根据控制台打印，可以看到 load 函数最先执行，然后是 constructor 属性修饰的 main_front 函数执行，然后是 main 函数执行，最后是 destructor 属性修饰的 main_back 函数执行。

&emsp;\_\_attribute__ 可以设置函数属性(Function Attribute)、变量属性(Variable Attribute)和类型属性(Type Attribute)。\_\_attribute__ 前后都有两个下划线，并且后面会紧跟一对原括弧，括弧里面是相应的 \_\_attribute__ 参数，\_\_attribute__ 语法格式为：`__attribute__((attribute-list))`。

&emsp;若函数被设定为 `constructor` 属性，则该函数会在 main 函数执行之前被自动的执行。类似的，若函数被设定为 `destructor` 属性，则该函数会在 main 函数执行之后或者 exit 被调用后被自动的执行。

&emsp;我们知道 .h、.m 的类在程序运行时先进行预编译，之后进行编译，编译完成后会进行汇编，在汇编结束后会进入一个阶段叫连接（把所有的代码链接到我们的程序中），最后会生成一个可执行文件。

&emsp;下面我们将了解 App 运行需要加载依赖库，需要加载 .h、.m 文件，那么谁来决定加载这些东西的先后顺序呢？这就是我们今天要说的主角 dyld（链接器）。就是由它来决定加载内容的先后顺序。

&emsp;app：images（镜像文件）-> dyld：读到内存（也就是加表里），启动主程序 - 进行 link - 一些必要对象的初始化（runtime，libsysteminit，OS_init 的初始化）。

### 探究 Dyld

&emsp;macOS 的 dyld 程序位置在 `/usr/lib/dyld`   

![截屏2021-05-12 08.08.33.png](https://p1-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/909ef5653e4c40479d3b43a437dcb9e7~tplv-k3u1fbpfcp-watermark.image)

```c++
hmc@bogon Simple % file dyld
dyld: Mach-O universal binary with 3 architectures: [x86_64:Mach-O 64-bit dynamic linker x86_64] [i386:Mach-O dynamic linker i386] [arm64e]
dyld (for architecture x86_64):    Mach-O 64-bit dynamic linker x86_64
dyld (for architecture i386):    Mach-O dynamic linker i386
dyld (for architecture arm64e):    Mach-O 64-bit dynamic linker arm64e
```

&emsp;dyld 是英文 the dynamic link editor 的简写，翻译过来就是动态链接器，是苹果操作系统的一个重要的组成部分。在 iOS/macOS 系统中，仅有很少量的进程只需要内核就能完成加载，基本上所有的进程都是动态链接的，所以 Mach-O 镜像文件中会有很多对外部的库和符号的引用，但是这些引用并不能直接用，在启动时还必须要通过这些引用进行内容的填补，这个填补工作就是由动态链接器 dyld 来完成的，也就是符号绑定。系统内核在加载 Mach-O 文件时，都需要用 dyld 链接程序，将程序加载到内存中。

















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


















## 参考链接
**参考链接:🔗**
+ [dyld-832.7.3](https://opensource.apple.com/tarballs/dyld/)
+ [OC底层原理之-App启动过程（dyld加载流程）](https://juejin.cn/post/6876773824491159565)
+ [iOS中的dyld缓存是什么？](https://blog.csdn.net/gaoyuqiang30/article/details/52536168)
+ [iOS进阶之底层原理-应用程序加载（dyld加载流程、类与分类的加载）](https://blog.csdn.net/hengsf123456/article/details/116205004?utm_medium=distribute.pc_relevant.none-task-blog-baidujs_title-4&spm=1001.2101.3001.4242)
+ [iOS里的动态库和静态库](https://www.jianshu.com/p/42891fb90304)
+ [Xcode 中的链接路径问题](https://www.jianshu.com/p/cd614e080078)
+ [iOS 利用 Framework 进行动态更新](https://nixwang.com/2015/11/09/ios-dynamic-update/)

