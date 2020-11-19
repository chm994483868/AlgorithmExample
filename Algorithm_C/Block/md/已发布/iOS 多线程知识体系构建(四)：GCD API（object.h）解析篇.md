# iOS 多线程知识体系构建(四)：GCD API（object.h）解析篇

> &emsp;那么继续学习 dispath 中也挺重要的 object.h 文件。

## dispatch_object_t
&emsp;`dispatch_object_t` 是所有调度对象（dispatch objects）的抽象基类型，且 `dispatch_object_t` 的具体定义在特定语言（Swift/Objective-C/C）下不同。调度对象通过调用 `dispatch_retain` 和 `dispatch_release` 进行引用计数管理。

&emsp;默认情况下，使用 Objective-C 编译器进行构建时，调度对象被声明为 Objective-C 类型。这使他们可以参与 ARC，通过 Blocks 运行时参与 RR（retain/release）管理以及通过静态分析器参与泄漏检查，并将它们添加到 Cocoa 集合（NSMutableArray、NSMutableDictionary...）中。详细信息可参考 <os/object.h>，下面会对 <os/object.h> 文件进行分析，特别是其中的 `OS_OBJECT_DECL_CLASS` 宏定义，分别针对不同的语言环境来定义不同的  `dispatch_object_t`。

&emsp;下面先看结论，然后再看 <os/object.h> 文件。

&emsp;`dispatch_object_t` 来自如下宏:
```c++
OS_OBJECT_DECL_CLASS(dispatch_object);
```
+ 在 Swift 下宏定义展开是:
```c++
OS_EXPORT OS_OBJECT_OBJC_RUNTIME_VISIBLE
@interface OS_dispatch_object : OS_object
- (instancetype)init OS_SWIFT_UNAVAILABLE("Unavailable in Swift");
@end
typedef OS_dispatch_object * dispatch_object_t
```
&emsp;`OS_dispatch_object` 是继承自 `OS_object` 的类，然后 `dispatch_object_t` 是一个指向 `OS_dispatch_object` 的指针。
+ 在 Objective-C 下宏定义展开是:
```c++
@protocol OS_dispatch_object <NSObject> 
@end
typedef NSObject<OS_dispatch_object> * dispatch_object_t;  
```
&emsp;`OS_dispatch_object` 是继承自 `NSObject` 协议的协议，并且为遵循该协议的 `NSObject` 实例对象类型的指针定义了一个 `dispatch_object_t` 的别名。
+ 在 C++ 下宏定义展开是:
```c++
typedef struct dispatch_object_s *dispatch_object_t
```
&emsp;`dispatch_object_t` 是一个指向 `dispatch_object_s` 结构体的指针。

&emsp;`dispatch_object_s` 的定义也很简单，就是一个很基本的 C++ 结构体定义：
（注释：调度对象不是 C++ 对象。尽管如此，我们至少可以使 C++ 知道类型兼容性。）
```c++
typedef struct dispatch_object_s {
private:
    dispatch_object_s(); // 构造函数
    ~dispatch_object_s(); // 析构函数
    dispatch_object_s(const dispatch_object_s &); // 复制构造函数
    void operator=(const dispatch_object_s &); // 赋值操作符
} *dispatch_object_t; // dispatch_object_t 是指向 dispatch_object_s 结构体的指针。
```
+ 在 C（Plain C）下是:
&emsp;`dispatch_object_t` 不再是一个指针而是一个联合体（union）：
```c++
typedef union {
    struct _os_object_s *_os_obj;
    struct dispatch_object_s *_do;
    struct dispatch_queue_s *_dq;
    struct dispatch_queue_attr_s *_dqa;
    struct dispatch_group_s *_dg;
    struct dispatch_source_s *_ds;
    struct dispatch_channel_s *_dch;
    struct dispatch_mach_s *_dm;
    struct dispatch_mach_msg_s *_dmsg;
    struct dispatch_semaphore_s *_dsema;
    struct dispatch_data_s *_ddata;
    struct dispatch_io_s *_dchannel;
} dispatch_object_t DISPATCH_TRANSPARENT_UNION;
```
&emsp;看到里面有很多我们陌生的结构体，这里暂时不进行解读，等到学习源码时我们自然能见到它们的定义。

