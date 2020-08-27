#  iOS_@synchronized

## 特点
1. `objc4-750` 版本之前（`iOS 12` 之前）`@synchronized` 是一个基于 `pthread_mutex_t` 封装的递归锁，之后实现则发生了改变，底层的封装变为了 `os_unfair_lock`。下面验证它，在 `@synchronized` 打断点，并且打开 `Debug-> Debug Workflow -> Always Show Disassembly`:
  ```objective-c
  #pragma mark - Private Methods

  - (void)recuresiveAction {
      // ➡️ 在下面 @synchronized 上打断点  
      @synchronized ([self class]) {
          NSLog(@"🌰🌰🌰 count = %d", count);
          if (count > 0) {
              count--;
              
              [self recuresiveAction];
          }
      }
  }
  
  // 汇编 objc_Simple`-[ViewController recuresiveAction]:
  ...
  0x10868fc4b <+43>:  callq  0x108690360               ; symbol stub for: objc_sync_enter // 👈 看到调用了 objc_sync_enter 函数
  0x10868fc50 <+48>:  movl   0x4952(%rip), %esi        ; count
  0x10868fc56 <+54>:  leaq   0x23fb(%rip), %rdi        ; @
  0x10868fc5d <+61>:  xorl   %ecx, %ecx
  0x10868fc5f <+63>:  movb   %cl, %dl
  0x10868fc61 <+65>:  movl   %eax, -0x2c(%rbp)
  0x10868fc64 <+68>:  movb   %dl, %al
  0x10868fc66 <+70>:  callq  0x1086902f4               ; symbol stub for: NSLog
  0x10868fc6b <+75>:  jmp    0x10868fc70               ; <+80> at ViewController.m:300:19
  0x10868fc70 <+80>:  cmpl   $0x0, 0x4931(%rip)        ; _dyld_private + 7
  0x10868fc77 <+87>:  jle    0x10868fcc3               ; <+163> at ViewController.m
  0x10868fc7d <+93>:  movl   0x4925(%rip), %eax        ; count
  0x10868fc83 <+99>:  decl   %eax
  0x10868fc85 <+101>: movl   %eax, 0x491d(%rip)        ; count
  0x10868fc8b <+107>: movq   -0x8(%rbp), %rdi
  0x10868fc8f <+111>: movq   0x47d2(%rip), %rsi        ; "recuresiveAction"
  0x10868fc96 <+118>: movq   0x2373(%rip), %rcx        ; (void *)0x00007fff513f7780: objc_msgSend
  0x10868fc9d <+125>: callq  *%rcx
  0x10868fc9f <+127>: jmp    0x10868fca4               ; <+132> at ViewController.m:304:9
  0x10868fca4 <+132>: jmp    0x10868fcc3               ; <+163> at ViewController.m
  0x10868fca9 <+137>: movl   %edx, %ecx
  0x10868fcab <+139>: movq   %rax, -0x18(%rbp)
  0x10868fcaf <+143>: movl   %ecx, -0x1c(%rbp)
  0x10868fcb2 <+146>: movq   -0x28(%rbp), %rdi
  0x10868fcb6 <+150>: callq  0x108690366               ; symbol stub for: objc_sync_exit
  0x10868fcbb <+155>: movl   %eax, -0x30(%rbp)
  0x10868fcbe <+158>: jmp    0x10868fcdf               ; <+191> at ViewController.m:305:5
  0x10868fcc3 <+163>: movq   -0x28(%rbp), %rdi
  0x10868fcc7 <+167>: callq  0x108690366               ; symbol stub for: objc_sync_exit // 👈 看到调用了 objc_sync_exit 函数
  ```
  看到 `@synchronized` 调用了 `objc_sync_enter` 和 `objc_sync_exit` 函数，下面从 `objc4-781` 中看一下这两个函数的实现：
  `objc_sync_exit` 函数(位于 `Source/objc-sync.mm` p327)：
  ```c++
  // End synchronizing on 'obj'. 
  // Returns OBJC_SYNC_SUCCESS or OBJC_SYNC_NOT_OWNING_THREAD_ERROR
  int objc_sync_exit(id obj)
  {
      int result = OBJC_SYNC_SUCCESS;
      
      if (obj) {
          SyncData* data = id2data(obj, RELEASE); 
          if (!data) {
              result = OBJC_SYNC_NOT_OWNING_THREAD_ERROR;
          } else {
              bool okay = data->mutex.tryUnlock(); // 尝试解锁，返回 true 表示解锁成功，否则表示失败
              if (!okay) {
                  result = OBJC_SYNC_NOT_OWNING_THREAD_ERROR;
              }
          }
      } else {
          // @synchronized(nil) does nothing
      }
      

      return result;
  }
  ```
  `objc_sync_enter` 函数(位于 `Source/objc-sync.mm` p284)：
  ```c++
  // Begin synchronizing on 'obj'. 
  // Allocates recursive mutex associated with 'obj' if needed.
  // Returns OBJC_SYNC_SUCCESS once lock is acquired.  
  int objc_sync_enter(id obj)
  {
      int result = OBJC_SYNC_SUCCESS;

      if (obj) {
          // 根据传入的对象，来获取一个锁，所以使用 @synchronized 时传入对象很重要
          SyncData* data = id2data(obj, ACQUIRE);
          ASSERT(data);
          data->mutex.lock(); // 这里使用 data 的 mutex 成员变量执行 lock
      } else {
          // @synchronized(nil) does nothing
          // 传入 nil 则什么也不做
          if (DebugNilSync) {
              _objc_inform("NIL SYNC DEBUG: @synchronized(nil); set a breakpoint on objc_sync_nil to debug");
          }
          objc_sync_nil();
      }
      
      return result;
  }
  ```
  `data->mutex.lock(); // 这里使用 data 的 mutex 成员变量执行 lock` 那么看一下 `SyncData` 实现。
  `SyncData` 底层结构(`Source/objc-sync.mm` P33)：
  ```c++
  typedef struct alignas(CacheLineSize) SyncData {
      struct SyncData* nextData;
      DisguisedPtr<objc_object> object;
      int32_t threadCount;  // number of THREADS using this block
      recursive_mutex_t mutex;
  } SyncData;
  ```
  `recursive_mutex_t` 使用使用 `using` 关键字声明的模版类。
  `using recursive_mutex_t = recursive_mutex_tt<LOCKDEBUG>;`
  下面看一下 `recursive_mutex_tt` 底层结构:
  ```c++
  template <bool Debug>
  class recursive_mutex_tt : nocopy_t {
      os_unfair_recursive_lock mLock;

    public:
      constexpr recursive_mutex_tt() : mLock(OS_UNFAIR_RECURSIVE_LOCK_INIT) {
          lockdebug_remember_recursive_mutex(this);
      }

      constexpr recursive_mutex_tt(const fork_unsafe_lock_t unsafe)
          : mLock(OS_UNFAIR_RECURSIVE_LOCK_INIT)
      { }

      void lock()
      {
          lockdebug_recursive_mutex_lock(this);
          os_unfair_recursive_lock_lock(&mLock);
      }
      ...
    };
  ```
  引入 `objc4-723` 版本:
   ```c++
   // 在 objc4-723 版本中 recursive_mutex_tt 的底层结构为
   class recursive_mutex_tt : nocopy_t {
       // 底层封装的是互斥锁pthread_mutex_t
       pthread_mutex_t mLock;

     public:
       recursive_mutex_tt() : mLock(PTHREAD_RECURSIVE_MUTEX_INITIALIZER) {
           lockdebug_remember_recursive_mutex(this);
       }

       recursive_mutex_tt(const fork_unsafe_lock_t unsafe)
           : mLock(PTHREAD_RECURSIVE_MUTEX_INITIALIZER)
       { }
   ...
   }
   ```
  继续查看 `os_unfair_recursive_lock` 底层实现:
  ```c++
  /*!
  * @typedef os_unfair_recursive_lock
  *
  * @abstract
  * Low-level lock that allows waiters to block efficiently on contention.
  *
  * @discussion
  * See os_unfair_lock.
  *
  */
  OS_UNFAIR_RECURSIVE_LOCK_AVAILABILITY
  typedef struct os_unfair_recursive_lock_s {
      os_unfair_lock ourl_lock; // 底层为互斥锁 os_unfair_lock 
      uint32_t ourl_count; // 因为 @synchronized 为递归锁，所以需要记录加锁次数
  } os_unfair_recursive_lock, *os_unfair_recursive_lock_t;
  ```
  到这里可以确认了底层是 `os_unfair_lock`。 
  然后我们注意到 `OS_UNFAIR_RECURSIVE_LOCK_AVAILABILITY`:
  ```c++
  /*! @group os_unfair_recursive_lock SPI
   *
   * @abstract
   * Similar to os_unfair_lock, but recursive.
   *
   * @discussion
   * Must be initialized with OS_UNFAIR_RECURSIVE_LOCK_INIT
   */

  #define OS_UNFAIR_RECURSIVE_LOCK_AVAILABILITY \
          __OSX_AVAILABLE(10.14) __IOS_AVAILABLE(12.0) \
          __TVOS_AVAILABLE(12.0) __WATCHOS_AVAILABLE(5.0)
  ```
  这里表明是 `iOS 12.0` 之后才是出现的。
  至此可验证 `iOS 12.0` 后 `@synchronized` 是一个封装了 `os_unfair_lock` 的递归锁（`os_unfair_recursive_lock`）。
2. `@synchronized(obj){...}` 传入一个对象 `obj` 进行加锁，如果传入空，则不执行操作。

## 示例代码:
```objective-c
#import "ViewController.h"

static int count = 3;

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    dispatch_queue_t global_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    __weak typeof(self) _self = self;
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
            
        [self recuresiveAction];
    });
}

#pragma mark - Private Methods

- (void)recuresiveAction {
    @synchronized ([self class]) {
        NSLog(@"🌰🌰🌰 count = %d", count);
        if (count > 0) {
            count--;
            
            [self recuresiveAction];
        }
    }
}

#pragma mark - dealloc
- (void)dealloc {
    NSLog(@"🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...");
}

@end

// 打印结果:
🌰🌰🌰 count = 3
🌰🌰🌰 count = 2
🌰🌰🌰 count = 1
🌰🌰🌰 count = 0
🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...
```

**参考链接:🔗**
+ [iOS 锁 部分三](https://www.jianshu.com/p/b6509683876c)
