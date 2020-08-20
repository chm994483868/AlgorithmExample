#  iOS底层os_unfair_lock

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
所以 `spinlock_t` 其实是一个互斥锁，与它的名字自旋锁是不符的，其实以前它是 `OSSpinLock`，因为其安全问题被遗弃了 :
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
`mute_tt` 第一个元素是: `os_unfair_lock mLock;`，它是作为 `OSSpinLock` 的替代在 iOS 10.0 后出现的。
> 自旋锁 `OSSpinLock` 不是一个安全的锁，等待锁的线程会处于忙等（`busy-wait`）状态，一直占用着 `CPU` 资源；
  （类似一个 `while(1)` 循环一样，不停的查询锁的状态，注意区分 `runloop` 的机制，同样是阻塞，但是 `runloop` 是类似休眠的阻塞，不会耗费 `cpu` 资源，自旋锁的这种忙等机制使它相比其它锁效率更高，毕竟没有**唤醒-休眠**这些类似操作，从而能更快的处理事情）
 自旋锁目前已经被废弃了，它可能会导致优先级反转；
 例如 A/B 两个线程，A 的优先级大于 B 的，我们的本意是 A 的任务优先执行，但是使用 `OSSpinLock` 后，如果是 B 优先访问了共享资源获得了锁并加锁，而 A 线程再去访问共享资源的时候锁就会处于忙等状态，由于A 的优先级高它会一直占用 `cpu` 资源不会让出时间片，这样 B 一直不能获得 `cpu` 资源去执行任务，导致无法完成。
 
在 `usr/include/os/lock.h` 中看到 `os_unfair_lock` 的定义。

首先是一个宏定义告诉我们 `os_unfair_lock` 出现的时机。
看到 `os_unfair_lock` 是在 iOS 10.0 以后首次出现的。
```
#define OS_UNFAIR_LOCK_AVAILABILITY \
__API_AVAILABLE(macos(10.12), ios(10.0), tvos(10.0), watchos(3.0))
```

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
OS_UNFAIR_LOCK_AVAILABILITY
typedef struct os_unfair_lock_s {
    uint32_t _os_unfair_lock_opaque;
} os_unfair_lock, *os_unfair_lock_t;

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
  
针对不同的平台或者 C++ 版本以不同的方式来进行初始化: `(os_unfair_lock){0}`
1. (os_unfair_lock){0}
2. os_unfair_lock{}
3. os_unfair_lock()
4. {0}
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

`os_unfair_lock` 是用来代替 `OSSpinLock` 的，那先学习 `OSSpinLock`
要使用 `OSSpinLock` 需要先引入 `#import <libkern/OSAtomic.h>`，看到 `usr/include/libkern/OSSpinLockDeprecated.h` 名字后面的 `Deprecated` 强烈的提示着我们 `OSSpinLock` 以及被废弃了，不要再用它了。
查看 `OSSpinLockDeprecated.h` 文件里面到处是 `Deprecated` 和呼吁我们使用 `os_unfair_lock`。




// 待读:
[iOS锁-OSSpinLock与os_unfair_lock](https://www.jianshu.com/p/40adc41735b6)
[os_unfair_lock pthread_mutex](https://www.jianshu.com/p/6ff0dfe719bf)
