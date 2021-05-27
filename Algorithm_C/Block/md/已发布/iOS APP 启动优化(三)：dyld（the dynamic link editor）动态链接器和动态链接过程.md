# iOS APP 启动优化(三)：dyld（the dynamic link editor）动态链接器和动态链接过程

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

&emsp;动态库和静态的知识我们就延伸到这里吧，下面我们继续学习 **链接器** 相关的内容。

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

&emsp;我们知道 .h、.m 的类在程序运行时先进行预编译，之后进行编译，编译完成后会进行汇编，在汇编结束后会进入一个阶段叫链接（把所有的代码链接到我们的程序中），最后会生成一个可执行文件。

&emsp;下面我们将了解 App 运行需要加载依赖库，需要加载 .h、.m 文件，那么谁来决定加载这些东西的先后顺序呢？这就是我们今天要说的主角 dyld（链接器）。就是由它来决定加载内容的先后顺序。

&emsp;app：images（镜像文件）-> dyld：读到内存（也就是加表里），启动主程序 - 进行 link - 一些必要对象的初始化（runtime，libsysteminit，OS_init 的初始化）。

&emsp;下面我们的目光聚焦在两个点上：链接器本身和链接过程的解读。

### Dyld 探索

&emsp;macOS 的 dyld 程序位置在 `/usr/lib/dyld`   

