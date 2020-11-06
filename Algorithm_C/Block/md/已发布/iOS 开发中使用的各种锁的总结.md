# iOS 开发中使用的各种锁的总结

> &emsp;

## spinlock_t

> &emsp;使用 OSSpinLock 需要先引入 #import <libkern/OSAtomic.h>。看到 usr/include/libkern/OSSpinLockDeprecated.h 名字后面的 Deprecated 强烈的提示着我们 OSSpinLock 已经不赞成使用了。
> &emsp;查看 OSSpinLockDeprecated.h 文件内容 OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock) 提示我们使用 os_unfair_lock 代替 OSSpinLock。
> &emsp;OSSpinLock 存在线程安全问题，它可能导致优先级反转问题，目前我们在任何情况下都不应该再使用它，我们可以使用 apple 在 iOS 10.0 后推出的 os_unfair_lock (作为 OSSpinLock 的替代) 。关于 os_unfair_lock 我们下一节展开学习。

### OSSpinLock API 简单使用
&emsp;`OSSpinLock API` 很简单，首先看下使用示例。
```objective-c
#import "ViewController.h"
#import <libkern/OSAtomic.h> // 引入 OSSpinLock

@interface ViewController ()

@property (nonatomic, assign) NSInteger sum;
@property (nonatomic, assign) OSSpinLock lock;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    self.lock = OS_SPINLOCK_INIT; // 初始化锁
    dispatch_queue_t globalQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0); // 取得一个全局并发队列
    self.sum = 0; // sum 从 0 开始

    dispatch_async(globalQueue, ^{ // 异步任务 1
        OSSpinLockLock(&_lock); // 获得锁
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        NSLog(@"⏰⏰⏰ %ld", self.sum);
        OSSpinLockUnlock(&_lock); // 解锁
    });
    
    dispatch_async(globalQueue, ^{ // 异步任务 2
        OSSpinLockLock(&_lock); // 获得锁
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        NSLog(@"⚽️⚽️⚽️ %ld", self.sum);
        OSSpinLockUnlock(&_lock); // 解锁
    });
}

@end

// 打印 🖨️：
⏰⏰⏰ 10000
⚽️⚽️⚽️ 20000

// 把 lock 注释后，运行多次可总结出如下三种不同情况，
// 其中只有一个任务达到了 10000 以上另一个是 10000 以下，另一种是两者都达到了 10000 以上

// 情况 1:
⏰⏰⏰ 9064
⚽️⚽️⚽️ 13708
// 情况 2:
⏰⏰⏰ 11326
⚽️⚽️⚽️ 9933
// 情况 3:
⏰⏰⏰ 10906
⚽️⚽️⚽️ 11903
...
```
> &emsp;sum 属性使用 atomic 或 nonatomic 时结果相同，atomic 虽是原子操作，但它不是线程安全的，它的原子性只是限于对它所修饰的变量在 setter 和 getter 时加锁而已，当遇到 self.sum++ 或者 self.sum = self.sum + 1 等等这种复合操作时，atomic 是完全保证不了线程安全的。

&emsp;在不加锁情况下打印的数字有一些有趣的点，这里分析一下：（假设在全局并发队列下的两个 `dispatch_async` 任务都开启了新线程，并把两条线分别命名为 “⏰线程” 和 “⚽️线程”）

1. 可以确定的是 ⏰线程 和 ⚽️线程 不会有任何一个可以打印 `20000`。
2. ⏰线程 和 ⚽️线程 两者的打印都到了 `10000` 以上。
3. ⏰线程 或 ⚽️线程 其中一个打印在 `10000` 以上一个在 `10000` 以下。

