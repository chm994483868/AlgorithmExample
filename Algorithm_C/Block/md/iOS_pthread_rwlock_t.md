#  iOS_pthread_rwlock_t

## 如何实现一个多读单写的模型
如何实现一个多读单写的模型，需求如下:
+ 同时可以有多个线程读取
+ 同时只能有一个线程写入
+ 同时只能执行读取或者写入的一种

### 方案 1: 读写锁 pthread_rwlock_t
特点:
1. 读取加锁可以同时多个线程进行，写入同时只能一个线程进行，等待的线程处于休眠状态
2. 可能会用到的方法：
  1. `pthread_rwlock_init()` 初始化一个读写锁
  2. `pthread_rwlock_rdlock()` 读写锁的读取加锁
  3. `pthread_rwlock_wrlock()` 读写锁的写入加锁
  4. `pthread_rwlock_unlock()` 解锁
  5. `pthread_rwlock_destroy()` 销毁锁

3. 代码示例，测试代码主要看，打印读取可以同时出现几个，打印写入同时只会出现一个：
```objective-c
#import "ViewController.h"
#import <pthread.h>

@interface ViewController ()

@property (nonatomic, assign) pthread_rwlock_t lock;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.

    [self rwlockType];
}

#pragma mark - Private methods
- (void)rwlockType {
    pthread_rwlock_init(&self->_lock, NULL);
    
    dispatch_queue_t globalQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    __weak typeof(self) _self = self;
    for (unsigned int i = 0; i < 100; ++i) {
        // 同时创建多个线程进行写入操作
        dispatch_async(globalQueue, ^{
            __weak typeof(_self) self = _self;
            if (!self) return;
            
            [self lockWriteAction];
        });
        dispatch_async(globalQueue, ^{
            __weak typeof(_self) self = _self;
            if (!self) return;
            
            [self lockWriteAction];
        });
        dispatch_async(globalQueue, ^{
            __weak typeof(_self) self = _self;
            if (!self) return;
            
            [self lockWriteAction];
        });
        
        // 同时创建多个线程进行读操作
        dispatch_async(globalQueue, ^{
            __strong typeof(_self) self = _self;
            if (!self) return;
            
            [self lockReadAction];
        });
        dispatch_async(globalQueue, ^{
            __strong typeof(_self) self = _self;
            if (!self) return;
            
            [self lockReadAction];
        });
        dispatch_async(globalQueue, ^{
            __strong typeof(_self) self = _self;
            if (!self) return;
            
            [self lockReadAction];
        });
    }
}

- (void)lockReadAction {
    pthread_rwlock_rdlock(&self->_lock);
    sleep(1);
    NSLog(@"RWLock read action %@", [NSThread currentThread]);
    pthread_rwlock_unlock(&self->_lock);
}

- (void)lockWriteAction {
    pthread_rwlock_wrlock(&self->_lock);
    sleep(1);
    NSLog(@"RWLock Write Action %@", [NSThread currentThread]);
    pthread_rwlock_unlock(&self->_lock);
}

#pragma mark - dealloc

-(void)dealloc {
    NSLog(@"🚚🚚🚚 deallocing...");
    
    pthread_rwlock_destroy(&self->_lock);
}

@end
// 打印结果: 可看到每次 write 操作同一个时间只执行一次，每次执行 write 操作至少相差 1 的时间，而 read 操作，几乎三次读取完全同一时刻进行

2020-08-23 21:56:47.918292+0800 algorithm_OC[17138:583665] RWLock Write Action <NSThread: 0x600001d45440>{number = 6, name = (null)}
2020-08-23 21:56:48.918953+0800 algorithm_OC[17138:583666] RWLock Write Action <NSThread: 0x600001d58740>{number = 4, name = (null)}
2020-08-23 21:56:49.924037+0800 algorithm_OC[17138:583667] RWLock Write Action <NSThread: 0x600001d06440>{number = 3, name = (null)}

2020-08-23 21:56:50.927716+0800 algorithm_OC[17138:583697] RWLock read action <NSThread: 0x600001d00d40>{number = 10, name = (null)}
2020-08-23 21:56:50.927716+0800 algorithm_OC[17138:583696] RWLock read action <NSThread: 0x600001d864c0>{number = 8, name = (null)}
2020-08-23 21:56:50.927721+0800 algorithm_OC[17138:583698] RWLock read action <NSThread: 0x600001da4b40>{number = 9, name = (null)}

2020-08-23 21:56:51.928224+0800 algorithm_OC[17138:583699] RWLock Write Action <NSThread: 0x600001d86f80>{number = 11, name = (null)}
2020-08-23 21:56:52.928578+0800 algorithm_OC[17138:583700] RWLock Write Action <NSThread: 0x600001d85d40>{number = 12, name = (null)}
2020-08-23 21:56:53.931865+0800 algorithm_OC[17138:583701] RWLock Write Action <NSThread: 0x600001d701c0>{number = 13, name = (null)}

2020-08-23 21:56:54.936356+0800 algorithm_OC[17138:583703] RWLock read action <NSThread: 0x600001d70500>{number = 14, name = (null)}
2020-08-23 21:56:54.936370+0800 algorithm_OC[17138:583702] RWLock read action <NSThread: 0x600001d82fc0>{number = 15, name = (null)}
2020-08-23 21:56:54.936370+0800 algorithm_OC[17138:583704] RWLock read action <NSThread: 0x600001da4e40>{number = 16, name = (null)}

2020-08-23 21:56:55.941718+0800 algorithm_OC[17138:583706] RWLock Write Action <NSThread: 0x600001d86c80>{number = 17, name = (null)}
2020-08-23 21:56:56.944550+0800 algorithm_OC[17138:583705] RWLock Write Action <NSThread: 0x600001d86400>{number = 18, name = (null)}
2020-08-23 21:56:57.945582+0800 algorithm_OC[17138:583707] RWLock Write Action <NSThread: 0x600001d04040>{number = 19, name = (null)}

2020-08-23 21:56:58.950336+0800 algorithm_OC[17138:583709] RWLock read action <NSThread: 0x600001d80180>{number = 21, name = (null)}
2020-08-23 21:56:58.950341+0800 algorithm_OC[17138:583710] RWLock read action <NSThread: 0x600001daef40>{number = 22, name = (null)}
2020-08-23 21:56:58.950336+0800 algorithm_OC[17138:583708] RWLock read action <NSThread: 0x600001d868c0>{number = 20, name = (null)}

2020-08-23 21:56:59.955688+0800 algorithm_OC[17138:583711] RWLock Write Action <NSThread: 0x600001d80140>{number = 23, name = (null)}
2020-08-23 21:57:00.956602+0800 algorithm_OC[17138:583712] RWLock Write Action <NSThread: 0x600001d05340>{number = 24, name = (null)}
2020-08-23 21:57:01.961885+0800 algorithm_OC[17138:583713] RWLock Write Action <NSThread: 0x600001d05680>{number = 25, name = (null)}

2020-08-23 21:57:02.962283+0800 algorithm_OC[17138:583715] RWLock read action <NSThread: 0x600001da4640>{number = 27, name = (null)}
2020-08-23 21:57:02.962283+0800 algorithm_OC[17138:583714] RWLock read action <NSThread: 0x600001d70480>{number = 26, name = (null)}
2020-08-23 21:57:02.962285+0800 algorithm_OC[17138:583716] RWLock read action <NSThread: 0x600001d87000>{number = 28, name = (null)}

2020-08-23 21:57:03.962688+0800 algorithm_OC[17138:583717] RWLock Write Action <NSThread: 0x600001d9da00>{number = 29, name = (null)}
2020-08-23 21:57:04.965468+0800 algorithm_OC[17138:583718] RWLock Write Action <NSThread: 0x600001d82780>{number = 30, name = (null)}
2020-08-23 21:57:05.969558+0800 algorithm_OC[17138:583719] RWLock Write Action <NSThread: 0x600001d07540>{number = 31, name = (null)}

2020-08-23 21:57:06.969737+0800 algorithm_OC[17138:583720] RWLock read action <NSThread: 0x600001d87040>{number = 34, name = (null)}
2020-08-23 21:57:06.969738+0800 algorithm_OC[17138:583721] RWLock read action <NSThread: 0x600001d70300>{number = 32, name = (null)}
2020-08-23 21:57:06.969740+0800 algorithm_OC[17138:583722] RWLock read action <NSThread: 0x600001d83d40>{number = 33, name = (null)}

2020-08-23 21:57:07.974805+0800 algorithm_OC[17138:583723] RWLock Write Action <NSThread: 0x600001d07700>{number = 35, name = (null)}
2020-08-23 21:57:08.975786+0800 algorithm_OC[17138:583724] RWLock Write Action <NSThread: 0x600001d83800>{number = 36, name = (null)}

2020-08-23 21:57:09.981012+0800 algorithm_OC[17138:583726] RWLock read action <NSThread: 0x600001da4600>{number = 37, name = (null)}
2020-08-23 21:57:10.984100+0800 algorithm_OC[17138:583725] RWLock Write Action <NSThread: 0x600001d86d40>{number = 38, name = (null)}
2020-08-23 21:57:11.986123+0800 algorithm_OC[17138:583727] RWLock read action <NSThread: 0x600001d80540>{number = 39, name = (null)}
2020-08-23 21:57:12.989780+0800 algorithm_OC[17138:583729] RWLock Write Action <NSThread: 0x600001d870c0>{number = 40, name = (null)}
2020-08-23 21:57:13.994964+0800 algorithm_OC[17138:583730] RWLock Write Action <NSThread: 0x600001d86f00>{number = 41, name = (null)}
2020-08-23 21:57:15.000216+0800 algorithm_OC[17138:583732] RWLock read action <NSThread: 0x600001dade80>{number = 43, name = (null)}
2020-08-23 21:57:15.000216+0800 algorithm_OC[17138:583728] RWLock read action <NSThread: 0x600001d80500>{number = 42, name = (null)}
```