&emsp;下面看一下 <os/object.h> 文件。
## <os/object.h> 文件
>
>  @header
>  @preprocinfo
>  &emsp;By default, libSystem objects such as GCD and XPC objects are declared as
>  Objective-C types when building with an Objective-C compiler. This allows
>  them to participate in ARC, in RR management by the Blocks runtime and in
>  leaks checking by the static analyzer, and enables them to be added to Cocoa
>  collections.
> 
>  NOTE: this requires explicit cancellation of dispatch sources and xpc
>        connections whose handler blocks capture the source/connection object,
>        resp. ensuring that such captures do not form retain cycles (e.g. by
>        declaring the source as __weak).
> 
>  &emsp;To opt-out of this default behavior, add -DOS_OBJECT_USE_OBJC=0 to your
>  compiler flags.
> 
>  &emsp;This mode requires a platform with the modern Objective-C runtime, the
>  Objective-C GC compiler option to be disabled, and at least a Mac OS X 10.8
>  or iOS 6.0 deployment target.
> 
> &emsp;默认情况下，在使用 Objective-C 编译器进行构建时，libSystem 对象（例如 GCD 和 XPC 对象）被声明为 Objective-C 类型，这使他们可以参与 ARC，通过 Blocks 运行时参与 RR 管理以及通过静态分析器参与泄漏检查，并将它们添加到 Cocoa 集合中。
> 
> &emsp;注意：这需要显式取消 dispatch sources 和 xpc 连接来处理 blocks 捕获 source/connection 对象。 确保此类捕获不会形成循环引用（例如，通过将来源声明为 __weak）。
>
> &emsp;要选择退出此默认行为，请将 DOS_OBJECT_USE_OBJC = 0 添加到的编译器标志中即可。
> 
> &emsp;此模式要求平台具有现代的 Objective-C runtime，要禁用的 Objective-C GC 编译器选项，以及至少 Mac OS X 10.8 或 iOS 6.0 的版本要求。

### OS_OBJECT_HAVE_OBJC_SUPPORT
&emsp;`OS_OBJECT_HAVE_OBJC_SUPPORT` 仅在 macOS 10.8（i386 则是 10.12）以上或者 iOS 6.0 值为 1， 其它情况为 0。
### OS_OBJECT_USE_OBJC
&emsp;在 `OS_OBJECT_HAVE_OBJC_SUPPORT` 为 1 的情况下，在 macOS/iOS `__swift__` 情况下都是 1。
### OS_OBJECT_SWIFT3
&emsp;在 `__swift__` 宏存在时，`OS_OBJECT_SWIFT3` 都为 1。
### OS_OBJC_INDEPENDENT_CLASS
&emsp;`OS_OBJECT_USE_OBJC` 为 1 的情况下，存在 `objc_independent_class` 属性，则 `OS_OBJC_INDEPENDENT_CLASS` 是： `__attribute__((objc_independent_class))` 否则只是一个空的宏定义。
### OS_OBJECT_CLASS
&emsp;`#define OS_OBJECT_CLASS(name) OS_##name` 仅是为 `name` 添加一个 `OS_` 前缀，如 `OS_OBJECT_CLASS(object)` 宏展开是 `OS_object`。
### OS_OBJECT_DECL_PROTOCOL
&emsp;用于协议声明，`__VA_ARGS__` 是多参的宏展开时连续按序拼接各个参。
```c++
#define OS_OBJECT_DECL_PROTOCOL(name, ...) \
@protocol OS_OBJECT_CLASS(name) __VA_ARGS__ \
@end
```
### OS_OBJECT_CLASS_IMPLEMENTS_PROTOCOL_IMPL
&emsp;类声明并遵循指定的协议。
```c++
#define OS_OBJECT_CLASS_IMPLEMENTS_PROTOCOL_IMPL(name, proto) \
@interface name () <proto> \
@end
```
### OS_OBJECT_CLASS_IMPLEMENTS_PROTOCOL
&emsp;给 `name` 和 `proto` 添加 `OS_` 前缀。
```c++
#define OS_OBJECT_CLASS_IMPLEMENTS_PROTOCOL(name, proto) \
OS_OBJECT_CLASS_IMPLEMENTS_PROTOCOL_IMPL( \
        OS_OBJECT_CLASS(name), OS_OBJECT_CLASS(proto))
```
### OS_OBJECT_DECL_IMPL
&emsp;声明一个 `OS_name` 的协议，然后声明指向 `NSObject` 遵循 `OS_name` 协议的类型指针的别名 `name_t`。
```c++
#define OS_OBJECT_DECL_IMPL(name, ...) \
OS_OBJECT_DECL_PROTOCOL(name, __VA_ARGS__) \
typedef NSObject<OS_OBJECT_CLASS(name)> \
        * OS_OBJC_INDEPENDENT_CLASS name##_t
```
### OS_OBJECT_DECL_BASE
&emsp;声明 `OS_name` 类型，`name` 后面的参表示其继承的父类，然后有一个 `init` 函数。
```c++
#define OS_OBJECT_DECL_BASE(name, ...) \
        @interface OS_OBJECT_CLASS(name) : __VA_ARGS__ \
        - (instancetype)init OS_SWIFT_UNAVAILABLE("Unavailable in Swift"); \
        @end
```
### OS_OBJECT_DECL_IMPL_CLASS
&emsp;先声明一个类 `OS_name` 然后声明一个指向该类指针的别名 `name_t`。
```c++
#define OS_OBJECT_DECL_IMPL_CLASS(name, ...) \
        OS_OBJECT_DECL_BASE(name, ## __VA_ARGS__) \
        typedef OS_OBJECT_CLASS(name) \
                * OS_OBJC_INDEPENDENT_CLASS name##_t
```
### OS_OBJECT_DECL
&emsp;继承自 `<NSObject>` 协议的 `OS_name` 协议。
```c++
#define OS_OBJECT_DECL(name, ...) \
        OS_OBJECT_DECL_IMPL(name, <NSObject>)
```
### OS_OBJECT_DECL_SUBCLASS
&emsp;指定 `OS_name` 协议继承自 `OS_super` 协议。
```c++
#define OS_OBJECT_DECL_SUBCLASS(name, super) \
OS_OBJECT_DECL_IMPL(name, <OS_OBJECT_CLASS(super)>)
```
### OS_OBJECT_RETURNS_RETAINED
&emsp;如果存在 `ns_returns_retained` 属性，则 `OS_OBJECT_RETURNS_RETAINED` 宏定义为 `__attribute__((__ns_returns_retained__))`，否则仅是一个空的宏定义。
### OS_OBJECT_CONSUMED
&emsp;如果存在 `ns_consumed` 属性，则 `OS_OBJECT_CONSUMED` 宏定义为 `__attribute__((__ns_consumed__))`，否则仅是一个空的宏定义。
### OS_OBJECT_BRIDGE
&emsp;如果是 `objc_arc` 环境，则 `OS_OBJECT_BRIDGE` 宏定义为 `__bridge`，在 ARC 下对象类型转为 `void *` 时，需要加 `__bridge`，MRC 下则不需要。

