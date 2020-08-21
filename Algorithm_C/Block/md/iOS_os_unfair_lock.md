#  iOS_os_unfair_lock

## 引子
> 看到 `struct SideTable` 定义中第一个成员变量是 `spinlock_t slock;`， 这里展开对 `spinlock_t` 的学习。

```c++
struct SideTable {
    spinlock_t slock;
    ...
};
```
按住 `command` 点进去看到 `spinlock_t` 其实是使用 `using` 声明的一个模版类名：
```c++
#if DEBUG
#   define LOCKDEBUG 1
#else
#   define LOCKDEBUG 0
#endif

template <bool Debug> class mutex_tt;
using spinlock_t = mutex_tt<LOCKDEBUG>;
```
所以 `spinlock_t` 其实是一个互斥锁，与它的名字自旋锁是不符的，其实以前它是 `OSSpinLock`，因为其线程安全问题被遗弃了 :
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
接着我们先看一下 `nocopy_t` 定义（可参考 = delete 和 = default 使用文章来了解它俩的作用）：
```c++
// Mix-in for classes that must not be copied.
// 构造函数和析构函数使用编译器默认生成的，删除复制构造函数和赋值函数
class nocopy_t {
  private:
    nocopy_t(const nocopy_t&) = delete;
    const nocopy_t& operator=(const nocopy_t&) = delete;
  protected:
    constexpr nocopy_t() = default;
    ~nocopy_t() = default;
};
```
> `nocopy_t` 正如其名，删除编译器默认生成的复制构造函数和赋值函数，
  而构造函数和析构函数则依然使用编译器默认生成的。
  
## mutex_tt 学习
```c++
template <bool Debug>
class mutex_tt : nocopy_t {
    os_unfair_lock mLock;
    public:
    ....
};
```
`mute_tt` 第一个元素是: `os_unfair_lock mLock;`。

`os_unfair_lock` iOS 10.0 后推出的 用来代替 `OSSpinLock` 的，那先学习 `OSSpinLock`
要使用 `OSSpinLock` 需要先引入 `#import <libkern/OSAtomic.h>`，看到 `usr/include/libkern/OSSpinLockDeprecated.h` 名字后面的 `Deprecated` 强烈的提示着我们 `OSSpinLock` 以及被废弃了，不要再用它了。
查看 `OSSpinLockDeprecated.h` 文件里面到处是 `Deprecated` 和呼吁我们使用 `os_unfair_lock`。

## 正片

### 使用示例
在 `usr/include/os/lock.h` 中看到 `os_unfair_lock` 的定义。
使用 `os_unfair_lock` 首先需要引入 `#import <os/lock.h>` 