![截屏2021-05-12 08.08.33.png](https://p1-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/909ef5653e4c40479d3b43a437dcb9e7~tplv-k3u1fbpfcp-watermark.image)

```c++
hmc@bogon Simple % file dyld
dyld: Mach-O universal binary with 3 architectures: [x86_64:Mach-O 64-bit dynamic linker x86_64] [i386:Mach-O dynamic linker i386] [arm64e]
dyld (for architecture x86_64):    Mach-O 64-bit dynamic linker x86_64
dyld (for architecture i386):    Mach-O dynamic linker i386
dyld (for architecture arm64e):    Mach-O 64-bit dynamic linker arm64e
```

&emsp;可以看到我电脑里面的 dyld 是一个 fat Mach-O 文件，同时集合了三个平台 x86_64、i386、arm64e。 

&emsp;dyld 是英文 the dynamic link editor 的简写，翻译过来就是动态链接器，是苹果操作系统的一个重要的组成部分。在 iOS/macOS 系统中，仅有很少量的进程只需要内核就能完成加载，基本上所有的进程都是动态链接的，所以 Mach-O 镜像文件中会有很多对外部的库和符号的引用，但是这些引用并不能直接用，在启动时还必须要通过这些引用进行内容的填补，这个填补工作就是由动态链接器 dyld 来完成的，也就是符号绑定。系统内核在加载 Mach-O 文件时，都需要用 dyld 链接程序，将程序加载到内存中。

&emsp;在编写项目时，我们大概最先接触到的可执行的代码是 main 和 load 函数，当我们不重写某个类的 load 函数时，大概会觉得 main 是我们 APP 的入口函数，当我们重写了某个类的 load 函数后，我们又已知的 load 函数是在 main 之前执行的。（上一小节我们也有说过 \_\_attribute__((constructor)) 修饰的 C  函数也会在 main 之前执行）那么从这里可以看出到我们的 APP 真的执行到 main 函数之前其实已经做了一些 APP 的 加载操作，那具体都有哪些呢，我们可以在 load 函数中打断点，然后打印出函数调用堆栈来发现一些端倪。如下图所示：

&emsp;在模拟器下的截图，其中的 sim 表示当前是在 TARGET_OS_SIMULATOR 环境下：

![截屏2021-05-13 08.11.38.png](https://p1-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/89b62441b6d646b39966c7e2bf52abdb~tplv-k3u1fbpfcp-watermark.image)

```c++
(lldb) bt
* thread #1, queue = 'com.apple.main-thread', stop reason = breakpoint 1.1
    frame #0: 0x0000000100a769c7 Test_ipa_Simple`+[ViewController load](self=ViewController, _cmd="load") at ViewController.m:17:5
    frame #1: 0x00007fff201804e3 libobjc.A.dylib`load_images + 1442
    frame #2: 0x0000000108cb5e54 dyld_sim`dyld::notifySingle(dyld_image_states, ImageLoader const*, ImageLoader::InitializerTimingList*) + 425
    frame #3: 0x0000000108cc4887 dyld_sim`ImageLoader::recursiveInitialization(ImageLoader::LinkContext const&, unsigned int, char const*, ImageLoader::InitializerTimingList&, ImageLoader::UninitedUpwards&) + 437
    frame #4: 0x0000000108cc2bb0 dyld_sim`ImageLoader::processInitializers(ImageLoader::LinkContext const&, unsigned int, ImageLoader::InitializerTimingList&, ImageLoader::UninitedUpwards&) + 188
    frame #5: 0x0000000108cc2c50 dyld_sim`ImageLoader::runInitializers(ImageLoader::LinkContext const&, ImageLoader::InitializerTimingList&) + 82
    frame #6: 0x0000000108cb62a9 dyld_sim`dyld::initializeMainExecutable() + 199
    frame #7: 0x0000000108cbad50 dyld_sim`dyld::_main(macho_header const*, unsigned long, int, char const**, char const**, char const**, unsigned long*) + 4431
    frame #8: 0x0000000108cb51c7 dyld_sim`start_sim + 122
    frame #9: 0x0000000200dea57a dyld`dyld::useSimulatorDyld(int, macho_header const*, char const*, int, char const**, char const**, char const**, unsigned long*, unsigned long*) + 2093
    frame #10: 0x0000000200de7df3 dyld`dyld::_main(macho_header const*, unsigned long, int, char const**, char const**, char const**, unsigned long*) + 1199
    frame #11: 0x0000000200de222b dyld`dyldbootstrap::start(dyld3::MachOLoaded const*, int, char const**, dyld3::MachOLoaded const*, unsigned long*) + 457
  * frame #12: 0x0000000200de2025 dyld`_dyld_start + 37
(lldb) 
```

&emsp;在真机下的截图，相比较与模拟器环境看到是少了 dyld\`dyld::useSimulatorDyld 和 dyld_sim\`start_sim 调用（切换到模拟器环境），后序的函数调用基本都是一样的，除了运行环境不同外（dyld_sim / dyld）。 

![截屏2021-05-15 08.06.39.png](https://p1-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/297c64db8ca44b99999fbd81146e4c3e~tplv-k3u1fbpfcp-watermark.image)

```c++
(lldb) bt
* thread #1, queue = 'com.apple.main-thread', stop reason = breakpoint 1.1
  * frame #0: 0x00000001043f19c0 Test_ipa_Simple`+[ViewController load](self=ViewController, _cmd="load") at ViewController.m:17:5
    frame #1: 0x00000001a2bc925c libobjc.A.dylib`load_images + 944
    frame #2: 0x00000001046ea21c dyld`dyld::notifySingle(dyld_image_states, ImageLoader const*, ImageLoader::InitializerTimingList*) + 464
    frame #3: 0x00000001046fb5e8 dyld`ImageLoader::recursiveInitialization(ImageLoader::LinkContext const&, unsigned int, char const*, ImageLoader::InitializerTimingList&, ImageLoader::UninitedUpwards&) + 512
    frame #4: 0x00000001046f9878 dyld`ImageLoader::processInitializers(ImageLoader::LinkContext const&, unsigned int, ImageLoader::InitializerTimingList&, ImageLoader::UninitedUpwards&) + 184
    frame #5: 0x00000001046f9940 dyld`ImageLoader::runInitializers(ImageLoader::LinkContext const&, ImageLoader::InitializerTimingList&) + 92
    frame #6: 0x00000001046ea6d8 dyld`dyld::initializeMainExecutable() + 216
    frame #7: 0x00000001046ef928 dyld`dyld::_main(macho_header const*, unsigned long, int, char const**, char const**, char const**, unsigned long*) + 5216
    frame #8: 0x00000001046e9208 dyld`dyldbootstrap::start(dyld3::MachOLoaded const*, int, char const**, dyld3::MachOLoaded const*, unsigned long*) + 396
    frame #9: 0x00000001046e9038 dyld`_dyld_start + 56
(lldb) 
```

&emsp;可以看到从 \_dyld_start 函数开始直到 +[ViewController load] 函数，中间的函数调用栈都集中在了 dyld/dyld_sim。（最后的 libobjc.A.dylib`load_images 调用，后面我们会详细分析）下面我们可以通过 [dyld 的源码](https://opensource.apple.com/tarballs/dyld/) 来一一分析上面函数调用堆栈中出现的函数。

&emsp;\_dyld_start 是汇编函数，这里我们只看 \_\_arm64__ && !TARGET_OS_SIMULATOR 平台下的。

```c++
#if __arm64__ && !TARGET_OS_SIMULATOR
    .text
    .align 2
    .globl __dyld_start
__dyld_start:
    mov     x28, sp
    and     sp, x28, #~15        // force 16-byte alignment of stack
    mov    x0, #0
    mov    x1, #0
    stp    x1, x0, [sp, #-16]!    // make aligned terminating frame
    mov    fp, sp            // set up fp to point to terminating frame
    sub    sp, sp, #16             // make room for local variables
    
#if __LP64__
    ldr     x0, [x28]               // get app's mh into x0
    ldr     x1, [x28, #8]           // get argc into x1 (kernel passes 32-bit int argc as 64-bits on stack to keep alignment)
    add     x2, x28, #16            // get argv into x2
#else
    ldr     w0, [x28]               // get app's mh into x0
    ldr     w1, [x28, #4]           // get argc into x1 (kernel passes 32-bit int argc as 64-bits on stack to keep alignment)
    add     w2, w28, #8             // get argv into x2
#endif

    adrp    x3,___dso_handle@page
    add     x3,x3,___dso_handle@pageoff // get dyld's mh in to x4
    mov    x4,sp                   // x5 has &startGlue

    // call dyldbootstrap::start(app_mh, argc, argv, dyld_mh, &startGlue) 
    bl    __ZN13dyldbootstrap5startEPKN5dyld311MachOLoadedEiPPKcS3_Pm
    mov    x16,x0                  // save entry point address in x16
    
#if __LP64__
    ldr     x1, [sp]
#else
    ldr     w1, [sp]
#endif

    cmp    x1, #0
    b.ne    Lnew

    // LC_UNIXTHREAD way, clean up stack and jump to result
#if __LP64__
    add    sp, x28, #8             // restore unaligned stack pointer without app mh
#else
    add    sp, x28, #4             // restore unaligned stack pointer without app mh
#endif

#if __arm64e__
    braaz   x16                     // jump to the program's entry point
#else
    br      x16                     // jump to the program's entry point
#endif

    // LC_MAIN case, set up stack for call to main()
Lnew:    mov    lr, x1            // simulate return address into _start in libdyld.dylib

#if __LP64__
    ldr    x0, [x28, #8]       // main param1 = argc
    add    x1, x28, #16        // main param2 = argv
    add    x2, x1, x0, lsl #3
    add    x2, x2, #8          // main param3 = &env[0]
    mov    x3, x2
Lapple:    ldr    x4, [x3]
    add    x3, x3, #8
#else
    ldr    w0, [x28, #4]       // main param1 = argc
    add    x1, x28, #8         // main param2 = argv
    add    x2, x1, x0, lsl #2
    add    x2, x2, #4          // main param3 = &env[0]
    mov    x3, x2
Lapple:    ldr    w4, [x3]
    add    x3, x3, #4
#endif

    cmp    x4, #0
    b.ne    Lapple            // main param4 = apple
    
#if __arm64e__
    braaz   x16
#else
    br      x16
#endif

#endif // __arm64__ && !TARGET_OS_SIMULATOR
```

&emsp;然后看到汇编函数 \_\_dyld_start 内部调用了 dyldbootstrap::start(app_mh, argc, argv, dyld_mh, &startGlue) 函数，即 dyldbootstrap 命名空间中的 start 函数，namespace dyldbootstrap 定义在 dyldInitialization.cpp 中，从其名字中我们已经能猜到一些它的作用：用来进行 dyld 的初始化，将 dyld 引导到可运行状态的。下面我们一起看下其中的 start 的函数。

```c++
//
//  This is code to bootstrap dyld.  This work in normally done for a program by dyld and crt.
//  In dyld we have to do this manually.
//
uintptr_t start(const dyld3::MachOLoaded* appsMachHeader, int argc, const char* argv[],
                const dyld3::MachOLoaded* dyldsMachHeader, uintptr_t* startGlue)
{

    // Emit kdebug tracepoint to indicate dyld bootstrap has started <rdar://46878536>
    // 发出 kdebug tracepoint 以指示 dyld bootstrap 已启动
    dyld3::kdebug_trace_dyld_marker(DBG_DYLD_TIMING_BOOTSTRAP_START, 0, 0, 0, 0);

    // if kernel had to slide dyld, we need to fix up load sensitive locations
    // we have to do this before using any global variables
    rebaseDyld(dyldsMachHeader); // 用于重定位（设置虚拟地址偏移，这里的偏移主要用于重定向）

    // kernel sets up env pointer to be just past end of agv array
    // 内核将 env 指针设置为刚好超出 agv 数组的末尾
    const char** envp = &argv[argc+1];
    
    // kernel sets up apple pointer to be just past end of envp array
    // 内核将 apple 指针设置为刚好超出 envp 数组的末尾
    const char** apple = envp;
    while(*apple != NULL) { ++apple; }
    ++apple;

    // set up random value for stack canary
    // 为 stack canary 设置随机值
    __guard_setup(apple);

#if DYLD_INITIALIZER_SUPPORT // 前面 DYLD_INITIALIZER_SUPPORT 宏的值是 0，所以这里 #if 内部的内容并不会执行 
    // run all C++ initializers inside dyld
    // 在 dyld 中运行所有 C++ 初始化器
    runDyldInitializers(argc, argv, envp, apple);
#endif

    _subsystem_init(apple);

    // now that we are done bootstrapping dyld, call dyld's main
    // 现在我们完成了 bootstrapping dyld，调用 dyld 的 main（进入 dyld 的主函数）
    uintptr_t appsSlide = appsMachHeader->getSlide();
    return dyld::_main((macho_header*)appsMachHeader, appsSlide, argc, argv, envp, apple, startGlue);
}
```

&emsp;appsMachHeader 和 dyldsMachHeader 两个参数的类型是 const dyld3::MachOLoaded*，在 dyld/dyld3/MachOLoaded.h 文件中可看到命名空间 dyld3 中定义的 struct VIS_HIDDEN MachOLoaded : public MachOFile，MachOLoaded 结构体公开继承自 MachOFile 结构体，在 dyld/dyld3/MachOFile.h 文件中可看到命名空间 dyld3 中定义的 struct VIS_HIDDEN MachOFile : mach_header，MachOFile 结构体继承自 mach_header 结构体。在 dyld/src/ImageLoader.h 中可看到在 \_\_LP64__ 下 macho_header 公开继承自 mach_header_64 其他平台则是继承自 mach_header（它们的名字仅差一个 0），mach_header 在前一篇 《iOS APP 启动优化(一)：ipa(iPhone application archive) 包和 Mach-O(Mach Object file format) 概述》中我们有详细分析过：

```c++
#if __LP64__
    struct macho_header                : public mach_header_64  {};
    struct macho_nlist                : public nlist_64  {};    
#else
    struct macho_header                : public mach_header  {};
    struct macho_nlist                : public nlist  {};    
#endif
```

> &emsp;Mach-O 文件的 Header 部分对应的数据结构定义在 darwin-xnu/EXTERNAL_HEADERS/mach-o/loader.h 中，struct mach_header 和 struct mach_header_64 分别对应 32-bit architectures 和 64-bit architectures。（对于 32/64-bit architectures，32/64 位的 mach header 都出现在 Mach-O 文件的最开头。）

```c++
struct mach_header_64 {
    uint32_t    magic;        /* mach magic number identifier */
    cpu_type_t    cputype;    /* cpu specifier */
    cpu_subtype_t    cpusubtype;    /* machine specifier */
    uint32_t    filetype;    /* type of file */
    uint32_t    ncmds;        /* number of load commands */
    uint32_t    sizeofcmds;    /* the size of all the load commands */
    uint32_t    flags;        /* flags */
    uint32_t    reserved;    /* reserved */
};
```

&emsp;综上，MachOLoaded -> MachOFile -> mach_header。MachOFile 继承 mach_header 使其拥有 mach_header 结构体中所有的成员变量，然后 MachOFile 定义中则声明了一大组针对 Mach-O 的 Header 的函数，例如架构名、CPU 类型等。MachOLoaded 继承自 MachOFile 其定义中则声明了一组加载 Mach-O 的 Header 的函数。 

&emsp;下面我们接着看 dyld::_main 函数。首先是根据函数调用方式可以看到 \_main 函数是属于 dyld 命名空间的，在 dyld/src/dyld2.cpp 中可看到 namespace dyld 的定义，在 dyld2.h 和 dyld2.cpp 中可看到分别进行了 `uintptr_t _main(const macho_header* mainExecutableMH, uintptr_t mainExecutableSlide, int argc, const char* argv[], const char* envp[], const char* apple[], uintptr_t* startGlue)` 的声明和定义。

&emsp;首先是 \_main 函数的注释：dyld 的入口点。内核加载 dyld 并跳到 \_\_dyld_start，设置一些寄存器并调用此函数。返回目标程序中的 main() 地址，\_\_dyld_start 跳到该地址。

&emsp;下面我们沿着 \_main 函数的定义，来分析 \_main 函数，并对必要的代码段进行摘录。

&emsp;调用 `getHostInfo(mainExecutableMH, mainExecutableSlide);` 函数来获取 Mach-O 头部信息中的当前运行架构信息，仅是为了给 sHostCPU 和 sHostCPUsubtype 两个全局变量赋值。getHostInfo 函数虽然有两个参数  mainExecutableMH 和 mainExecutableSlide 但是实际都只是为了在 \_\_x86_64__ && !TARGET_OS_SIMULATOR 下使用的。

```c++
static void getHostInfo(const macho_header* mainExecutableMH, uintptr_t mainExecutableSlide)
{
#if CPU_SUBTYPES_SUPPORTED
#if __ARM_ARCH_7K__
    sHostCPU        = CPU_TYPE_ARM;
    sHostCPUsubtype = CPU_SUBTYPE_ARM_V7K;
#elif __ARM_ARCH_7A__
    sHostCPU        = CPU_TYPE_ARM;
    sHostCPUsubtype = CPU_SUBTYPE_ARM_V7;
#elif __ARM_ARCH_6K__
    sHostCPU        = CPU_TYPE_ARM;
    sHostCPUsubtype = CPU_SUBTYPE_ARM_V6;
#elif __ARM_ARCH_7F__
    sHostCPU        = CPU_TYPE_ARM;
    sHostCPUsubtype = CPU_SUBTYPE_ARM_V7F;
#elif __ARM_ARCH_7S__
    sHostCPU        = CPU_TYPE_ARM;
    sHostCPUsubtype = CPU_SUBTYPE_ARM_V7S;
#elif __ARM64_ARCH_8_32__
    sHostCPU        = CPU_TYPE_ARM64_32;
    sHostCPUsubtype = CPU_SUBTYPE_ARM64_32_V8;
#elif __arm64e__
    sHostCPU        = CPU_TYPE_ARM64;
    sHostCPUsubtype = CPU_SUBTYPE_ARM64E;
#elif __arm64__
    sHostCPU        = CPU_TYPE_ARM64;
    sHostCPUsubtype = CPU_SUBTYPE_ARM64_V8;
#else
    struct host_basic_info info;
    mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
    mach_port_t hostPort = mach_host_self();
    kern_return_t result = host_info(hostPort, HOST_BASIC_INFO, (host_info_t)&info, &count);
    if ( result != KERN_SUCCESS )
        throw "host_info() failed";
    sHostCPU        = info.cpu_type;
    sHostCPUsubtype = info.cpu_subtype;
    mach_port_deallocate(mach_task_self(), hostPort);
  #if __x86_64__
      // host_info returns CPU_TYPE_I386 even for x86_64.  Override that here so that
      // we don't need to mask the cpu type later.
      sHostCPU = CPU_TYPE_X86_64;
    #if !TARGET_OS_SIMULATOR
      sHaswell = (sHostCPUsubtype == CPU_SUBTYPE_X86_64_H);
      // <rdar://problem/18528074> x86_64h: Fall back to the x86_64 slice if an app requires GC.
      if ( sHaswell ) {
        if ( isGCProgram(mainExecutableMH, mainExecutableSlide) ) {
            // When running a GC program on a haswell machine, don't use and 'h slices
            sHostCPUsubtype = CPU_SUBTYPE_X86_64_ALL;
            sHaswell = false;
            gLinkContext.sharedRegionMode = ImageLoader::kDontUseSharedRegion;
        }
      }
    #endif
  #endif
#endif
#endif
}
```

&emsp;这里对 main Mach-O 文件进行实例化。

```c++
// instantiate ImageLoader for main executable
sMainExecutable = instantiateFromLoadedImage(mainExecutableMH, mainExecutableSlide, sExecPath);
gLinkContext.mainExecutable = sMainExecutable;
gLinkContext.mainExecutableCodeSigned = hasCodeSignatureLoadCommand(mainExecutableMH);
```

&emsp;instantiateFromLoadedImage 函数返回值是一个 ImageLoaderMachO 指针，`class ImageLoaderMachO : public ImageLoader` ImageLoaderMachO 类公开继承自 ImageLoader 类。ImageLoader 是一个抽象基类。为了支持加载特定的可执行文件格式，可以创建 ImageLoader 的一个具体子类。对于使用中的每个可执行文件（dynamic shared object），将实例化一个 ImageLoader。ImageLoader 基类负责将 images 链接在一起，但它对任何特定的文件格式一无所知。 ImageLoaderMachO 是 ImageLoader 的子类，可加载 mach-o 格式的文件。

&emsp;下面看一下 instantiateFromLoadedImage 函数实现，它内部直接调用 ImageLoaderMachO 的 instantiateMainExecutable 函数进行可执行文件的加载。对于程序中需要的依赖库、插入库，会创建一个对应的 image 对象，对这些 image 进行链接，调用各 image 的初始化方法等等，包括对 runtime 的初始化。

```c++
// The kernel maps in main executable before dyld gets control.  We need to 
// make an ImageLoader* for the already mapped in main executable.
static ImageLoaderMachO* instantiateFromLoadedImage(const macho_header* mh, uintptr_t slide, const char* path)
{
    // try mach-o loader
    // isCompatibleMachO 是检查 mach-o 的 subtype 是否支持当前的 cpu 
//    if ( isCompatibleMachO((const uint8_t*)mh, path) ) {
        ImageLoader* image = ImageLoaderMachO::instantiateMainExecutable(mh, slide, path, gLinkContext);
        
        // 将 image 加载到 imagelist 中，所以我们在 xcode 中使用 image list 命令查看的第一个便是我们的 mach-o
        addImage(image);
        
        return (ImageLoaderMachO*)image;
//    }
    
//    throw "main executable not a known format";
}
```



&emsp;加载共享缓存库

```c++
// load shared cache
checkSharedRegionDisable((dyld3::MachOLoaded*)mainExecutableMH, mainExecutableSlide);
```




```c++
if ( sEnv.DYLD_PRINT_OPTS )
    printOptions(argv);
if ( sEnv.DYLD_PRINT_ENV ) 
    printEnvironmentVariables(envp);
```

&emsp;此处是判断是否设置了环境变量，如果设置了，那么 xcode 就会在控制台打印相关的详细信息。（在 Edit Scheme... -> Run -> Arguments -> Environment Variables 进行添加） 

&emsp;当添加了 DYLD_PRINT_OPTS 时，会在控制台输出可执行文件的位置。
```c++
opt[0] = "/Users/hmc/Library/Developer/CoreSimulator/Devices/4E072E27-E586-4E81-A693-A02A3ED83DEC/data/Containers/Bundle/Application/ECDA091A-1610-49D2-8BC0-B41A58BC76EC/Test_ipa_Simple.app/Test_ipa_Simple"
```

&emsp;当添加了 DYLD_PRINT_ENV 时，会在控制台输出用户级别、插入的动态库、动态库的路径、模拟器的信息等等一系列的信息，由于内容过多这里就粘贴出来了。

































































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

&emsp;clang:Clang 是一个 C++ 编写、基于 LLVM、发布于 LLVM BSD 许可证下的 C/C++/Objective-C/Objective-C++ 编译器。它与 GNU C 语言规范几乎完全兼容（当然，也有部分不兼容的内容， 包括编译命令选项也会有点差异），并在此基础上增加了额外的语法特性，比如 C 函数重载（通过 \_ attribute_((overloadable)) 来修饰函数)，其目标(之一)就是超越 GCC。

## 参考链接
**参考链接:🔗**
+ [dyld-832.7.3](https://opensource.apple.com/tarballs/dyld/)
+ [OC底层原理之-App启动过程（dyld加载流程）](https://juejin.cn/post/6876773824491159565)
+ [iOS中的dyld缓存是什么？](https://blog.csdn.net/gaoyuqiang30/article/details/52536168)
+ [iOS进阶之底层原理-应用程序加载（dyld加载流程、类与分类的加载）](https://blog.csdn.net/hengsf123456/article/details/116205004?utm_medium=distribute.pc_relevant.none-task-blog-baidujs_title-4&spm=1001.2101.3001.4242)
+ [iOS应用程序在进入main函数前做了什么？](https://www.jianshu.com/p/73d63220d4f1)
+ [dyld加载应用启动原理详解](https://www.jianshu.com/p/1b9ca38b8b9f)
+ [iOS里的动态库和静态库](https://www.jianshu.com/p/42891fb90304)
+ [Xcode 中的链接路径问题](https://www.jianshu.com/p/cd614e080078)
+ [iOS 利用 Framework 进行动态更新](https://nixwang.com/2015/11/09/ios-dynamic-update/)
+ [命名空间namespace ，以及重复定义的问题解析](https://blog.csdn.net/u014357799/article/details/79121340)
+ [C++ 命名空间namespace](https://www.jianshu.com/p/30e960717ef1)
+ [一文了解 Xcode 生成「静态库」和「动态库」 的流程](https://mp.weixin.qq.com/s/WH8emrMpLeVW-LfGwN09cw)