&emsp;情况 1 我们都能想到，因为 ⏰线程 和 ⚽️线程 是并发进行的，不会存在一个线程先把 `sum` 自增到 `10000` 然后另一个线程再把 `sum` 自增到 `20000`，只有加锁或者 `self.sum` 自增的任务在串行队列中执行才行。
&emsp;情况 2 我们可能也好理解，两者都打印到 `10000` 以上，可以分析为某个时间点 ⏰线程 持续自增，然后 ⚽️线程 在这个时间点后执行循环时 `sum` 已经大于它上一次循环时的值了，然后 ⏰线程 和 ⚽️线程 下 `sum` 的值都是以大于其上一次循环的值往下继续循环，最后两条线程的打印 `sum` 值都是大于 `10000` 的。
&emsp;情况 3 则理解比较麻烦，为什么其中一个可以小于 `10000`，可能是其中一个线程执行忽快忽慢造成的吗？ 还有如果被缩小一次，那不是会导致两条线程最终打印 `sum` 都会小于 `10000` 吗？可能是 `self.sum` 读取时是从寄存器或内存中读取造成的吗？想到了 `volatile` 关键字。（暂时先分析到这里，分析不下去了)

### OSSpinLockDeprecated.h 文件内容
&emsp;下面直接查看 `OSSpinLockDeprecated.h` 中的代码内容。
&emsp;上面示例代码中每一行与 `OSSpinLock` 相关的代码都会有这样一行警告 ⚠️⚠️ `'OSSpinLock' is deprecated: first deprecated in iOS 10.0 - Use os_unfair_lock() from <os/lock.h> instead` 。正是由下面的 `OSSPINLOCK_DEPRECATED` 所提示，在 4 大系统中都提示我们都不要再用 `OSSpinLock` 了。
```c++
#ifndef OSSPINLOCK_DEPRECATED

#define OSSPINLOCK_DEPRECATED 1
#define OSSPINLOCK_DEPRECATED_MSG(_r) "Use " #_r "() from <os/lock.h> instead"
#define OSSPINLOCK_DEPRECATED_REPLACE_WITH(_r) \
    __OS_AVAILABILITY_MSG(macosx, deprecated=10.12, OSSPINLOCK_DEPRECATED_MSG(_r)) \
    __OS_AVAILABILITY_MSG(ios, deprecated=10.0, OSSPINLOCK_DEPRECATED_MSG(_r)) \
    __OS_AVAILABILITY_MSG(tvos, deprecated=10.0, OSSPINLOCK_DEPRECATED_MSG(_r)) \
    __OS_AVAILABILITY_MSG(watchos, deprecated=3.0, OSSPINLOCK_DEPRECATED_MSG(_r))
    
#else

#undef OSSPINLOCK_DEPRECATED
#define OSSPINLOCK_DEPRECATED 0
#define OSSPINLOCK_DEPRECATED_REPLACE_WITH(_r)

#endif
```
&emsp;下面是不同情况下的 `OSSpinLock API` 实现:
1. `#if !(defined(OSSPINLOCK_USE_INLINED) && OSSPINLOCK_USE_INLINED)` 为真不使用内联时的原始 API：
+ `#define    OS_SPINLOCK_INIT    0` 初始化。
```c++
/*! @abstract The default value for an <code>OSSpinLock</code>. OSSpinLock 的默认值是 0（unlocked 状态）
    @discussion
    The convention is that unlocked is zero, locked is nonzero. 惯例是: unlocked 时是零，locked 时时非零
 */
#define    OS_SPINLOCK_INIT    0
```
+ `OSSpinLock` 数据类型。
```c++
/*! @abstract Data type for a spinlock. 自旋锁的数据类型是 int32_t
    @discussion
    You should always initialize a spinlock to {@link OS_SPINLOCK_INIT} before using it. 
    在使用一个自旋锁之前，我们应该总是先把它初始化为 OS_SPINLOCK_INIT。（其实是对它赋值为数字 0）
 */
typedef int32_t OSSpinLock OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock);
```
+ `OSSpinLockTry` 尝试加锁，`bool` 类型的返回值表示是否加锁成功，即使加锁失败也不会阻塞线程。
```c++
/*! @abstract Locks a spinlock if it would not block. 如果一个 spinlock 未锁定，则锁定它。
    @result
    Returns <code>false</code> if the lock was already held by another thread,
    <code>true</code> if it took the lock successfully. 
    如果锁已经被另一个线程所持有则返回 false，否则返回 true 表示加锁成功。
 */
OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock_trylock)
__OSX_AVAILABLE_STARTING(__MAC_10_4, __IPHONE_2_0)
bool    OSSpinLockTry( volatile OSSpinLock *__lock );
```
+ `OSSpinLockLock` 加锁。
```
/*! @abstract Locks a spinlock. 锁定一个 spinlock
    @discussion
    Although the lock operation spins, it employs various strategies to back
    off if the lock is held.
    尽管锁定操作旋转，（当加锁失败时会一直处于等待状态，一直到获取到锁为止，获取到锁之前会一直处于阻塞状态）
    它采用各种策略来支持如果加锁成功，则关闭旋转。
 */
OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock_lock)
__OSX_AVAILABLE_STARTING(__MAC_10_4, __IPHONE_2_0)
void    OSSpinLockLock( volatile OSSpinLock *__lock );
```
+ `OSSpinLockUnlock` 解锁。
```
/*! @abstract Unlocks a spinlock */
OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock_unlock)
__OSX_AVAILABLE_STARTING(__MAC_10_4, __IPHONE_2_0)
void    OSSpinLockUnlock( volatile OSSpinLock *__lock );
```
2. `OSSPINLOCK_USE_INLINED` 为真使用内联，内联实现是使用 `os_unfair_lock_t` 代替 `OSSpinLock`。
> &emsp;Inline implementations of the legacy OSSpinLock interfaces in terms of the of the <os/lock.h> primitives. Direct use of those primitives is preferred.
> &emsp;NOTE: the locked value of os_unfair_lock is implementation defined and subject to change, code that relies on the specific locked value used by the legacy OSSpinLock interface WILL break when using these inline implementations in terms of os_unfair_lock.
> 
> &emsp;就 <os/lock.h> 中的原始接口而言，此处是原始 OSSpinLock 接口的内联实现。最好直接使用这些 primitives。
> &emsp;NOTE: os_unfair_lock 的锁定值是实现定义的，可能会更改。当使用这些内联实现时，依赖于旧版 OSSpinLock 接口使用的特定锁定值的代码会中断 os_unfair_lock。

