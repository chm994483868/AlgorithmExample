# iOS APP 启动优化(二)：Code Size Performance Guidelines

> &emsp;**Retired Document**
> &emsp;**Important:** This document may not represent best practices for current development. Links to downloads and other resources may no longer be valid. （虽然针对当前开发可能已经不是最佳实践，但是依然具有其参考价值）

## Introduction to Code Size Performance Guidelines

&emsp;在程序性能方面，内存使用率和效率之间有明显的相关性。应用程序占用的内存越多，效率就越低。更多的内存意味着更多的内存分配、更多的代码和更多潜在的分页活动的可能性。

&emsp;本文档的编程主题的重点是减少可执行代码。减少代码占用不仅仅是在编译器中启用代码优化的问题，尽管这确实有帮助。你还可以通过组织代码来减少代码占用空间，以便在任何给定时间仅将最少数量的必需函数存储在内存中。你可以通过分析代码来实现此优化。

&emsp;减少应用程序分配的内存量对于减少内存占用也很重要；Performance Documentation 中的 [Memory Usage Performance Guidelines](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/ManagingMemory/ManagingMemory.html#//apple_ref/doc/uid/10000160i) 中包含了这些信息。

### Organization of This Document

&emsp;本编程主题（文档）包含以下内容：

+ Overview of the Mach-O Executable Format 介绍如何使用 Mach-O 可执行格式的组织来提高代码效率。
+ Managing Code Size 描述可用于减小可执行文件总体大小的几个编译器选项。
+ Improving Locality of Reference 描述如何配置和重组代码以缩短代码段（code segments）的加载时间。
+ Reducing Shared Memory Pages 描述减小 \_\_DATA 段大小的方法。
+ Minimizing Your Exported Symbols 显示如何识别和消除代码中不必要的符号信息（symbol information）。

## Overview of the Mach-O Executable Format

&emsp;Mach-O 是 OS X 中二进制文件的 native 可执行格式，是 shipping code 的首选格式。可执行格式决定二进制文件中的代码（code）和数据（data）读入内存的顺序。代码和数据的顺序会影响内存使用和分页活动（paging activity），因此会直接影响程序的性能。

&emsp;Mach-O 二进制文件被组织成段（segments）。每个段包含一个或多个 sections。不同类型的代码或数据进入每个 section。Segments 总是从页（page）边界开始，但 sections 不一定是页对齐的（page-aligned）。Segment 的大小由它包含的所有 sections 中的字节数来度量，并向上舍入到下一个虚拟内存页的边界（virtual memory page boundary）。因此，一个 segment 总是 4096 字节或 4 KB 的倍数，其中 4096 字节是最小大小。

&emsp;Mach-O 可执行文件的 segments 和 sections 根据其预期用途命名。Segment 名称的约定是使用前有双下划线的所有大写字母组成（例如：\_\_TEXT）；Section 名称的约定是使用前有双下划线的所有小写字母组成（例如：\_\_text）。

&emsp;Mach-O 可执行文件中有几个可能的 segments，但是只有两个与性能有关：\_\_TEXT segment 和 \_\_DATA segment。

### The \_\_TEXT Segment: Read Only

&emsp;\_\_TEXT segment 是一个只读区域，包含可执行代码和常量数据。按照惯例，编译器工具创建的每个可执行文件至少有一个只读 \_\_TEXT segment。由于该 segment 是只读的，内核可以将可执行文件中的 \_\_TEXT segment 直接映射（map）到内存中一次。当 segment 映射到内存中时，它可以在对其内容感兴趣的所有进程之间共享。（这主要是 frameworks 和 shared libraries 的情况。）只读属性还意味着组成 \_\_TEXT segment 的页不必保存到备份存储。如果内核需要释放物理内存，它可以丢弃一个或多个 \_\_TEXT 页，并在需要时从磁盘重新读取它们。

&emsp;表 1 列出了可以出现在 \_\_TEXT segment 中的一些更重要的 sections。有关 segments 的完整列表，请参阅 Mach-O Runtime Architecture。

&emsp;**Table 1**  Major sections in the \_\_TEXT segment

| Section | Description |
| --- | --- |
| \_\_text | The compiled machine code for the executable（可执行文件的已编译机器码） |
| \_\_const | The general constant data for the executable（可执行文件的常规常量数据） |
| \_\_cstring | Literal string constants (quoted strings in source code) 字面量字符串常量（源代码中带引号的字符串） |
| \_\_picsymbol_stub | Position-independent code stub routines used by the dynamic linker (dyld) 动态链接器（dyld）使用的与位置无关的 code stub routines |

### The \_\_DATA Segment: Read/Write

&emsp;\_\_DATA segment 包含可执行文件的非常量数据。此 segment 既可读又可写。因为它是可写的，所以 framework 或其他 shared library 的 \_\_DATA segment 在逻辑上是为每个与 library 链接的进程复制的。当内存页可读写时，内核将它们标记为 copy-on-write。此技术延迟复制页（page），直到共享该页的某个进程尝试写入该页。当发生这种情况时，内核会为该进程创建一个页（page）的私有副本。

&emsp;\_\_DATA segment 有许多 sections，其中一些 sections 仅由动态链接器（dynamic linker）使用。表 2 列出了 \_\_DATA segment 中可能出现的一些更重要的 sections。有关 segments 的完整列表，请参阅 Mach-O Runtime Architecture。

&emsp;**Table 2** Major sections of the \_\_DATA segment

| Section | Description |
| --- | --- |
| \_\_data | Initialized global variables (for example int a = 1; or static int a = 1;). 初始化的全局变量 |
| \_\_const | Constant data needing relocation (for example, char * const p = "foo";). 需要重定位的常量数据 |
| \_\_bss | Uninitialized static variables (for example, static int a;). 未初始化的静态变量 |
| \_\_common | Uninitialized external globals (for example, int a; outside function blocks). 未初始化的外部全局变量 |
| \_\_dyld | A placeholder section, used by the dynamic linker. 动态链接器使用的占位符部分 |
| \_\_la_symbol_ptr | “Lazy” symbol pointers. Symbol pointers for each undefined function called by the executable. “Lazy” 符号指针。可执行文件调用的每个未定义函数的符号指针 |
| \_\_nl_symbol_ptr | “Non lazy” symbol pointers. Symbol pointers for each undefined data symbol referenced by the executable. “Non lazy” 符号指针。可执行文件引用的每个未定义数据符号的符号指针 |

### Mach-O Performance Implications

&emsp;Mach-O 可执行文件的 \_\_TEXT 和 \_\_DATA 的组成对性能有直接影响。优化这些 segments 的技术和目标是不同的。然而，他们有一个共同的目标：提高内存的使用效率。

&emsp;大多数典型的 Mach-O 文件都由可执行代码组成，这些代码占据了 \_\_TEXT 中的 \_\_text section。如上面 The \_\_TEXT Segment: Read Only 中所述，\_\_TEXT segment 是只读的，直接映射到可执行文件。因此，如果内核需要回收某些 \_\_text 页所占用的物理内存，它不必将这些页保存到备份存储区，并在以后对它们进行分页。它只需要释放内存，当以后引用代码时，从磁盘读回它。虽然这比交换成本更低，因为它涉及一个磁盘访问而不是两个磁盘访问，所以它仍然是昂贵的，特别是在必须从磁盘重新创建许多页的情况下。

&emsp;改善这种情况的一种方法是通过过程重新排序（procedure reordering）来改善代码的引用位置，如 [Improving Locality of Reference](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/CodeFootprint/Articles/ImprovingLocality.html#//apple_ref/doc/uid/20001862-CJBJFIDD) 中所述。这项技术根据方法和函数的执行顺序、调用频率以及调用频率将它们组合在一起。如果 \_\_text section 组中的页以这种方式正常工作，则不太可能释放它们并多次读回。例如，如果将所有启动时初始化函数放在一个或两个页上，则不必在这些初始化发生后重新创建页。

&emsp;与 \_\_TEXT segment 不同，\_\_DATA segment 可以写入，因此 \_\_DATA segment 中的页不可共享。frameworks 中的非常量全局变量（non-constant global variables）可能会对性能产生影响，因为与 framework 链接的每个进程（process）都有自己的变量副本。这个问题的主要解决方案是将尽可能多的非常量全局变量移到 \_\_TEXT 中的 \_\_const section，方法是声明它们为 const。[Reducing Shared Memory Pages](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/CodeFootprint/Articles/SharedPages.html#//apple_ref/doc/uid/20001863-CJBJFIDD) 描述了这一点和相关的技术。对于应用程序来说，这通常不是问题，因为应用程序中的 \_\_DATA section 不与其他应用程序共享。

&emsp;编译器将不同类型的非常量全局数据（nonconstant global data）存储在 \_\_DATA segment 的不同 sections 中。这些类型的数据是未初始化的静态数据和符号（uninitialized static data and symbols），它们与未声明为 extern 的 ANSI C “tentative definition” 概念一致。未初始化的静态数据（Uninitialized static data）位于 \_\_DATA segment 的 \_\_bss section。临时定义符号（tentative-definition symbols）位于 \_\_DATA segment 的 \_\_common section。

&emsp;ANSI C 和 C++ 标准规定系统必须将未初始化静态变量（uninitialized static variables）设置为零。（其他类型的未初始化数据保持未初始化状态）由于未初始化的静态变量和临时定义符号（tentative-definition symbols）存储在分开的 sections 中，系统需要对它们进行不同的处理。但是，当变量位于不同的 sections 时，它们更有可能最终出现在不同的内存页上，因此可以分别进行换入和换出操作，从而使你的代码运行速度更慢。这些问题的解决方案（如 [Reducing Shared Memory Pages](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/CodeFootprint/Articles/SharedPages.html#//apple_ref/doc/uid/20001863-CJBJFIDD) 中所述）是将非常量全局数据（non-constant global data）合并到 \_\_DATA segment 的一个 section 中。

## Managing Code Size

&emsp;GCC 编译器支持各种优化代码的选项。这些技术中的大多数都会根据你的需要生成更少的代码或更快的代码（less code or faster code）。当你准备发布软件时，你应该尝试这些技术，看看哪些技术对你的代码最有好处。

### Compiler-Level Optimizations

&emsp;当你的项目代码稳定下来时，你应该开始试验用于优化代码的基本 GCC 选项。GCC 编译器支持优化选项，允许你选择是使用较小的二进制大小（smaller binary size）、更快的代码（faster code）还是更快的构建时间（faster build times）。

&emsp;对于新项目，Xcode 会自动禁用开发构建样式的优化（optimizations for the development build style），并选择 “fastest, smallest” 部署构建样式（“fastest, smallest” option for the deployment build style）的选项。任何类型的代码优化都会导致生成时间变慢，因为优化过程涉及到额外的工作。如果你的代码正在更改（如在开发周期中所做的那样），则你不希望启用优化。在开发周期即将结束时，部署构建样式（deployment build style）可以为你指示最终产品的大小。

&emsp;Table 1 列出了 Xcode 中可用的优化级别（optimization levels）。当你选择其中一个选项时，Xcode 会将给定组或文件的相应标志传递给 GCC 编译器（ Xcode passes the appropriate flags to the GCC compiler for the given group or files）。这些选项可在 target-level 或作为 build style 的一部分可见。有关项目的构建设置的信息，请参阅 Xcode Help。

&emsp;Table 1  GCC compiler optimization options
| Xcode Setting | Description |
| --- | --- |
| None | 编译器不会尝试优化代码。当你专注于解决逻辑错误并且需要快速编译时，请在开发过程中使用此选项。 Do not use this option for shipping your executable。 |
| Fast | 编译器执行简单的优化以提高代码性能，同时最小化对编译时间的影响。此选项在编译期间也会使用更多内存。 |
| Faster | 执行几乎所有不需要 space-time trade-off 的受支持优化。使用此选项时编译器不执行循环展开（loop unrolling）或函数内联（function inlining）。此选项增加编译时间和生成代码的性能。 |
| Fastest | 执行所有优化以提高生成代码的速度。当编译器执行积极的函数内联时，此选项可以增加生成代码的大小。<br>通常不建议使用此选项。有关详细信息，请参见避免过多的函数内联（Avoid Excessive Function Inlining）。 |
| Fastest, smallest | 执行通常不会增加代码大小的所有优化。这是传送代码（shipping code）的首选选项，因为它使可执行文件的内存占用更小。 |

&emsp;与任何性能增强一样，不要假设哪个选项会给你带来最佳效果。你应该始终衡量你尝试的每个优化的结果。例如，“Fastest” 选项可能会为特定模块生成速度极快的代码，但这样做通常会以牺牲可执行文件的大小为代价。如果代码需要在运行时从磁盘中传入，则你从代码生成中获得的任何速度优势都很容易丢失。（如果代码需要在运行时从磁盘分页，那么从代码生成中获得的任何速度优势都很容易丢失。）

### Additional Optimizations

&emsp;除了代码级（code-level）优化之外，你还可以使用一些附加技术在 module level 组织代码。以下各节介绍这些技术。

#### Dead Strip Your Code

&emsp;对于静态链接（statically-linked）的可执行文件，dead-code stripping 是从可执行文件中删除未引用代码的过程。dead-stripping 背后的思想是，如果代码未被引用，就不能使用它，因此在可执行文件中就不需要它。删除 dead code 可以减少可执行文件的大小，并有助于减少分页（reduce paging）。

&emsp;从 Xcode Tools 版本 1.5 开始，静态链接器（static linker）（ld）支持可执行文件的 dead stripping。你可以直接从 Xcode 或通过向静态链接器（static linker）传递适当的命令行选项来启用此功能。

&emsp;要在 Xcode 中启用 dead-code stripping，请执行以下操作：

1. 选择你的 target。
2. 打开 Inspector 或 Get Info 窗口并选择 Build 选项卡。
3. 在链接设置（Linking settings）中，启用 Dead Code Stripping 选项。

&emsp;TARGETS -> Build Settings -> 搜索 Linking -> Dead Code Stripping 设置为 YES/NO（默认是 YES）。

4. 在 Code Generation settings 中，将 Level of Debug Symbols 选项设置为 All Symbols。
&emsp;TARGETS -> Build Settings -> 搜索 All Symbols -> Strip Style 设置为 All Symbols/Non-Global Symbols/Debugging Symbols（默认是 All Symbols）。

&emsp;要从命令行启用 dead-code stripping，请将 -dead_strip 选项传递给 ld。还应该将 -gfull 选项传递给 GCC，以便为代码生成一组完整的调试符号（debugging symbols）。链接器（linker）使用这个额外的调试信息对可执行文件进行 dead strip。

> &emsp;Note: 建议使用 “All Symbols” 或 -gfull 选项，即使你不打算 dead strip。尽管该选项生成较大的中间文件（intermediate files），但通常会生成较小的可执行文件，因为链接器（linker）能够更有效地删除重复的符号信息。

&emsp;如果不想删除任何未使用的函数，至少应该将它们隔离在 \_\_TEXT segment 的一个单独部分中。将未使用的函数移到 common section 可以改进代码引用的局部性，并降低它们被加载到内存中的可能性。有关如何在 common section 中对函数进行分组的详细信息，请参见 Improving Locality of Reference。

#### Strip Symbol Information

&emsp;调试符号（debugging symbols）和动态绑定信息（dynamic-binding information）可能会占用大量空间，并且占可执行文件大小的很大一部分。在 shipping 代码之前，应该去掉所有不需要的符号。

&emsp;要从可执行文件中删除调试符号（debugging symbols），请将 Xcode build-style 设置更改为 “Deployment”，然后 rebuild 可执行文件。如果你愿意，还可以按目标（target）生成调试符号。有关构建样式（build styles）和目标设置（target settings）的详细信息，请参阅Xcode Help。

&emsp;要从可执行文件中手动删除动态绑定符号（dynamic-binding symbols），请使用 strip tool。此工具删除动态链接器（dynamic linker）在运行时通常用于绑定外部符号的符号信息。删除不希望动态绑定的函数的符号会减少可执行文件的大小，并减少动态链接器必须绑定的符号数。通常，你会使用此命令而不使用任何选项来删除非外部符号（non-external symbols），如以下示例所示：

```c++
% cd ~/MyApp/MyApp.app/Contents/MacOS
% strip MyApp
```

&emsp;此命令相当于使用 -u 和 -r 选项运行 strip。它删除所有标记为非外部的符号，但不删除标记为外部的符号。

&emsp;手动剥离动态绑定符号的另一种方法是使用导出文件来限制在构建时导出的符号。导出文件标识运行时从可执行文件中可用的特定符号。有关创建导出文件的详细信息，请参见 Minimizing Your Exported Symbols。

&emsp;An alternative to stripping out dynamic-binding symbols manually is to use an exports file to limit the symbols exported at build time. An exports file identifies the specific symbols available at runtime from your executable. For more information on creating an exports file, see Minimizing Your Exported Symbols.

#### Eliminate C++ Exception Handling Overhead

&emsp;当抛出异常时，C++ 运行库必须能够将堆栈展开回第一匹配 catch 块的点。为此，GCC  编译器为每个可能引发异常的函数生成堆栈展开信息。此展开信息存储在可执行文件中，并描述堆栈上的对象。这些信息使得在抛出异常时调用这些对象的析构函数来清除它们成为可能。

&emsp;即使你的代码不抛出异常，GCC 编译器仍然会为 C++ 代码生成默认的堆栈展开信息。如果你广泛使用异常，这个额外的代码会显著增加可执行文件的大小。

##### Disabling Exceptions

&emsp;通过禁用目标的 “Enable C++ Exceptions” 构建选项，可以禁用 XCoad 中的异常处理。从命令行，将 -fno-exceptions 选项传递给编译器。此选项删除函数的堆栈展开信息。但是，仍然必须从代码中删除任何 try、catch 和 throw 语句。

##### Selectively Disabling Exceptions

&emsp;如果代码在某些地方使用异常，而不是在任何地方使用异常，则可以通过向方法声明中添加空的异常规范来显式标识不需要展开信息的方法。例如，在下面的代码中，编译器必须为 my_function 生成堆栈展开信息，理由是 my_other_function 或它调用的函数可能引发异常。

```c++
extern int my_other_function (int a, int b);
int my_function (int a, int b)
{
   return my_other_function (a, b);
}
```

&emsp;但是，如果你知道 my_other_function 函数不能抛出异常，你可以通过在函数声明中包含空异常规范（throw ()）来向编译器发出信号。因此，你可以如下声明前面的函数：

```c++
extern int foo (int a, int b) throw ();
int my_function (int a, int b) throw ()
{
   return foo (a, b);
}

```

##### Minimizing Exception Use

&emsp;















## 参考链接
**参考链接:🔗**
+ [Code Size Performance Guidelines](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/CodeFootprint/CodeFootprint.html#//apple_ref/doc/uid/10000149-SW1)
+ [Memory Usage Performance Guidelines](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/ManagingMemory/ManagingMemory.html#//apple_ref/doc/uid/10000160-SW1)