```objective-c
#import "ViewController.h"

#import <libkern/OSAtomic.h> // OSSpinLock
#import <os/lock.h> // os_unfair_lock

@interface ViewController ()

@property (nonatomic, assign) NSInteger sum;

//@property (nonatomic, assign) OSSpinLock lock;
@property (nonatomic, assign) os_unfair_lock unfairL;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    _sum = 100; // 只是给自动生成的 _sum 成员变量赋值，不会调用 sum 的 setter 函数
    self->_sum = 1000; // 只是给自动生成的 _sum 成员变量赋值，不会调用 sum 的 setter 函数
    
//    self.lock = OS_SPINLOCK_INIT;
    
    // 一定要区分 self. 中的 .，它和 C/C++ 中的 . 是不一样的
    // 开始一直疑惑 self.xxx，self 是一个指针，不是应该使用 self->xxx 吗
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

        // OSSpinLockLock(&_lock);
        
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
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        // OSSpinLockUnlock(&_lock);
        os_unfair_lock_unlock(&self->_unfairL); // 解锁

        NSLog(@"⏰⏰⏰ %ld", self.sum);
    });

    dispatch_async(globalQueue_DEFAULT, ^{
        __strong typeof(_self) self = _self;

        // OSSpinLockLock(&_lock2);
        os_unfair_lock_lock(&self->_unfairL); // 加锁
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        // OSSpinLockUnlock(&_lock2);
        os_unfair_lock_unlock(&self->_unfairL); // 解锁

        NSLog(@"⚽️⚽️⚽️ %ld", self.sum);
    });
}
@end
// 打印:
⚽️⚽️⚽️ 10000
⏰⏰⏰ 20000
```
### os_unfair_lock.h 文件分析
#### 推出时间
首先是一个宏定义告诉我们 `os_unfair_lock` 出现的时机。
看到 `os_unfair_lock` 是在 iOS 10.0 以后首次出现的，作为 `OSSpinLock` 的替代。
```c++
#define OS_LOCK_API_VERSION 20160309
#define OS_UNFAIR_LOCK_AVAILABILITY \
__API_AVAILABLE(macos(10.12), ios(10.0), tvos(10.0), watchos(3.0))
```
#### 摘要
```c++
/*!
 * @typedef os_unfair_lock
 *
 * @abstract
 * Low-level lock that allows waiters to block efficiently on contention.
 *
 * In general, higher level synchronization primitives such as those provided by
 * the pthread or dispatch subsystems should be preferred.
 *
 * ⚠️⚠️⚠️ The values stored in the lock should be considered opaque and implementation
 * defined, they contain thread ownership information that the system may use
 * to attempt to resolve priority inversions. 
 *
 * This lock must be unlocked from the same thread that locked it, attempts to
 * unlock from a different thread will cause an assertion aborting the process.
 * 
 * This lock must not be accessed from multiple processes or threads via shared
 * or multiply-mapped memory, the lock implementation relies on the address of
 * the lock value and owning process.
 *
 * Must be initialized with OS_UNFAIR_LOCK_INIT
 *
 * @discussion
 * Replacement for the deprecated OSSpinLock. Does not spin on contention but
 * waits in the kernel to be woken up by an unlock.
 *
 * As with OSSpinLock there is no attempt at fairness or lock ordering, e.g. an
 * unlocker can potentially immediately reacquire the lock before a woken up
 * waiter gets an opportunity to attempt to acquire the lock. This may be
 * advantageous for performance reasons, but also makes starvation of waiters a
 * possibility.
 */
```
1. `os_unfair_lock` 作为一个低等级低锁。一些高等级的锁才应该是我们的首选。
2. 必须使用加锁时的同一个线程来进行解锁，尝试使用不同的线程来解锁将导致断言中止进程。
3. 不能通过共享或多重映射内存从多个进程或线程访问此锁，锁的实现依赖于锁值的地址和所属进程。
4. 必须使用 `OS_UNFAIR_LOCK_INIT` 来进行初始化。

最重点关注下面这句:
> ⚠️⚠️⚠️ The values stored in the lock should be considered opaque and implementation
  defined, they contain thread ownership information that the system may use to attempt to
  resolve priority inversions.
  锁里面包含线程所有权信息来解决优先级反转问题。

#### os_unfair_lock_s 结构
typedef 定义别名：
`os_unfair_lock` 是一个 `os_unfair_lock_s` 结构体，`os_unfair_lock_t` 是一个 `os_unfair_lock_s` 指针，该结构体内部就一个 `uint32_t _os_unfair_lock_opaque` 成员变量。
```c++
OS_UNFAIR_LOCK_AVAILABILITY
typedef struct os_unfair_lock_s {
    uint32_t _os_unfair_lock_opaque;
} os_unfair_lock, *os_unfair_lock_t;
```

#### 初始化
针对不同的平台或者 C++ 版本以不同的方式来进行初始化: `(os_unfair_lock){0}`
1. `(os_unfair_lock){0}`
2. `os_unfair_lock{}`
3. `os_unfair_lock()`
4. `{0}`
```c++
#ifndef OS_UNFAIR_LOCK_INIT
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define OS_UNFAIR_LOCK_INIT ((os_unfair_lock){0})
#elif defined(__cplusplus) && __cplusplus >= 201103L
#define OS_UNFAIR_LOCK_INIT (os_unfair_lock{})
#elif defined(__cplusplus)
#define OS_UNFAIR_LOCK_INIT (os_unfair_lock())
#else
#define OS_UNFAIR_LOCK_INIT {0}
#endif
#endif // OS_UNFAIR_LOCK_INIT
```
#### `os_unfair_lock_lock` 加锁
```c++
/*!
 * @function os_unfair_lock_lock
 *
 * @abstract
 * Locks an os_unfair_lock. // 锁定 一个 os_unfair_lock
 *
 * @param lock
 * Pointer to an os_unfair_lock. // 参数是一个 os_unfair_lock 指针
 */
OS_UNFAIR_LOCK_AVAILABILITY
OS_EXPORT OS_NOTHROW OS_NONNULL_ALL
void os_unfair_lock_lock(os_unfair_lock_t lock);
```

