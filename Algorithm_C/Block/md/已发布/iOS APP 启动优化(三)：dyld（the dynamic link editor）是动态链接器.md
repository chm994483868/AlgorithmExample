# iOS APP 启动优化(二)：dyld（the dynamic link editor）是动态链接器


## 静态库与动态库
&emsp;TARGETS -> Build Phases -> Link Binary With Libraries 中我们可以添加多个系统库或我们自己的库，其中包含静态库和动态库。静态库通常以 .a .lib 或者 .framework 结尾，动态库以 .tbd .so .framework 结尾。链接时，静态库会被完整的复制到可执行文件中，被多次使用就会有多份冗余拷贝，动态库链接时不复制，程序运行时由系统动态加载到内存中，供程序调用，系统只加载一次，多个程序共用，节省内存。




## 参考链接
**参考链接:🔗**
+ [OC底层原理之-App启动过程（dyld加载流程）](https://juejin.cn/post/6876773824491159565)