&emsp;下面是一组在 Swift 中使用 ObjC 的宏，为了在 10.12 之前的 SDK 上向 Swift 中的 ObjC 对象提供向后部署，可以将 OS_object 类标记为 OS_OBJECT_OBJC_RUNTIME_VISIBLE。使用早于 OS X 10.12（iOS 10.0，tvOS 10.0，watchOS 3.0）的部署目标进行编译时，Swift编译器将仅在运行时（使用ObjC运行时）引用此类型。

### OS_OBJECT_DECL_CLASS
&emsp;最重要的一条宏，涉及到不同语言环境下的定义，如开篇的 `OS_OBJECT_DECL_CLASS(dispatch_object)` 所示。
```c++
#if OS_OBJECT_SWIFT3

// 1⃣️：SWift 环境下
#define OS_OBJECT_DECL_CLASS(name) \
        OS_OBJECT_DECL_SUBCLASS_SWIFT(name, object)
        
#elif OS_OBJECT_USE_OBJC

// 2⃣️：Objective-c 环境下
#define OS_OBJECT_DECL_CLASS(name) \
        OS_OBJECT_DECL(name)
        
#else

// 3⃣️：C/C++ 环境下
#define OS_OBJECT_DECL_CLASS(name) \
        typedef struct name##_s *name##_t
        
#endif
```
### OS_OBJECT_GLOBAL_OBJECT
&emsp;桥接转化，如 ARC 下 `NSObject *` 转化为 `void *`。
```c++
#define OS_OBJECT_GLOBAL_OBJECT(type, object) ((OS_OBJECT_BRIDGE type)&(object))
```
### os_retain
&emsp;`os_retain` 增加 os_object 的引用计数。
```c++
API_AVAILABLE(macos(10.10), ios(8.0))
OS_EXPORT OS_SWIFT_UNAVAILABLE("Can't be used with ARC")
void*
os_retain(void *object);
#if OS_OBJECT_USE_OBJC
// ObjC 下则是定义成一个宏，调用 retain 函数
#undef os_retain
#define os_retain(object) [object retain]
#endif
```
&emsp;在具有现代 Objective-C 运行时的平台上，这完全等同于向对象发送 -[retain] 消息。