#### `os_unfair_lock_trylock` 判断是否可加锁：
```c++
/*!
 * @function os_unfair_lock_trylock
 *
 * @abstract
 * Locks an os_unfair_lock if it is not already locked. // 锁定一个 os_unfair_lock，如果它是尚未锁定的
 *
 * @discussion
 * It is invalid to surround this function with a retry loop, if this function returns false, the program must be able to proceed without having acquired the lock, or it must call os_unfair_lock_lock() directly (a retry loop around os_unfair_lock_trylock() amounts to an inefficient implementation of os_unfair_lock_lock() that hides the lock waiter from the system and prevents resolution of priority inversions).
 用重试循环包围此函数是无效的，如果此函数返回false，程序必须能够在没有获得锁的情况下继续运行，或者必须直接调用 os_unfair_lock_lock（）。（围绕os_unfair_lock_trylock（）的重试循环等于os_unfair_lock_lock（）的低效实现，该实现将 lock waiter 从系统中隐藏并解决了优先级反转问题）
 *
 * @param lock
 * Pointer to an os_unfair_lock. // 参数是一个 os_unfair_lock 指针
 *
 * @result
 * Returns true if the lock was succesfully locked and false if the lock was
 * already locked. // 锁定成功返回 true，如果已经被锁定则返回 false
 */
OS_UNFAIR_LOCK_AVAILABILITY
OS_EXPORT OS_NOTHROW OS_WARN_RESULT OS_NONNULL_ALL
bool os_unfair_lock_trylock(os_unfair_lock_t lock);
```
#### `os_unfair_lock_unlock` 解锁：
```c++
/*!
 * @function os_unfair_lock_unlock
 *
 * @abstract
 * Unlocks an os_unfair_lock. // 解锁
 *
 * @param lock
 * Pointer to an os_unfair_lock.
 */
OS_UNFAIR_LOCK_AVAILABILITY
OS_EXPORT OS_NOTHROW OS_NONNULL_ALL
void os_unfair_lock_unlock(os_unfair_lock_t lock);
```
#### `os_unfair_lock_assert_owner` 判断当前线程是否是 `os_unfair_lock` 的所有者，否则触发断言
```c++
/*!
 * @function os_unfair_lock_assert_owner
 *
 * @abstract
 * Asserts that the calling thread is the current owner of the specified unfair lock.
 *
 * @discussion
 * If the lock is currently owned by the calling thread, this function returns. // 如果锁当前由调用线程拥有，则此函数返回
 *
 * If the lock is unlocked or owned by a different thread, this function asserts and terminates the process.
 // 如果锁是由另一个线程解锁或拥有的，则执行断言
 *
 * @param lock
 * Pointer to an os_unfair_lock.
 */
OS_UNFAIR_LOCK_AVAILABILITY
OS_EXPORT OS_NOTHROW OS_NONNULL_ALL
void os_unfair_lock_assert_owner(os_unfair_lock_t lock);
```

#### `os_unfair_lock_assert_not_owner` 与上相反，如果是所有者则触发断言
```c++
/*!
 * @function os_unfair_lock_assert_not_owner
 *
 * @abstract
 * Asserts that the calling thread is not the current owner of the specified unfair lock.
 *
 * @discussion
 * If the lock is unlocked or owned by a different thread, this function returns.
 *
 * If the lock is currently owned by the current thread, this function assertsand terminates the process.
 *
 * @param lock
 * Pointer to an os_unfair_lock.
 */
OS_UNFAIR_LOCK_AVAILABILITY
OS_EXPORT OS_NOTHROW OS_NONNULL_ALL
void os_unfair_lock_assert_not_owner(os_unfair_lock_t lock);
```
测试：
```objective-c
dispatch_async(globalQueue_DEFAULT, ^{
    os_unfair_lock_assert_owner(&self->_unfairL);
});
os_unfair_lock_assert_not_owner(&self->_unfairL);
```

// 待读:
[iOS锁-OSSpinLock与os_unfair_lock](https://www.jianshu.com/p/40adc41735b6)
[os_unfair_lock pthread_mutex](https://www.jianshu.com/p/6ff0dfe719bf)