### 方案 2: 异步栅栏 dispatch_barrier_async
特点
1. 传入的并发队列必须是手动创建的，`dispatch_queue_create()` 方式，如果传入串行队列或者通过 `dispatch_get_global_queue()` 方式创建，则 `dispatch_barrier_async` 的作用就跟 `dispatch_async` 变的一样。
2. 可能会用到的方法：
  1. `dispatch_queue_create()` 创建并发队列
  2. `dispatch_barrier_async()` 异步栅栏
  
测试代码：
测试代码主要看的是，打印读取可以同时出现几个，打印写入同时只会出现一个：
```objective-c
#import "ViewController.h"

@interface ViewController ()

@property (nonatomic, strong) dispatch_queue_t queue;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    [self barrierAsyncType];
}

#pragma mark - Private methods
- (void)barrierAsyncType {
    self.queue = dispatch_queue_create("rw_queue", DISPATCH_QUEUE_CONCURRENT);
    for (unsigned int i = 0; i < 100; ++i) {
        // 同时创建多个线程进行写入操作
        [self barrierWriteAction];
        [self barrierWriteAction];
        [self barrierWriteAction];
        // 同时创建多个线程进行读取操作
        [self barrierReadAction];
        [self barrierReadAction];
        [self barrierReadAction];
    }
}

- (void)barrierReadAction {
    dispatch_async(self.queue, ^{
        sleep(1);
        NSLog(@"barrier Read Action %@", [NSThread currentThread]);
    });
}

- (void)barrierWriteAction {
    dispatch_barrier_async(self.queue, ^{
        sleep(1);
        NSLog(@"barrier Write Action %@", [NSThread currentThread]);
    });
}

@end

// 打印结果: 从打印时间可以看出，write 操作是依序进行的，每次间隔 1 秒，而 read 操作几乎都是同时进行 3 次
2020-08-23 22:25:14.144265+0800 algorithm_OC[17695:604062] barrier Write Action <NSThread: 0x6000012a0180>{number = 5, name = (null)}
2020-08-23 22:25:15.148017+0800 algorithm_OC[17695:604062] barrier Write Action <NSThread: 0x6000012a0180>{number = 5, name = (null)}
2020-08-23 22:25:16.151869+0800 algorithm_OC[17695:604062] barrier Write Action <NSThread: 0x6000012a0180>{number = 5, name = (null)}

2020-08-23 22:25:17.156004+0800 algorithm_OC[17695:604062] barrier Read Action <NSThread: 0x6000012a0180>{number = 5, name = (null)}
2020-08-23 22:25:17.156040+0800 algorithm_OC[17695:604063] barrier Read Action <NSThread: 0x600001230340>{number = 6, name = (null)}
2020-08-23 22:25:17.156023+0800 algorithm_OC[17695:604065] barrier Read Action <NSThread: 0x6000012e6300>{number = 3, name = (null)}

2020-08-23 22:25:18.159721+0800 algorithm_OC[17695:604063] barrier Write Action <NSThread: 0x600001230340>{number = 6, name = (null)}
2020-08-23 22:25:19.165158+0800 algorithm_OC[17695:604063] barrier Write Action <NSThread: 0x600001230340>{number = 6, name = (null)}
2020-08-23 22:25:20.166640+0800 algorithm_OC[17695:604063] barrier Write Action <NSThread: 0x600001230340>{number = 6, name = (null)}

2020-08-23 22:25:21.170586+0800 algorithm_OC[17695:604062] barrier Read Action <NSThread: 0x6000012a0180>{number = 5, name = (null)}
2020-08-23 22:25:21.170589+0800 algorithm_OC[17695:604065] barrier Read Action <NSThread: 0x6000012e6300>{number = 3, name = (null)}
2020-08-23 22:25:21.170584+0800 algorithm_OC[17695:604063] barrier Read Action <NSThread: 0x600001230340>{number = 6, name = (null)}
```

