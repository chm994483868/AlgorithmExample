# iOS 多线程知识体系构建(六)：GCD API（block.h、io.h）解析篇

> &emsp;那么继续学习 dispath 中也挺重要的 <dispatch/block.h> 文件。

## <dispatch/block.h>
&emsp;Dispatch block objects.
### dispatch_block_flags_t
&emsp;
&emsp;`DISPATCH_OPTIONS` 宏定义：
```c++
#if __has_feature(objc_fixed_enum) || __has_extension(cxx_strong_enums) || \
        __has_extension(cxx_fixed_enum) || defined(_WIN32)

#define DISPATCH_OPTIONS(name, type, ...) \
        typedef enum : type { __VA_ARGS__ } __DISPATCH_OPTIONS_ATTR __DISPATCH_ENUM_ATTR name##_t
        
#else

#define DISPATCH_OPTIONS(name, type, ...) \
        enum { __VA_ARGS__ } __DISPATCH_OPTIONS_ATTR __DISPATCH_ENUM_ATTR; typedef type name##_t
        
#endif // __has_feature(objc_fixed_enum) ...
```
&emsp;`dispatch_block_flags_t`:
```c++
DISPATCH_OPTIONS(dispatch_block_flags, unsigned long,
    DISPATCH_BLOCK_BARRIER
            DISPATCH_ENUM_API_AVAILABLE(macos(10.10), ios(8.0)) = 0x1, // 二进制表示每次进一位
    DISPATCH_BLOCK_DETACHED
            DISPATCH_ENUM_API_AVAILABLE(macos(10.10), ios(8.0)) = 0x2,
    DISPATCH_BLOCK_ASSIGN_CURRENT
            DISPATCH_ENUM_API_AVAILABLE(macos(10.10), ios(8.0)) = 0x4,
    DISPATCH_BLOCK_NO_QOS_CLASS
            DISPATCH_ENUM_API_AVAILABLE(macos(10.10), ios(8.0)) = 0x8,
    DISPATCH_BLOCK_INHERIT_QOS_CLASS
            DISPATCH_ENUM_API_AVAILABLE(macos(10.10), ios(8.0)) = 0x10,
    DISPATCH_BLOCK_ENFORCE_QOS_CLASS
            DISPATCH_ENUM_API_AVAILABLE(macos(10.10), ios(8.0)) = 0x20,
);
```


## 参考链接
**参考链接:🔗**
+ [swift-corelibs-libdispatch-main](https://github.com/apple/swift-corelibs-libdispatch)
+ [Dispatch 官方文档](https://developer.apple.com/documentation/dispatch?language=objc)
+ [iOS libdispatch浅析](https://juejin.im/post/6844904143174238221)
+ [GCD--百度百科词条](https://baike.baidu.com/item/GCD/2104053?fr=aladdin)
+ [iOS多线程：『GCD』详尽总结](https://juejin.im/post/6844903566398717960)
+ [iOS底层学习 - 多线程之GCD初探](https://juejin.im/post/6844904096973979656)
+ [GCD 中的类型](https://blog.csdn.net/u011374318/article/details/87870585)
+ [iOS Objective-C GCD之queue（队列）篇](https://www.jianshu.com/p/d0017f74f9ca)
+ [变态的libDispatch结构分析-object结构](https://blog.csdn.net/passerbysrs/article/details/18223845)
+ [__builtin_expect 说明](https://www.jianshu.com/p/2684613a300f)
+ [内存屏障(__asm__ __volatile__("": : :"memory"))](https://blog.csdn.net/whycold/article/details/24549571)