&emsp;`object` 要 retain 的对象。

&emsp;`result` return 保留的对象。
### os_release
&emsp;`os_release` 减少 os_object 的引用计数。
```c++
API_AVAILABLE(macos(10.10), ios(8.0))
OS_EXPORT
void OS_SWIFT_UNAVAILABLE("Can't be used with ARC")
os_release(void *object);
#if OS_OBJECT_USE_OBJC
// ObjC 下则是定义成一个宏，调用 release 函数
#undef os_release
#define os_release(object) [object release]
#endif
```
&emsp;在具有现代 Objective-C 运行时的平台上，这完全等同于向对象发送 -[release] 消息。

&emsp;`object` 要释放的对象。

&emsp;<os/object.h> 文件到这里就全部结束了，下面我们接着看 <dispatch/object.h> 文件。

&emsp;接下来是分别针对不同的情况（Swift/Objective-C/C++/C）定义了一些 DISPATCH 前缀开头的宏，而宏定义的内容其中 Swift/Objective-C 相关部分来自 <os/object.h> 中的宏，C++/C 部分是来自它们的语言环境。（例如 C++ 下的 `static_cast` 函数的调用、结构体的继承等，C 下的直接取地址强制转换、联合体的使用等）
## DISPATCH_DECL/DISPATCH_DECL_SUBCLASS
&emsp;`DISPATCH_DECL` 默认使用 `dispatch_object` 作为继承的父类，`DISPATCH_DECL_SUBCLASS` 则是自行指定父类，并且针对不同的语言环境作了不同的定义。

&emsp;这里我们看 C++ 和 C 环境下（GCD 源码内部是此环境）。
+ C++ 环境下：
&emsp;上面我们我们已经看过 `struct dispatch_object_s` 声明，已知 `dispatch_object_t` 是指向 `struct dispatch_object_s` 的指针。
```c++
#define DISPATCH_DECL(name) \
typedef struct name##_s : public dispatch_object_s {} *name##_t
```
&emsp;如上篇中的 `DISPATCH_DECL(dispatch_queue)` 在此则转换为:
```c++
typedef struct dispatch_queue_s : public dispatch_object_s {} *dispatch_queue_t;
```
&emsp;即 `dispatch_queue_t` 是指向 `dispatch_queue_s` 结构体的指针，`dispatch_queue_s` 则是公开继承自 `dispatch_object_s` 结构体。 
```c++
#define DISPATCH_DECL_SUBCLASS(name, base) \
typedef struct name##_s : public base##_s {} *name##_t
```
&emsp;`DISPATCH_DECL_SUBCLASS` 则是指定父类，上面 `DISPATCH_DECL` 是默认继承自 `dispatch_object_s` 结构体。
+ C（Plain C）环境下：
&emsp;上面我们已经看到 `dispatch_object_t` 是一个联合体。
```c++
#define DISPATCH_DECL(name) typedef struct name##_s *name##_t
```
&emsp;如上篇中的 `DISPATCH_DECL(dispatch_queue)` 则直接转换为 `typedef struct dispatch_queue_s *dispatch_queue_t`，即 `dispatch_queue_t` 是指向 `dispatch_queue_s` 结构体的指针，对比 C++，此处的 `dispatch_queue_t` 仅是一个结构体。
```c++
#define DISPATCH_DECL_SUBCLASS(name, base) typedef base##_t name##_t
#define DISPATCH_GLOBAL_OBJECT(type, object) ((type)&(object))
```
&emsp;`DISPATCH_DECL_SUBCLASS` 指定指针指向类型，`DISPATCH_GLOBAL_OBJECT` 则是直接取地址后强制指针类型转换。


## DISPATCH_GLOBAL_OBJECT
&emsp;在 C++ 环境下是调用 `static_cast` 函数直接进行强制类型转换，OC 下则是进行桥接转换。
```c++
#define DISPATCH_GLOBAL_OBJECT(type, object) (static_cast<type>(&(object)))
```

## _dispatch_object_validate
&emsp;
```c++
DISPATCH_INLINE DISPATCH_ALWAYS_INLINE DISPATCH_NONNULL_ALL DISPATCH_NOTHROW
void
_dispatch_object_validate(dispatch_object_t object)
{
    void *isa = *(void *volatile*)(OS_OBJECT_BRIDGE void*)object;
    (void)isa;
}
```
&emsp;


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
