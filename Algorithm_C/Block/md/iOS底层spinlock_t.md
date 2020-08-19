#  iOS底层spinlock_t

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
所以 `spinlock_t` 其实是:
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
> `os_unfair_lock` 是作为 `OSSpinLock` 的替代出现的。`OSSpinLock` 因为其自身问题，目前已经被遗弃了。

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