&emsp;在函数前加 `OSSPINLOCK_INLINE` 告诉编译器尽最大努力保证被修饰的函数内联实现。
```c++
  #if __has_attribute(always_inline) // 尽最大努力保证函数内联实现
  #define OSSPINLOCK_INLINE static __inline
  #else
  #define OSSPINLOCK_INLINE static __inline __attribute__((__always_inline__))
  #endif

  #define OS_SPINLOCK_INIT 0 // 初始化为 0
  typedef int32_t OSSpinLock; // 类型依然是 int32_t

  #if  __has_extension(c_static_assert)
  // 如果 OSSpinLock 和 os_unfair_lock 内存长度不同，即类型不兼容，不能保证双方能正确的转换，直接断言。
  _Static_assert(sizeof(OSSpinLock) == sizeof(os_unfair_lock), "Incompatible os_unfair_lock type"); 
  #endif
```
+ `os_unfair_lock` 加锁。
```c++
  OSSPINLOCK_INLINE
  void
  OSSpinLockLock(volatile OSSpinLock *__lock)
  {
      // 转换为 os_unfair_lock_t。
      os_unfair_lock_t lock = (os_unfair_lock_t)__lock;
      return os_unfair_lock_lock(lock);
  }
```
+ `os_unfair_lock` 尝试加锁。
```c++
OSSPINLOCK_INLINE
bool
OSSpinLockTry(volatile OSSpinLock *__lock)
{
    // 转换为 os_unfair_lock_t。
    os_unfair_lock_t lock = (os_unfair_lock_t)__lock;
    return os_unfair_lock_trylock(lock);
}
```
+ `os_unfair_lock` 解锁。
```c++
OSSPINLOCK_INLINE
void
OSSpinLockUnlock(volatile OSSpinLock *__lock)
{
    // 转换为 os_unfair_lock_t。
    os_unfair_lock_t lock = (os_unfair_lock_t)__lock;
    return os_unfair_lock_unlock(lock);
}
```
&emsp;`#undef OSSPINLOCK_INLINE` 解除上面的宏定义。

