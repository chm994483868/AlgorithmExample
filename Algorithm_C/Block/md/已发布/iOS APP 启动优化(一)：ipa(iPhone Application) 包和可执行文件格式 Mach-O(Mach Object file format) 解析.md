# iOS APP 启动优化(一)：ipa(iPhone application archive) 包和可执行文件格式 Mach-O(Mach Object file format) 解析

> IPA 后缀的文件是 iOS 系统的软件包，全称为 iPhone application archive。通常情况下，IPA 文件都是使用苹果公司的 FairPlayDRM 技术进行加密保护的。每个 IPA 文件都是 ARM 架构的可执行文件以及该应用的资源文件的打包文件，只能安装在 iPhone、iPod Touch、iPad 以及使用 Apple Silicon 平台的 Mac 上。该文件可以通过修改后缀名为 zip 后，进行解压缩，查看其软件包中的内容。[IPA文件-维基百科](https://zh.wikipedia.org/wiki/IPA文件)

## 解压 .ipa 文件查看其内容并引出 Mach-O 格式
&emsp;相信每一位 iOS 开发者都进行过打包测试，当我们把 Ad Hoc 或者 App Store Connect 的包导出到本地时会看到一个 xxx.ipa 文件，ipa 是 iPhone Application 的缩写。实际上 xxx.ipa 只是一个变相的 zip 压缩包，我们可以把 xxx.ipa 文件直接通过 unzip 命令进行解压。

&emsp;我们直接新建一个命名为 Test_ipa_Simple 的空白 iOS App，直接进行 Archive 后并导出 Test_ipa_Simple.ipa 文件查看它的内部结构。在终端执行 unzip Test_ipa_Simple.ipa 解压之后，会有一个 Payload 目录，而 Payload 里则是一个看似是文件的 Test_ipa_Simple.app，而实际上它又是一个目录，或者说是一个完整的 App Bundle。其中 Base.lproj 中是我们的 Main.storyboard 和 LaunchScreen.storyboard 的内容，然后是 embedded.mobileprovision（描述文件）和 PkgInfo、Info.plist、_CodeSignature 用于描述 App 的一些信息，然后我们要重点关注的便是当前这个目录里面体积最大的文件 Test_ipa_Simple，它是和我们的 ipa 包同名的一个二进制文件，然后用 file 命令查看他的文件类型是一个在 arm64 处理器架构下的可执行文件，格式则是 Mach-O。（下面是终端执行记录，可大致浏览一下）

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












## 参考链接
**参考链接:🔗**
+ [深入理解MachO数据解析规则](https://juejin.cn/post/6947843156163428383)
+ [iOS App启动优化（一）—— 了解App的启动流程](https://juejin.cn/post/6844903968837992461)
+ [了解iOS上的可执行文件和Mach-O格式](http://www.cocoachina.com/articles/10988)
+ [Apple 操作系统可执行文件 Mach-O](https://xiaozhuanlan.com/topic/1895704362)
+ [iOS开发之runtime（11）：Mach-O 犹抱琵琶半遮面](https://xiaozhuanlan.com/topic/0328479651)
+ [iOS开发之runtime（12）：深入 Mach-O](https://xiaozhuanlan.com/topic/9204153876)