## 关于锁的一些总结:
1.  常用的锁的效率排序

1. `os_unfair_lock` (`iOS 10` 之后)
2. `OSSpinLock` (`iOS 10` 之前)
3. `dispatch_semaphore` (`iOS` 版本兼容性好)
4. `pthread_mutex_t` (`iOS` 版本兼容性好)
5. `NSLock` (基于 `pthread_mutex_t` 封装)
6. `NSCondition` (基于 `pthread_mutex_t` 封装)
7. `pthread_mutex_t(recursive)` 递归锁的优先推荐
8. `NSRecursiveLock` (基于 `pthread_mutex_t` 封装)
9. `NSConditionLock` (基于 `NSCondition` 封装)
10. `@synchronized`
  1. `iOS 12` 之前基于 `pthread_mutex_t` 封装
  2. `iOS 12` 之后基于 `os_unfair_lock` 封装（iOS 12 之后它的效率应该不是最低，应该在 3/4 左右）

2. 自旋锁和互斥锁的取舍
自旋锁和互斥锁怎么选择，其实这个问题已经没有什么意义，因为自旋锁 `OSSpinLock` 在 `iOS 10` 之后已经废弃了，而它的替换方案 `os_unfair_lock` 是互斥锁，但是我们仍然做一下对比:
**自旋锁:**
+ 预计线程需要等待的时间较短
+ 多核处理器
+ `CPU` 的资源不紧张
**互斥锁:**
+ 预计线程需要等待的时间较长
+ 单核处理器
+ 临界区（加锁解锁之间的部分）有 I/O 操作
+ 临界区的较为复杂和循环了比较大

**其它:**

加锁和解锁的实现一定要配套出现，不然就会出现死锁的现象。

**参考链接:🔗**
[iOS 锁 部分四](https://www.jianshu.com/p/6ebb208f9a4c)
