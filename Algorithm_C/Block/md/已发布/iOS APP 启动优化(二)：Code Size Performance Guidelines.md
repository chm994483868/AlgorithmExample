# iOS APP 启动优化(二)：Code Size Performance Guidelines

> &emsp;**Retired Document**
> &emsp;**Important:** This document may not represent best practices for current development. Links to downloads and other resources may no longer be valid. （虽然针对当前开发可能已经不是最佳实践，但是依然具有其参考价值）

## Introduction to Code Size Performance Guidelines

&emsp;在程序性能方面，内存使用率和效率之间有明显的相关性。应用程序占用的内存越多，效率就越低。更多的内存意味着更多的内存分配、更多的代码和更多潜在的分页活动的可能性。

&emsp;本文档的主题的重点是减少可执行代码。减少代码占用不仅仅是在编译器中启用代码优化的问题，尽管这确实有帮助。你还可以通过组织代码来减少代码占用空间，以便在任何给定时间仅将最少数量的必需函数存储在内存中。你可以通过分析代码来实现此优化。

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

##### Minimizing Exception Use（尽量减少异常捕获的使用）

&emsp;在编写代码时，请仔细考虑异常的使用。异常应该用来表示异常情况，也就是说，它们应该用来报告你没有预料到的问题。如果从文件读取时出现文件结束错误（end-of-file error），则不希望抛出异常，因为这是一种已知类型的错误，可以轻松处理。如果你试图读取一个已知已打开的文件，并且被告知该文件 ID 无效，那么你可能希望抛出一个异常。

#### Avoid Excessive Function Inlining（避免内联函数使用过度）

&emsp;尽管内联函数（inline functions）在某些情况下可以提高速度，但如果使用过度，它们也会降低 OS X 上的性能。内联函数消除了调用函数的开销，但是通过用代码的副本（copy of the code）替换每个函数调用来实现。如果内联函数经常被调用，那么这些额外的代码会很快累积起来，使可执行文件膨胀并导致分页问题。

&emsp;如果使用得当，内联函数可以节省时间，并且对代码占用的影响最小。请记住，内联函数的代码通常应该非常短，很少调用。如果在函数中执行代码所需的时间少于调用函数所需的时间，则函数是内联的最佳候选函数。一般来说，这意味着一个内联函数的代码应该不超过几行。你还应该确保从代码中尽可能少的地方调用函数。即使是一个很短的函数，如果在几十个或几百个地方内联使用，也会导致过度膨胀。

&emsp;另外，你应该知道，一般应该避免使用 GCC 的 “Fastest” 优化级别。在这个优化级别上，编译器会积极地尝试创建内联函数，即使对于没有标记为内联的函数也是如此。不幸的是，这样做会大大增加可执行文件的大小，并由于分页而导致更糟糕的性能问题。

#### Build Frameworks as a Single Module

&emsp;大多数共享库（shared libraries）不需要 Mach-O 运行时的模块特性（module features）。此外，跨模块调用产生的开销与跨库调用相同。因此，你应该将项目的所有中间对象文件链接到一个模块中。

&emsp;要合并对象文件，必须在链接阶段（link phase）将 -r 选项传递给 ld。如果你使用 Xcode 来构建代码，那么默认情况下这是为你完成的。

## Improving Locality of Reference

&emsp;对应用程序性能的一个重要改进是减少应用程序在任何给定时间使用的虚拟内存页（virtual memory pages）的数量。这组页（set of pages）称为工作集（working set），由应用程序代码（application code）和运行时数据（runtime data）组成。减少内存中数据的数量（in-memory data）是算法的一个功能（is a function of your algorithms），但是减少内存中代码的数量（in-memory code）可以通过一个称为分散加载（scatter loading）的处理来实现。这种技术也被称为改进代码引用的局部性（improving the locality of reference）。

&emsp;通常，方法和函数的编译代码是由源文件以生成的二进制文件组织的。（通常，编译的方法和函数代码由生成的二进制文件中的源文件组织。）分散加载（scatter loading）会更改此组织，而是将相关方法和函数分组在一起，而与这些方法和函数的原始位置无关。这个过程允许内核将活动应用程序（active application）最常引用的可执行页保存在尽可能小的内存空间中。这不仅减少了应用程序的占用空间，还降低了这些页面被调出（大概是指内存紧张时被回收）的可能性。

> &emsp;Important:通常应该等到开发周期的很晚才分散加载应用程序。在开发过程中，代码往往会四处移动，这会使以前的评测结果无效。

### Profiling Code With gprof（通过 gprof 分析代码）