3. 最后一种情况。
```c++
#define OS_SPINLOCK_INIT 0 // 初始化
typedef int32_t OSSpinLock OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock); // 类型 int32_t
typedef volatile OSSpinLock *_os_nospin_lock_t
        OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock_t); // 命名 _os_nospin_lock_t

OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock_lock)
OS_NOSPIN_LOCK_AVAILABILITY
void _os_nospin_lock_lock(_os_nospin_lock_t lock); // 加锁
#undef OSSpinLockLock // 解除上面的原始 API 的加锁的宏定义
#define OSSpinLockLock(lock) _os_nospin_lock_lock(lock)

OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock_trylock)
OS_NOSPIN_LOCK_AVAILABILITY
bool _os_nospin_lock_trylock(_os_nospin_lock_t lock); // 尝试加锁
#undef OSSpinLockTry // 解除上面的原始 API 的判断能否加锁的宏定义
#define OSSpinLockTry(lock) _os_nospin_lock_trylock(lock)

OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock_unlock)
OS_NOSPIN_LOCK_AVAILABILITY
void _os_nospin_lock_unlock(_os_nospin_lock_t lock); // 解锁
#undef OSSpinLockUnlock // 解除上面的原始 API 的解锁的宏定义
#define OSSpinLockUnlock(lock) _os_nospin_lock_unlock(lock)
```
&emsp;至此 `OSSpinLockDeprecated.h` 文件代码结束，整体而言只有 4 条。

1. `OS_SPINLOCK_INIT` 初始化。
2. `OSSpinLockTry()` 尝试加锁，如果锁已经被另一个线程所持有则返回 false，否则返回 true，即使加锁失败也不会阻塞当前线程。
3. `OSSpinLockLock()` 加锁，加锁失败会一直等待，会阻塞当前线程。
4. `OSSpinLockUnlock` 解锁。

### OSSpinLock 的安全问题
&emsp;自旋锁 `OSSpinLock` 不是一个线程安全的锁，等待锁的线程会处于忙等（`busy-wait`）状态，一直占用着 `CPU` 资源。（类似一个 `while(1)` 循环一样，不停的查询锁的状态，注意区分 `runloop` 的机制，同样是阻塞，但是 `runloop` 是类似休眠的阻塞，不会耗费 `CPU` 资源，自旋锁的这种忙等机制使它相比其它锁效率更高，毕竟没有 **唤醒-休眠** 这些类似操作，从而能更快的处理事情。）自旋锁目前已经被废弃了，它可能会导致优先级反转。

&emsp;例如 `A/B` 两个线程，`A` 的优先级大于 `B` 的，我们的本意是 `A` 的任务优先执行，但是使用 `OSSpinLock` 后，如果是 `B` 优先访问了共享资源获得了锁并加锁，而 `A` 线程再去访问共享资源的时候锁就会处于忙等状态，由于 `A` 的优先级高它会一直占用 `CPU` 资源不会让出时间片，这样 `B` 一直不能获得 `CPU` 资源去执行任务，导致无法完成。

