#  iOS_NSRecursiveLock
## 特点 
1. 同 `NSLock` 一样，也是基于 `mutex` 的封装，不过是基于 `mutex` 递归锁的封装，所以这是一个递归锁。
2. 遵守 `NSLocking` 协议，`NSLocking` 协议中两个方法如下:
```objective-c
- (void)lock;
- (void)unlock;
```
3. 可能会用到的方法
  1. 继承自 NSObject，所以初始化跟其他 OC 对象一样，直接进行 alloc 和 init 操作。
  2. `- (void)lock;` 加锁
  3. `- (void)unlock;` 解锁
  4. `- (BOOL)tryLock;` 尝试加锁
  5. `- (BOOL)lockBeforeDate:(NSDate *)limit;` 在某一个时间点之前等待加锁。

## 死锁验证
1. 递归锁是可以连续调用 `lock` 不会直接导致死锁，但是依然要执行相等次数的 `unlock`。不然异步线程再获取该递归锁会导致该异步线程死锁。
  ```
  // 在主线程执行如下代码，连续 lock 不会导致主线程死锁，然后必须执行相同次数 unlock，
  // 不然在下面的异步线程里面再执行 lock，会直接导致异步线程死锁
  [self.recursiveLock lock];
  [self.recursiveLock lock];
  [self.recursiveLock lock];
  
  [self.recursiveLock unlock];
  [self.recursiveLock unlock];
  [self.recursiveLock unlock];
  ``` 

## 测试代码
```objective-c
#import "ViewController.h"

static int count = 3;

@interface ViewController ()

@property (nonatomic, assign) NSInteger sum;
@property (nonatomic, strong) NSLock *lock;
@property (nonatomic, strong) NSRecursiveLock *recursiveLock;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    self.sum = 0;
    self.recursiveLock = [[NSRecursiveLock alloc] init];
    dispatch_queue_t global_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    
    __weak typeof(self) _self = self;
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
    
        [self.recursiveLock lock];
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        [self.recursiveLock unlock];
        
        NSLog(@"👿👿👿 %ld", (long)self.sum);
    });
    
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        [self recursiveAction];
    });
    
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        [self.recursiveLock lock];
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        [self.recursiveLock unlock];
        
        NSLog(@"😵😵😵 %ld", (long)self.sum);
    });
    
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        [self recursiveAction];
    });
}

#pragma mark - Private Methods

- (void)recursiveAction {
    [self.recursiveLock lock];
    NSLog(@"😓😓😓 count = %d", count);
    if (count > 0) {
        count--;
        [self recursiveAction];
    }

    // else { // 如果是单线程的话，这里加一个递归出口没有任何问题
    // return;
    // }

    [self.recursiveLock unlock];
    count = 3;
}

#pragma mark - dealloc
- (void)dealloc {
    NSLog(@"🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...");
}

@end
// 打印结果:
😓😓😓 count = 3
👿👿👿 10000
😓😓😓 count = 2
😓😓😓 count = 1
😓😓😓 count = 0
😵😵😵 20000
😓😓😓 count = 3
😓😓😓 count = 2
😓😓😓 count = 1
😓😓😓 count = 0
```

**参考链接:🔗**
[iOS 锁 部分二](https://www.jianshu.com/p/d0fd5a5869e5)
