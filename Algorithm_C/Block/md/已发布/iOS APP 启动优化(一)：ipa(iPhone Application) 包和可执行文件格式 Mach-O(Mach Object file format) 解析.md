# iOS APP 启动优化(一)：ipa(iPhone application archive) 包和可执行文件格式 Mach-O(Mach Object file format) 解析

> IPA 后缀的文件是 iOS 系统的软件包，全称为 iPhone application archive。通常情况下，IPA 文件都是使用苹果公司的 FairPlayDRM 技术进行加密保护的。每个 IPA 文件都是 ARM 架构的可执行文件以及该应用的资源文件的打包文件，只能安装在 iPhone、iPod Touch、iPad 以及使用 Apple Silicon 平台的 Mac 上。该文件可以通过修改后缀名为 zip 后，进行解压缩，查看其软件包中的内容。[IPA文件-维基百科](https://zh.wikipedia.org/wiki/IPA文件)
> 
> 数字版权管理（英语：Digital rights management，缩写为 DRM）是一系列访问控制技术，通常用于控制数字内容和设备在被销售之后的使用过程。DRM 有时也称为拷贝保护、复制控制、技术保护措施等，但这些称呼存在争议。许多数字出版社和软件厂商都使用了 DRM，例如亚马逊、AT&T、AOL、Apple Inc.、Netflix、Google[7]、BBC、微软、Sony、Valve Corporation 等。[数字版权管理-维基百科](https://zh.wikipedia.org/wiki/数字版权管理)

## 解压 .ipa 文件查看其内容并引出 Mach-O 格式
&emsp;相信每一位 iOS 开发者都进行过打包测试，当我们把 Ad Hoc 或者 App Store Connect 的包导出到本地时会看到一个 xxx.ipa 文件，ipa 是 iPhone Application 的缩写。实际上 xxx.ipa 只是一个变相的 zip 压缩包，我们可以把 xxx.ipa 文件直接通过 unzip 命令进行解压。

&emsp;我们直接新建一个命名为 Test_ipa_Simple 的空白 iOS App，直接进行 Archive 后并导出 Test_ipa_Simple.ipa 文件查看它的内部结构。在终端执行 unzip Test_ipa_Simple.ipa 解压之后，会有一个 Payload 目录，而 Payload 里则是一个看似是文件的 Test_ipa_Simple.app，而实际上它又是一个目录，或者说是一个完整的 App Bundle。其中 Base.lproj 中是我们的 Main.storyboard 和 LaunchScreen.storyboard 的内容，然后是 embedded.mobileprovision（描述文件）和 PkgInfo、Info.plist、_CodeSignature 用于描述 App 的一些信息，然后我们要重点关注的便是当前这个目录里面体积最大的文件 Test_ipa_Simple，它是和我们的 ipa 包同名的一个二进制文件，然后用 file 命令查看他的文件类型是一个在 arm64 处理器架构下的可执行（executable）文件，格式则是 Mach-O。（下面是终端执行记录，可大致浏览一下）

```c++
hmc@bogon Test_ipa_Simple 2021-04-09 08-10-25 % unzip Test_ipa_Simple.ipa 
Archive:  Test_ipa_Simple.ipa
   creating: Payload/
   creating: Payload/Test_ipa_Simple.app/
   creating: Payload/Test_ipa_Simple.app/_CodeSignature/
  inflating: Payload/Test_ipa_Simple.app/_CodeSignature/CodeResources  
  inflating: Payload/Test_ipa_Simple.app/Test_ipa_Simple  
   creating: Payload/Test_ipa_Simple.app/Base.lproj/
   creating: Payload/Test_ipa_Simple.app/Base.lproj/Main.storyboardc/
  inflating: Payload/Test_ipa_Simple.app/Base.lproj/Main.storyboardc/UIViewController-BYZ-38-t0r.nib  
  inflating: Payload/Test_ipa_Simple.app/Base.lproj/Main.storyboardc/BYZ-38-t0r-view-8bC-Xf-vdC.nib  
  inflating: Payload/Test_ipa_Simple.app/Base.lproj/Main.storyboardc/Info.plist  
   creating: Payload/Test_ipa_Simple.app/Base.lproj/LaunchScreen.storyboardc/
  inflating: Payload/Test_ipa_Simple.app/Base.lproj/LaunchScreen.storyboardc/01J-lp-oVM-view-Ze5-6b-2t3.nib  
  inflating: Payload/Test_ipa_Simple.app/Base.lproj/LaunchScreen.storyboardc/UIViewController-01J-lp-oVM.nib  
  inflating: Payload/Test_ipa_Simple.app/Base.lproj/LaunchScreen.storyboardc/Info.plist  
  inflating: Payload/Test_ipa_Simple.app/embedded.mobileprovision  
  inflating: Payload/Test_ipa_Simple.app/Info.plist  
  inflating: Payload/Test_ipa_Simple.app/PkgInfo  
hmc@bogon Test_ipa_Simple 2021-04-09 08-10-25 % cd Payload 
hmc@bogon Payload % ls
Test_ipa_Simple.app
hmc@bogon Payload % cd Test_ipa_Simple.app 
hmc@bogon Test_ipa_Simple.app % ls -lht
total 240
drwxr-xr-x  4 hmc  staff   128B  4  9 08:10 Base.lproj
-rw-r--r--@ 1 hmc  staff   3.0K  4  9 08:10 Info.plist
-rw-r--r--  1 hmc  staff     8B  4  9 08:10 PkgInfo
-rwxr-xr-x  1 hmc  staff    86K  4  9 08:10 Test_ipa_Simple
drwxr-xr-x  3 hmc  staff    96B  4  9 08:10 _CodeSignature
-rw-r--r--  1 hmc  staff    20K  4  9 08:10 embedded.mobileprovision
hmc@bogon Test_ipa_Simple.app % file Test_ipa_Simple 
Test_ipa_Simple: Mach-O 64-bit executable arm64
```

## Mach-O 格式概述
> Mach-O 为 Mach Object 文件格式的缩写，它是一种用于可执行档、目标代码、动态库、内核转储的文件格式。作为 a.out 格式的替代者，Mach-O 提供了更强的扩展性，并提升了符号表中信息的访问速度。
Mach-O 曾经为大部分基于 Mach 核心的操作系统所使用。NeXTSTEP、Darwin 和 Mac OS X 等系统使用这种格式作为其原生可执行档、库和目标代码的格式。而同样使用 GNU Mach 作为其微内核的 GNU Hurd 系统则使用 ELF 而非 Mach-O 作为其标准的二进制文件格式。[Mach-O-维基百科](https://zh.wikipedia.org/wiki/Mach-O)

&emsp;在 [Code Size Performance Guidelines](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/CodeFootprint/CodeFootprint.html#//apple_ref/doc/uid/10000149-SW1) 文档中的 [Overview of the Mach-O Executable Format](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/CodeFootprint/Articles/MachOOverview.html#//apple_ref/doc/uid/20001860-BAJGJEJC) 章节提到了 Mach-O 格式，并描述了如何组织 Mach-O executable format 来提高代码的效率，下面我们先看下这一节的原文。

&emsp;Mach-O 是 OS X 中二进制文件的 native 可执行格式，是 shipping code 的首选格式。可执行格式决定二进制文件中的代码（code）和数据（data）读入内存的顺序。代码和数据的顺序会影响内存使用和分页活动（paging activity），因此会直接影响程序的性能。

&emsp;Mach-O 二进制文件被组织成段（segments）。每个段包含一个或多个 sections。不同类型的代码或数据进入每个 section。Segments 总是从页（page）边界开始，但 sections 不一定是页对齐的（page-aligned）。Segment 的大小由它包含的所有 sections 中的字节数来度量，并向上舍入到下一个虚拟内存页的边界（virtual memory page boundary）。因此，一个 segment 总是 4096 字节或 4 KB 的倍数，其中 4096 字节是最小大小。

&emsp;Mach-O 可执行文件的 segments 和 sections 根据其预期用途命名。Segment 名称的约定是使用前有双下划线的所有大写字母组成（例如：\_\_TEXT）；Section 名称的约定是使用前有双下划线的所有小写字母组成（例如：\_\_text）。

&emsp;Mach-O 可执行文件中有几个可能的 segments，但是只有两个与性能有关：\_\_TEXT segment 和 \_\_DATA segment。

+ The \_\_TEXT Segment: Read Only

&emsp;\_\_TEXT segment 是一个只读区域，包含可执行代码和常量数据。按照惯例，编译器工具创建的每个可执行文件至少有一个只读 \_\_TEXT segment。由于该 segment 是只读的，内核可以将可执行文件中的 \_\_TEXT segment 直接映射（map）到内存中一次。当 segment 映射到内存中时，它可以在对其内容感兴趣的所有进程之间共享。（这主要是 frameworks 和 shared libraries 的情况。）只读属性还意味着组成 \_\_TEXT segment 的页不必保存到备份存储。如果内核需要释放物理内存，它可以丢弃一个或多个 \_\_TEXT 页，并在需要时从磁盘重新读取它们。

&emsp;表 1 列出了可以出现在 \_\_TEXT segment 中的一些更重要的 sections。有关 segments 的完整列表，请参阅 Mach-O Runtime Architecture。

&emsp;Table 1  Major sections in the __TEXT segment

| Section | Description |
| --- | --- |
| \_\_text | The compiled machine code for the executable（可执行文件的已编译机器码） |
| \_\_const | The general constant data for the executable（可执行文件的常规常量数据） |
| \_\_cstring | Literal string constants (quoted strings in source code) 字面量字符串常量（源代码中带引号的字符串） |
| \_\_picsymbol_stub | Position-independent code stub routines used by the dynamic linker (dyld) 动态链接器（dyld）使用的与位置无关的 code stub routines |

+ The __DATA Segment: Read/Write

&emsp;\_\_DATA segment 包含可执行文件的非常量数据。此 segment 既可读又可写。因为它是可写的，所以 framework 或其他 shared library 的 \_\_DATA segment 在逻辑上是为每个与 library 链接的进程复制的。当内存页可读写时，内核将它们标记为 copy-on-write。此技术延迟复制页（page），直到共享该页的某个进程尝试写入该页。当发生这种情况时，内核会为该进程创建一个页（page）的私有副本。

&emsp;\_\_DATA segment 有许多 sections，其中一些 sections 仅由动态链接器（dynamic linker）使用。表 2 列出了 \_\_DATA segment 中可能出现的一些更重要的 sections。有关 segments 的完整列表，请参阅 Mach-O Runtime Architecture。

&emsp;Table 2  Major sections of the __DATA segment

| Section | Description |
| --- | --- |
| \_\_data | Initialized global variables (for example int a = 1; or static int a = 1;). 初始化的全局变量 |
| \_\_const | Constant data needing relocation (for example, char * const p = "foo";). 需要重定位的常量数据 |
| \_\_bss | Uninitialized static variables (for example, static int a;). 未初始化的静态变量 |
| \_\_common | Uninitialized external globals (for example, int a; outside function blocks). 未初始化的外部全局变量 |
| \_\_dyld | A placeholder section, used by the dynamic linker. 动态链接器使用的占位符部分 |
| \_\_la_symbol_ptr | “Lazy” symbol pointers. Symbol pointers for each undefined function called by the executable. “Lazy” 符号指针。可执行文件调用的每个未定义函数的符号指针 |
| \_\_nl_symbol_ptr | “Non lazy” symbol pointers. Symbol pointers for each undefined data symbol referenced by the executable. “Non lazy” 符号指针。可执行文件引用的每个未定义数据符号的符号指针 |

+ Mach-O Performance Implications

&emsp;Mach-O 可执行文件的 \_\_TEXT 和 \_\_DATA 的组成对性能有直接影响。优化这些 segments 的技术和目标是不同的。然而，他们有一个共同的目标：提高内存的使用效率。

&emsp;大多数典型的 Mach-O 文件都由可执行代码组成，这些代码占据了 \_\_TEXT 中的 \_\_text section。如上面 The \_\_TEXT Segment: Read Only 中所述，\_\_TEXT segment 是只读的，直接映射到可执行文件。因此，如果内核需要回收某些 \_\_text 页所占用的物理内存，它不必将这些页保存到备份存储区，并在以后对它们进行分页。它只需要释放内存，当以后引用代码时，从磁盘读回它。虽然这比交换成本更低，因为它涉及一个磁盘访问而不是两个磁盘访问，所以它仍然是昂贵的，特别是在必须从磁盘重新创建许多页的情况下。

&emsp;改善这种情况的一种方法是通过过程重新排序（procedure reordering）来改善代码的引用位置，如 [Improving Locality of Reference](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/CodeFootprint/Articles/ImprovingLocality.html#//apple_ref/doc/uid/20001862-CJBJFIDD) 中所述。这项技术根据方法和函数的执行顺序、调用频率以及调用频率将它们组合在一起。如果 \_\_text section 组中的页以这种方式正常工作，则不太可能释放它们并多次读回。例如，如果将所有启动时初始化函数放在一个或两个页上，则不必在这些初始化发生后重新创建页。

&emsp;与 \_\_TEXT segment 不同，\_\_DATA segment 可以写入，因此 \_\_DATA segment 中的页不可共享。frameworks 中的非常量全局变量（non-constant global variables）可能会对性能产生影响，因为与 framework 链接的每个进程（process）都有自己的变量副本。这个问题的主要解决方案是将尽可能多的非常量全局变量移到 \_\_TEXT 中的 \_\_const section，方法是声明它们为 const。[Reducing Shared Memory Pages](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/CodeFootprint/Articles/SharedPages.html#//apple_ref/doc/uid/20001863-CJBJFIDD) 描述了这一点和相关的技术。对于应用程序来说，这通常不是问题，因为应用程序中的 \_\_DATA section 不与其他应用程序共享。

&emsp;编译器将不同类型的非常量全局数据（nonconstant global data）存储在 \_\_DATA segment 的不同 sections 中。这些类型的数据是未初始化的静态数据和符号（uninitialized static data and symbols），它们与未声明为 extern 的 ANSI C “tentative definition” 概念一致。未初始化的静态数据（Uninitialized static data）位于 \_\_DATA segment 的 \_\_bss section。临时定义符号（tentative-definition symbols）位于 \_\_DATA segment 的 \_\_common section。

&emsp;ANSI C 和 C++ 标准规定系统必须将未初始化静态变量（uninitialized static variables）设置为零。（其他类型的未初始化数据保持未初始化状态）由于未初始化的静态变量和临时定义符号（tentative-definition symbols）存储在分开的 sections 中，系统需要对它们进行不同的处理。但是，当变量位于不同的 sections 时，它们更有可能最终出现在不同的内存页上，因此可以分别进行换入和换出操作，从而使你的代码运行速度更慢。这些问题的解决方案（如 [Reducing Shared Memory Pages](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/CodeFootprint/Articles/SharedPages.html#//apple_ref/doc/uid/20001863-CJBJFIDD) 中所述）是将非常量全局数据（non-constant global data）合并到 \_\_DATA segment 的一个 section 中。

&emsp;以上是 Overview of the Mach-O Executable Format 章节中的全部内容，可能我们对其中的 segment 和 section 还不太熟悉，后续我们会进行详细的解读。












## 参考链接
**参考链接:🔗**
+ [深入理解MachO数据解析规则](https://juejin.cn/post/6947843156163428383)
+ [iOS App启动优化（一）—— 了解App的启动流程](https://juejin.cn/post/6844903968837992461)
+ [了解iOS上的可执行文件和Mach-O格式](http://www.cocoachina.com/articles/10988)
+ [Apple 操作系统可执行文件 Mach-O](https://xiaozhuanlan.com/topic/1895704362)
+ [iOS开发之runtime（11）：Mach-O 犹抱琵琶半遮面](https://xiaozhuanlan.com/topic/0328479651)
+ [iOS开发之runtime（12）：深入 Mach-O](https://xiaozhuanlan.com/topic/9204153876)
+ [Overview of the Mach-O Executable Format](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/CodeFootprint/Articles/Articles/Articles/MachOOverview.html#//apple_ref/doc/uid/20001860-BAJGJEJC)
+ [iOS安全：Mach-O Type](https://easeapi.com/blog/blog/23.html)
+ [探秘 Mach-O 文件](http://hawk0620.github.io/blog/2018/03/22/study-mach-o-file/)
