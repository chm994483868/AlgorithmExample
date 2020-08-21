#  iOS_spinlock_t

> 使用 `OSSpinLock` 需要先引入 `#import <libkern/OSAtomic.h>`。
  看到 `usr/include/libkern/OSSpinLockDeprecated.h` 名字后面的 `Deprecated` 强烈的提示着我们 `OSSpinLock` 已经被废弃了。查看 `OSSpinLockDeprecated.h` 文件，里面到处是 `Deprecated` 和呼吁我们使用 `os_unfair_lock`。
  `OSSpinLock` 存在线程安全问题，它可能导致优先级反转问题，目前我们在任何情况下都不应该再使用它，我们可以使用 `apple` 在 `iOS 10.0` 后推出的 `os_unfair_lock` (作为 `OSSpinLock` 的替代) 。关于 `os_unfair_lock` 我们下一节再展开学习。

## 由于 `OSSpinLock API` 过于简单，我们首先看下使用示例：
***
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
    dispatch_queue_t globalQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0); // 取得一个全局（并行）队列
    self.sum = 0;

    dispatch_async(globalQueue, ^{ // 异步任务 1
        OSSpinLockLock(&_lock); // 获得锁
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        NSLog(@"⏰⏰⏰ %ld", self.sum);
        OSSpinLockUnlock(&_lock); // 解锁
    });
    
    dispatch_async(globalQueue, ^{ // 异步任务
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

// 把 lock 注释后:
// 一次运行:
⏰⏰⏰ 9064
⚽️⚽️⚽️ 13708
// 二次运行:
⏰⏰⏰ 11326
⚽️⚽️⚽️ 9933
// 三次运行:
⏰⏰⏰ 10307
⚽️⚽️⚽️ 5913
...
```
🌟🌟🌟
sum 属性使用 `atomic` 和 `nonatomic` 结果相同，下篇文章再分析 `atomic` 虽是原子操作，但是依然不是线程安全的原因。它的原子操作只是限于对它所修饰的变量在做 `set` 操作时加锁而已。

这里有一个有趣的点，我们来分析一下不加锁的情况下的打印：（这里我们把两条线层分别命名为 ⏰线程和⚽️线程）
1. 可以确定的是⏰线程和⚽️线程不会有任何一个可以打印 20000
2. ⏰线程和⚽️线程两者的打印都到了 10000 以上
3. ⏰线程或足球线程其中一个打印在 10000 以上一个在 10000 以下
> 情况 1 我们都能想到，因为⏰线程和⚽️线程是并发进行的，不会一个线程先把 sum 自增到 10000 然后另一个线程再把 sum 自增到 20000，只有加锁或者两条线程在串行队列中执行才行。

  以下情况时盲猜😬（目前还是小白，很多基础知识还没有，现有的知识还太浅😣）：

  情况 2 我们可能也好理解，两者至少打印到 10000，可是为什么都是 10000 以上呢，可以分析为某个时间点⏰线程持续自增，然后⚽️线程在这个时间点后执行循环时 sum 已经大于它上一次循环时的值了，然后⏰线程和⚽️线程下 sum 的值都是大于其上一次循环的值往下继续循环，最后两条线程的打印 sum 值都是大于 10000 的。
  
  情况 3 则理解比较麻烦，为什么其中一个可以小于 10000，可能是其中一个线程执行忽快忽慢造成的吗？ 还有如果被缩小一次，那不是也导致两条线程最终打印 sum 都会小于 10000 吗？忽快忽慢也不可能因为循环次数一定是 20000 次， sum 值只有一份，只要循环一次 sum 就会变大，不会存在被缩小的情况。可能是 寄存器和内存中的 sum 的值造成的吗，想到了 volatile 关键字。（暂时先分析到这里，分析不下去了 😖）

## 查看 `OSSpinLockDeprecated.h` 文件内容：
> 上面代码中每一行与 `OSSpinLock` 相关的代码都会有这样一行警告 ⚠️⚠️ `'OSSpinLock' is deprecated: first deprecated in iOS 10.0 - Use os_unfair_lock() from <os/lock.h> instead` 。

正是由下面的 `OSSPINLOCK_DEPRECATED` 所提示，在 4 大系统中都提示我们都不要再用 `OSSpinLock` 了。
**OSSPINLOCK_DEPRECATED：**
```
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
### 下面是不同情况下的 `OSSpinLock API` 实现

1. `#if !(defined(OSSPINLOCK_USE_INLINED) && OSSPINLOCK_USE_INLINED)` 为真不使用内联时的原始API：
  + `#define    OS_SPINLOCK_INIT    0` 初始化：
  ```objective-c
  /*! @abstract The default value for an <code>OSSpinLock</code>. OSSpinLock 的默认值是 0（unlocked 状态）
      @discussion
      The convention is that unlocked is zero, locked is nonzero. 惯例是: unlocked 时是零，locked 时时非零
   */
  #define    OS_SPINLOCK_INIT    0
  ```
  + `OSSpinLock` 数据类型：（目前对 `OSSpinLock` 的内部具体实现一无所有，看它的数据类型是 `int32_t` 很是疑惑）
  ```objective-c
  /*! @abstract Data type for a spinlock. 自旋锁的数据类型是 int32_t
      @discussion
      You should always initialize a spinlock to {@link OS_SPINLOCK_INIT} before
      using it. 在使用一个自旋锁之前，我们应该总是先把它初始化为 OS_SPINLOCK_INIT。（其实是对它赋值为数字 0 😣）
   */
  typedef int32_t OSSpinLock OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock);
  ```
  + 尝试加锁：
  ```
  /*! @abstract Locks a spinlock if it would not block 是否可以加锁
      @result
      Returns <code>false</code> if the lock was already held by another thread,
      <code>true</code> if it took the lock successfully. 如果锁已经被另一个线程所持有则返回 false，否则返回 true
   */
  OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock_trylock)
  __OSX_AVAILABLE_STARTING(__MAC_10_4, __IPHONE_2_0)
  bool    OSSpinLockTry( volatile OSSpinLock *__lock );
  ```
  + 加锁：
  ```
  /*! @abstract Locks a spinlock
      @discussion
      Although the lock operation spins, it employs various strategies to back
      off if the lock is held.
   */
  OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock_lock)
  __OSX_AVAILABLE_STARTING(__MAC_10_4, __IPHONE_2_0)
  void    OSSpinLockLock( volatile OSSpinLock *__lock );
  ```
  + 解锁：
  ```
  /*! @abstract Unlocks a spinlock */
  OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock_unlock)
  __OSX_AVAILABLE_STARTING(__MAC_10_4, __IPHONE_2_0)
  void    OSSpinLockUnlock( volatile OSSpinLock *__lock );
  ```
2. `OSSPINLOCK_USE_INLINED` 为真使用内联时：
  > Inline implementations of the legacy OSSpinLock interfaces in terms of the of the <os/lock.h> primitives. Direct use of those primitives is preferred.
  NOTE: the locked value of os_unfair_lock is implementation defined and subject to change, code that relies on the specific locked value used by the legacy OSSpinLock interface WILL break when using these inline implementations in terms of os_unfair_lock.
  
  `!OSSPINLOCK_USE_INLINED_TRANSPARENT` 为真时（不知道是干啥的 😣），把 `OSSpinLock` 转换为 `os_unfair_lock_t`:
  
  ***在函数前加 `OSSPINLOCK_INLINE` 告诉编译器尽最大努力保证被修饰的函数内联：***
  > ````objective-c
    #if __has_attribute(always_inline) // 尽最大努力保证函数内联
    #define OSSPINLOCK_INLINE static __inline
    #else
    #define OSSPINLOCK_INLINE static __inline __attribute__((__always_inline__))
    #endif

    #define OS_SPINLOCK_INIT 0 // 初始化为 0
    typedef int32_t OSSpinLock; // 类型依然是 int32_t

    #if  __has_extension(c_static_assert)
    _Static_assert(sizeof(OSSpinLock) == sizeof(os_unfair_lock),
            "Incompatible os_unfair_lock type"); // 如果 OSSpinLock 和 os_unfair_lock 内存长度不同，即类型不兼容，不能保证双方能正确的转换，直接断言❗️
    #endif
  > ```
  + `os_unfair_lock` 加锁：
    ```c++
    OSSPINLOCK_INLINE
    void
    OSSpinLockLock(volatile OSSpinLock *__lock)
    {
        os_unfair_lock_t lock = (os_unfair_lock_t)__lock; // 转换为 os_unfair_lock_t
        return os_unfair_lock_lock(lock);
    }
    ```
  + `os_unfair_lock` 尝试加锁：
      ```
      OSSPINLOCK_INLINE
      bool
      OSSpinLockTry(volatile OSSpinLock *__lock)
      {
          os_unfair_lock_t lock = (os_unfair_lock_t)__lock; // 转换为 os_unfair_lock_t
          return os_unfair_lock_trylock(lock);
      }
      ```
  + `os_unfair_lock` 解锁:
    ```
    OSSPINLOCK_INLINE
    void
    OSSpinLockUnlock(volatile OSSpinLock *__lock)
    {
        os_unfair_lock_t lock = (os_unfair_lock_t)__lock; // 转换为 os_unfair_lock_t
        return os_unfair_lock_unlock(lock);
    }
    ```
`#undef OSSPINLOCK_INLINE` 解除上面的宏定义。

3. 最后一种情况:
```c++
#define OS_SPINLOCK_INIT 0 // 初始化
typedef int32_t OSSpinLock OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock);
typedef volatile OSSpinLock *_os_nospin_lock_t
        OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock_t);

OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock_lock)
OS_NOSPIN_LOCK_AVAILABILITY
void _os_nospin_lock_lock(_os_nospin_lock_t lock); // nospin 非自旋锁吗？
#undef OSSpinLockLock // 解除上面的原始 API 的加锁的宏定义
#define OSSpinLockLock(lock) _os_nospin_lock_lock(lock)

OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock_trylock)
OS_NOSPIN_LOCK_AVAILABILITY
bool _os_nospin_lock_trylock(_os_nospin_lock_t lock);
#undef OSSpinLockTry // 解除上面的原始 API 的判断能否加锁的宏定义
#define OSSpinLockTry(lock) _os_nospin_lock_trylock(lock)

OSSPINLOCK_DEPRECATED_REPLACE_WITH(os_unfair_lock_unlock)
OS_NOSPIN_LOCK_AVAILABILITY
void _os_nospin_lock_unlock(_os_nospin_lock_t lock);
#undef OSSpinLockUnlock // 解除上面的原始 API 的解锁的宏定义
#define OSSpinLockUnlock(lock) _os_nospin_lock_unlock(lock)
```
至此 `OSSpinLockDeprecated.h` 文件代码结束，整体而言只有 4 条：

1. `OS_SPINLOCK_INIT` 初始化
2. `OSSpinLockTry()` 判断是否可以获取锁（尝试加锁），如果锁已经被另一个线程所持有则返回 false，否则返回 true
3. `OSSpinLockLock()` 加锁
4. `OSSpinLockUnlock` 解锁

## OSSpinLock 的问题

自旋锁 `OSSpinLock` 不是一个线程安全的锁，等待锁的线程会处于忙等（`busy-wait`）状态，一直占用着 `CPU` 资源；（类似一个 `while(1)` 循环一样，不停的查询锁的状态，注意区分 `runloop` 的机制，同样是阻塞，但是 `runloop` 是类似休眠的阻塞，不会耗费 `cpu` 资源，自旋锁的这种忙等机制使它相比其它锁效率更高，毕竟没有**唤醒-休眠**这些类似操作，从而能更快的处理事情）自旋锁目前已经被废弃了，它可能会导致优先级反转。

例如 A/B 两个线程，A 的优先级大于 B 的，我们的本意是 A 的任务优先执行，但是使用 `OSSpinLock` 后，如果是 B 优先访问了共享资源获得了锁并加锁，而 A 线程再去访问共享资源的时候锁就会处于忙等状态，由于A 的优先级高它会一直占用 `cpu` 资源不会让出时间片，这样 B 一直不能获得 `cpu` 资源去执行任务，导致无法完成。


> 《不再安全的 OSSpinLock》原文: 新版 iOS 中，系统维护了 5 个不同的线程优先级/QoS: background，utility，default，user-initiated，user-interactive。高优先级线程始终会在低优先级线程前执行，一个线程不会受到比它更低优先级线程的干扰。这种线程调度算法会产生潜在的优先级反转问题，从而破坏了 spin lock。
  具体来说，如果一个低优先级的线程获得锁并访问共享资源，这时一个高优先级的线程也尝试获得这个锁，它会处于 spin lock 的忙等状态从而占用大量 CPU。此时低优先级线程无法与高优先级线程争夺 CPU 时间，从而导致任务迟迟完不成、无法释放 lock。这并不只是理论上的问题，libobjc 已经遇到了很多次这个问题了，于是苹果的工程师停用了 OSSpinLock。
  苹果工程师 Greg Parker 提到，对于这个问题，一种解决方案是用 truly unbounded backoff 算法，这能避免 livelock 问题，但如果系统负载高时，它仍有可能将高优先级的线程阻塞数十秒之久；另一种方案是使用 handoff lock 算法，这也是 libobjc 目前正在使用的。锁的持有者会把线程 ID 保存到锁内部，锁的等待者会临时贡献出它的优先级来避免优先级反转的问题。理论上这种模式会在比较复杂的多锁条件下产生问题，但实践上目前还一切都好。
  libobjc 里用的是 Mach 内核的 thread_switch() 然后传递了一个 mach thread port 来避免优先级反转，另外它还用了一个私有的参数选项，所以开发者无法自己实现这个锁。另一方面，由于二进制兼容问题，OSSpinLock 也不能有改动。
  最终的结论就是，除非开发者能保证访问锁的线程全部都处于同一优先级，否则 iOS 系统中所有类型的自旋锁都不能再使用了。

**参考链接:🔗**
[不再安全的 OSSpinLock](https://blog.ibireme.com/2016/01/16/spinlock_is_unsafe_in_ios/)
[iOS 锁 部分一](https://www.jianshu.com/p/8ce323dbc491)

// 待读：
[如何深入理解 iOS 开发中的锁？](https://zhuanlan.zhihu.com/p/148788634)
[iOS 常见知识点（三）：Lock](https://www.jianshu.com/p/ddbe44064ca4)
