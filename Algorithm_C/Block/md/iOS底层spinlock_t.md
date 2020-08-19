#  iOS底层spinlock_t

> 看到 `struct SideTable` 定义中第一个成员变量是 `spinlock_t slock;`， 这里展开对 `spinlock_t` 的学习。

```objective-c
struct SideTable {
    spinlock_t slock;
    ...
};
```
按住 `command` 点进去看到 `spinlock_t` 其实使用 `using` 声明的一个模版类名：
```objective-c

#if DEBUG
#   define LOCKDEBUG 1
#else
#   define LOCKDEBUG 0
#endif

template <bool Debug> class mutex_tt;
using spinlock_t = mutex_tt<LOCKDEBUG>;
```
所以 `spinlock_t` 其实是:
```objective-c

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
```objective-c
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