> &emsp;《不再安全的 OSSpinLock》原文: 新版 iOS 中，系统维护了 5 个不同的线程优先级/QoS: background，utility，default，user-initiated，user-interactive。高优先级线程始终会在低优先级线程前执行，一个线程不会受到比它更低优先级线程的干扰。这种线程调度算法会产生潜在的优先级反转问题，从而破坏了 spin lock。
具体来说，如果一个低优先级的线程获得锁并访问共享资源，这时一个高优先级的线程也尝试获得这个锁，它会处于 spin lock 的忙等状态从而占用大量 CPU。此时低优先级线程无法与高优先级线程争夺 CPU 时间，从而导致任务迟迟完不成、无法释放 lock。这并不只是理论上的问题，libobjc 已经遇到了很多次这个问题了，于是苹果的工程师停用了 OSSpinLock。
苹果工程师 Greg Parker 提到，对于这个问题，一种解决方案是用 truly unbounded backoff 算法，这能避免 livelock 问题，但如果系统负载高时，它仍有可能将高优先级的线程阻塞数十秒之久；另一种方案是使用 handoff lock 算法，这也是 libobjc 目前正在使用的。锁的持有者会把线程 ID 保存到锁内部，锁的等待者会临时贡献出它的优先级来避免优先级反转的问题。理论上这种模式会在比较复杂的多锁条件下产生问题，但实践上目前还一切都好。
libobjc 里用的是 Mach 内核的 thread_switch() 然后传递了一个 mach thread port 来避免优先级反转，另外它还用了一个私有的参数选项，所以开发者无法自己实现这个锁。另一方面，由于二进制兼容问题，OSSpinLock 也不能有改动。
最终的结论就是，除非开发者能保证访问锁的线程全部都处于同一优先级，否则 iOS 系统中所有类型的自旋锁都不能再使用了。-[《不再安全的 OSSpinLock》](https://blog.ibireme.com/2016/01/16/spinlock_is_unsafe_in_ios/)

## os_unfair_lock
> &emsp;`os_unfair_lock` 设计宗旨是用于替换 `OSSpinLock`，从 `iOS 10` 之后开始支持，跟 `OSSpinLock` 不同，等待 `os_unfair_lock` 的线程会处于休眠状态（类似 `Runloop` 那样），不是忙等（`busy-wait`）。

### os_unfair_lock 引子
&emsp;看到 `struct SideTable` 定义中第一个成员变量是 `spinlock_t slock;`， 这里展开对 `spinlock_t` 的学习。
```c++
struct SideTable {
    spinlock_t slock;
    ...
};
```
&emsp;`spinlock_t` 其实是使用 `using` 声明的一个模版类。
```c++
#if DEBUG
#   define LOCKDEBUG 1
#else
#   define LOCKDEBUG 0
#endif

template <bool Debug> class mutex_tt;
using spinlock_t = mutex_tt<LOCKDEBUG>;
```
&emsp;所以 `spinlock_t` 其实是一个互斥锁，与它的名字自旋锁是不符的，其实以前它是 `OSSpinLock`，因为其优先级反转导致的安全问题而被遗弃了。
```c++
template <bool Debug>
class mutex_tt : nocopy_t { // 继承自 nocopy_t
    os_unfair_lock mLock;
 public:
    constexpr mutex_tt() : mLock(OS_UNFAIR_LOCK_INIT) {
        lockdebug_remember_mutex(this);
    }
    ...
};
```
&emsp;`nocopy_t` 正如其名，删除编译器默认生成的复制构造函数和赋值操作符，而构造函数和析构函数则依然使用编译器默认生成的。
```c++
// Mix-in for classes that must not be copied.
// 构造函数 和 析构函数 使用编译器默认生成的，删除 复制构造函数 和 赋值操作符。
class nocopy_t {
  private:
    nocopy_t(const nocopy_t&) = delete;
    const nocopy_t& operator=(const nocopy_t&) = delete;
  protected:
    constexpr nocopy_t() = default;
    ~nocopy_t() = default;
};
```

&emsp;`mute_tt` 类的第一个成员变量是: `os_unfair_lock mLock`。
### os_unfair_lock 正片
&emsp;在 `usr/include/os/lock.h` 中看到 `os_unfair_lock` 的定义，使用 `os_unfair_lock` 首先需要引入 `#import <os/lock.h>` 。

### os_unfair_lock API 简单使用
&emsp;`os_unfair_lock API` 很简单，首先看下使用示例。
```c++
#import "ViewController.h"
#import <os/lock.h> // os_unfair_lock

@interface ViewController ()

@property (nonatomic, assign) NSInteger sum;
@property (nonatomic, assign) os_unfair_lock unfairL;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    _sum = 100; // 只是给自动生成的 _sum 成员变量赋值，不会调用 sum 的 setter 函数
    self->_sum = 1000; // 只是给自动生成的 _sum 成员变量赋值，不会调用 sum 的 setter 函数
    
    // 一定要区分 self. 中的 .，它和 C/C++ 中的 . 是不一样的，OC 中的 . 是调用 getter/setter 函数。
    // 开始一直疑惑 self.xxx，self 是一个指针，不是应该使用 self->xxx 吗?
    // 在 OC 中，应该是 self->_xxx，_xxx 是 xxx 属性自动生成的对应的成员变量 _xxx
    // self 是一个结构体指针，所以访问指针的成员变量，只能是 self->_xxx，不能是 self->xxx
    
    // 等号左边的 "self.unfairL = xxx" 相当于调用 unfairL 的 setter 函数给它赋值
    // 即 [self setUnfairL:OS_UNFAIR_LOCK_INIT];
    
    // 等号右边的 "xxx = self.unfaiL" 或者 "self.unfairL" 的使用，
    // 相当于调用 unfairL 的 getter 函数，读取它的值
    // 相当于调用 getter 函数：[self unfairL]
    
    /*
     // os_unfair_lock 是一个结构体
     typedef struct os_unfair_lock_s {
     uint32_t _os_unfair_lock_opaque;
     } os_unfair_lock, *os_unfair_lock_t;
     */
     
    self.unfairL = OS_UNFAIR_LOCK_INIT; // 初始化
    
    dispatch_queue_t globalQueue_DEFAULT = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

    self.sum = 0;
    
    __weak typeof(self) _self = self;
    dispatch_async(globalQueue_DEFAULT, ^{
        __strong typeof(_self) self = _self;

        // 不是使用 &self.unfairL，
        // 这样使用相当于 &[self unfairL]
        // 不能这样取地址
        // &[self unfairL]，
        // 报错: Cannot take the address of an rvalue of type 'os_unfair_lock'
        // 报错: 不能获取类型为 "os_unfair_lock" 的右值的地址
        // &self.unfairL;
        // 报错: Address of property expression requested
        // 只能使用 &self->_unfairL
        // 先拿到成员变量 _unfairL，然后再取地址
        
        os_unfair_lock_lock(&self->_unfairL); // 加锁
        // os_unfair_lock_lock(&self->_unfairL); // 重复加锁会直接 crash
        
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }

        os_unfair_lock_unlock(&self->_unfairL); // 解锁

        NSLog(@"⏰⏰⏰ %ld", self.sum);
    });

    dispatch_async(globalQueue_DEFAULT, ^{
        __strong typeof(_self) self = _self;

        os_unfair_lock_lock(&self->_unfairL); // 加锁
        
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        
        os_unfair_lock_unlock(&self->_unfairL); // 解锁

        NSLog(@"⚽️⚽️⚽️ %ld", self.sum);
    });
}
@end

// 打印:
⚽️⚽️⚽️ 10000
⏰⏰⏰ 20000
```
### os_unfair_lock.h 文件内容




## 参考链接
**参考链接:🔗**
+ [自旋锁](https://baike.baidu.com/item/自旋锁/9137985?fr=aladdin)
+ [不再安全的 OSSpinLock](https://blog.ibireme.com/2016/01/16/spinlock_is_unsafe_in_ios/)
+ [iOS 锁 部分一](https://www.jianshu.com/p/8ce323dbc491)
+ [如何深入理解 iOS 开发中的锁？](https://zhuanlan.zhihu.com/p/148788634)
+ [iOS 常见知识点（三）：Lock](https://www.jianshu.com/p/ddbe44064ca4)
+ [iOS锁-OSSpinLock与os_unfair_lock](https://www.jianshu.com/p/40adc41735b6)
+ [os_unfair_lock pthread_mutex](https://www.jianshu.com/p/6ff0dfe719bf)
+ [iOS 锁 部分一](https://www.jianshu.com/p/8ce323dbc491)