&emsp;根据运行时收集的分析数据
，gprof 生成程序的 execution profile。被调用例程的效果包含在每个调用方的 profile 中。profile 数据取自 call graph profile file(gmon.out 默认情况下），它是由程序编译创建的，并与 -pg 选项链接。可执行文件中的符号表（symbol table）与 call graph profile file 相关联。如果指定了多个 profile file，gprof 输出将显示给定 profile files 中 profile 信息的总和。

&emsp;gprof 工具有很多用途，包括：

+ Sampler application 工作不好的情况，例如 command-line tools 或在短时间后退出的应用程序
+ 在这种情况下，你需要一个包含给定程序中可能调用的所有代码的 call graph，而不是周期性地对调用进行采样
+ 需要更改代码的 link order 以优化代码局部性的情况

#### Generating Profiling Data

&emsp;在分析应用程序之前，必须将项目设置为 generate profiling information。要为 Xcode 项目生成 profiling information，必须修改 target 或 build-style settings，以包含 “Generate profiling code” 选项。（位置在 TARGETS -> Build Settings -> Build Options -> Generate profiling code(YES/NO)）(有关启用 target 和 build-style settings 的信息，请参见 Xcode Help）

&emsp;程序内的 profiling code 生成一个名为 gmon.out 且包含 profiling information 的文件。
(通常，此文件放在当前工作目录中。）若要分析此文件中的数据，请在调用 gprof 之前将其复制到包含可执行文件的目录中，或只是指定路径到 gmon.out 当你运行 gprof 时。

&emsp;除了分析你自己的代码之外，你可以通过与 Carbon 和 Cocoa frameworks 这些框架的 profile versions 进行链接，找出它们花费了多少时间。
为此，请将 DYLD_IMAGE_SUFFIX 设置添加到 target 或 build style，并将其值设置为 \_profile。dynamic linker 将此后缀与 framework 名称相结合，以针对 framework 的 profile version 进行链接。要确定哪些 frameworks 支持 profiling （概要分析），请查看 frameworks 本身。例如，Carbon library 附带了 profile 和 debug 版本。

&emsp;Note: libraries 的 profile 和 debug 版本是作为 developer tools package 的一部分安装的，在用户系统上可能不可用。确保你的 shipping executable 没有链接到这些库之一。

#### Generating Order Files

&emsp;order file 包含一个有序的 lines 序列，每一 line 由一个 source file name 和一个 symbol name 组成，用冒号分隔，没有其他空格。每一 line 表示要放置在可执行文件 section 中的 block。如果手动修改 order file，则必须完全遵循此格式，以便 linker 可以处理该 order file。如果 object file 的 name:symbol name pair 并不完全是 linker 看到的名称，它会尽最大努力将名称与被 linked 的 objects 匹配起来。

&emsp;procedure 重新排序的 order file 中的 lines 由 an object filename 和 procedure name（function、method 或其他 symbol）组成。order file 中列出 procedures（程序） 的顺序表示它们链接到可执行文件的 \_\_text section 的顺序。

&emsp;要从使用 program 生成的 profiling data 创建 order file，请使用 -S 选项运行 gprof（请参阅 gprof 的手册页）。例如：

```c++
gprof -S MyApp.profile/MyApp gmon.out
```

&emsp;-S 选项生成四个相互排斥的 order files：

| gmon.order | 基于 profiling call graph 的 “closest is best” 分析进行排序。经常互相 call 的 call 放在一起。 |
| callf.order | Routines 按对每个 Routine 的调用次数排序，首先最大调用次数。 |
| callo.order | 按照调用顺序对 Routines 进行排序 |
| time.order | 按花费的 CPU 时间对 Routines 进行排序，花最多时间的 Routine 放在第一。 |

&emsp;你应该尝试使用这些 files 中的每一个，看看哪些 file 提供了最大的性能改进（如果有的话）。
请参阅 Using pagestuff to Examine Pages on Disk，以便讨论如何衡量 ordering 结果。

&emsp;这些 order files 只包含 profiling 期间使用的那些 procedures。linker 跟踪缺失的 procedures（程序），并在 order files 中列出的程序之后以默认顺序将它们链接起来。
仅当项目目录（project directory）包含由 linker 的 -whatsloaded 选项生成的文件时，才会在 order file 中生成 library functions 的 static names（静态名称）；有关详细信息，请参见 Creating a Default Order File。

&emsp;gprof-S 选项不适用于已使用 order file 链接的可执行文件。

#### Fixing Up Your Order Files

&emsp;生成 order files 后，你应该仔细检查它们并确保它们是正确的。在许多情况下，你需要手动编辑 order files，包括：

+ 可执行文件包含汇编语言文件（assembly-language files）。
+ 你 profiled（分析）了一个 stripped 的可执行文件。
+ 你的可执行文件包含未使用 -g 选项编译的文件。
+ 你的项目定义内部标签（defines internal labels）（通常用于 goto 语句）。
+ 你希望保留特定 object file 中例程的顺序（order of routines）。

&emsp;如果 symbol 的定义位于 an assembly file、a stripped executable file 或 a file compiled without the -g option，gprof 将从 order file 中的 symbol’s entry 中忽略 source file name。如果项目使用此类 files，则必须手动编辑 order file 并添加适当的 source filenames。或者，你可以完全删除 symbol references，以强制以默认顺序 linked 相应的 routines。

&emsp;如果代码包含 internal labels，则必须从 order files 中删除这些 labels；否则，定义标签的函数将在链接阶段被拆分。可以通过在程序集文件前面加上字符串L\来防止将内部标签包含在程序集文件中。汇编程序将带有此前缀的符号解释为特定函数的本地符号，并将其剥离以防止其他工具（如gprof）访问。









If your code contains internal labels, you must remove those labels from the order files; otherwise, the function that defines the label will be split apart during the linking phase. You can prevent the inclusion of internal labels in assembly files altogether by prefixing them with the string L_. The assembler interprets symbols with this prefix as local to a particular function and strips them to prevent access by other tools such as gprof.

















## 参考链接
**参考链接:🔗**
+ [Code Size Performance Guidelines](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/CodeFootprint/CodeFootprint.html#//apple_ref/doc/uid/10000149-SW1)
+ [Memory Usage Performance Guidelines](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/ManagingMemory/ManagingMemory.html#//apple_ref/doc/uid/10000160-SW1)
