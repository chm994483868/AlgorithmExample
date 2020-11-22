# iOS 多线程知识体系构建(六)：GCD API（block.h、io.h）解析篇

> &emsp;那么继续学习 dispath 中也挺重要的 <dispatch/block.h> 文件。

## <dispatch/block.h>
&emsp;Dispatch block objects.
### dispatch_block_flags_t
&emsp;`DISPATCH_OPTIONS` 宏定义内容，即不同平台或者环境下的枚举定义：
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
&emsp;传递给 `dispatch_block_create*` 函数的标志。
+ `DISPATCH_BLOCK_BARRIER`：指示调度块对象（dispatch block object）在提交给 `DISPATCH_QUEUE_CONCURRENT` 队列时应充当屏障块（barrier block）的标志。有关详细信息，参考 `dispatch_barrier_async`。当直接调用调度块对象（dispatch block object）时，此标志无效。
+ `DISPATCH_BLOCK_DETACHED`：指示应该执行与当前执行上下文属性（例如 os_activity_t 和当前 IPC 请求的属性，如果有）无关的调度块对象（dispatch block object）的标志。关于 QoS 类别，其行为与 DISPATCH_BLOCK_NO_QOS 相同。如果直接调用，则块对象将在块主体的持续时间内从调用线程中删除其他属性（在应用分配给块对象的属性（如果有）之前）。如果提交给队列，则将使用队列的属性（或专门分配给该块对象的任何属性）执行该块对象。
+ `DISPATCH_BLOCK_ASSIGN_CURRENT`：指示应为调度块对象分配创建块对象时当前的执行上下文属性的标志。这适用于诸如 QOS 类，os_activity_t 的属性以及当前 IPC 请求的属性（如果有）。如果直接调用，则块对象将在块主体的持续时间内将这些属性应用于调用线程。如果将块对象提交到队列，则此标志替换将提交的块实例与提交时最新的执行上下文属性相关联的默认行为。如果使用 `DISPATCH_BLOCK_NO_QOS_CLASS` 或 `dispatch_block_create_with_qos_class` 分配了特定的 QOS 类，则该 QOS 类优先于此标志指示的 QOS 类分配。
+ `DISPATCH_BLOCK_NO_QOS_CLASS`：指示不应为调度块对象分配 QOS 类的标志。如果直接调用，则块对象将与调用线程的 QOS 类一起执行。如果将块对象提交到队列，这将替换默认行为，即在提交时将提交的块实例与当前的 QOS 类相关联。如果为特定的 QOS 类分配了 `dispatch_block_create_with_qos_class`，则忽略此标志。
+ `DISPATCH_BLOCK_INHERIT_QOS_CLASS`：指示执行提交到队列的调度块对象的标志应优先于分配给队列的 QOS 类，而不是分配给该块的 QOS 类（提交时与该块相关联的 resp。）。仅当所讨论的队列没有分配的 QOS 类时，才使用后者，只要这样做不会导致 QOS 类低于从队列的目标队列继承的 QOS 类。当将调度块对象提交到队列以进行异步执行时，此标志是默认设置；当直接调用调度块对象时，此标志无效。如果还传递了 `DISPATCH_BLOCK_ENFORCE_QOS_CLASS`，则将其忽略。
+ `DISPATCH_BLOCK_ENFORCE_QOS_CLASS`：指示执行提交到队列的调度块对象的标志应优先于分配给队列的 QOS 类，而不是分配给队列的 QOS 类，分配给该块的 QOS 类（在提交时与该块相关联）不会导致较低的 QOS 等级。当将调度块对象提交到队列以进行同步执行或直接调用调度块对象时，此标志是默认设置。
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
### dispatch_block_create
&emsp;根据现有块（existing block）和给定的标志（flags）在堆（heap）上创建一个新的调度块对象（dispatch block object）。
```c++
API_AVAILABLE(macos(10.10), ios(8.0))
DISPATCH_EXPORT DISPATCH_NONNULL2 DISPATCH_RETURNS_RETAINED_BLOCK
DISPATCH_WARN_RESULT DISPATCH_NOTHROW
dispatch_block_t
dispatch_block_create(dispatch_block_flags_t flags, dispatch_block_t block);
```
&emsp;提供的块被 `Block_copy` 到堆中，并由新创建的调度块对象保留。

&emsp;返回的调度块对象（dispatch block object）旨在通过 `dispatch_async` 和相关函数提交给调度队列，但也可以直接调用。两种操作都可以执行任意次，但只有第一次完成的调度块对象（dispatch block object）的执行才能用 `dispatch_block_wait` 等待，或用 `dispatch_block_notify` 来观察。

&emsp;

/*!
 * @function dispatch_block_create
 *
 * @abstract
 * Create a new dispatch block object on the heap from an existing block and the given flags.
 
 * @discussion
 * The provided block is Block_copy'ed to the heap and retained by the newly created dispatch block object.
 
 * The returned dispatch block object is intended to be submitted to a dispatch queue with dispatch_async() and related functions, but may also be invoked directly. Both operations can be performed an arbitrary number of times but only the first completed execution of a dispatch block object can be waited on with dispatch_block_wait() or observed with dispatch_block_notify().
 
 * If the returned dispatch block object is submitted to a dispatch queue, the submitted block instance will be associated with the QOS class current at the time of submission, unless one of the following flags assigned a specific QOS class (or no QOS class) at the time of block creation:
 *  - DISPATCH_BLOCK_ASSIGN_CURRENT
 *  - DISPATCH_BLOCK_NO_QOS_CLASS
 *  - DISPATCH_BLOCK_DETACHED
 
 * The QOS class the block object will be executed with also depends on the QOS class assigned to the queue and which of the following flags was specified or defaulted to:
 
 *  - DISPATCH_BLOCK_INHERIT_QOS_CLASS (default for asynchronous execution)
 *  - DISPATCH_BLOCK_ENFORCE_QOS_CLASS (default for synchronous execution)
 
 * See description of dispatch_block_flags_t for details.
 *
 * If the returned dispatch block object is submitted directly to a serial queue
 * and is configured to execute with a specific QOS class, the system will make
 * a best effort to apply the necessary QOS overrides to ensure that blocks
 * submitted earlier to the serial queue are executed at that same QOS class or
 * higher.
 *
 * @param flags
 * Configuration flags for the block object.
 * Passing a value that is not a bitwise OR of flags from dispatch_block_flags_t
 * results in NULL being returned.
 *
 * @param block
 * The block to create the dispatch block object from.
 *
 * @result
 * The newly created dispatch block object, or NULL.
 * When not building with Objective-C ARC, must be released with a -[release]
 * message or the Block_release() function.
 */


 

 





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
