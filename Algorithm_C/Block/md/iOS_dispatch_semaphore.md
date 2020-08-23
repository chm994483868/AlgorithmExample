#  iOS_dispatch_semaphore 信号量

## 特点
1. 本来是用于控制线程的最大并发数量，我们将并发数量设置为 1 也可以认为是加锁的功能。
2. 可能会用到的方法：
  1. 初始化 `dispatch_semaphore_create()` 传入的值为最大并发数量，设置为 1 则达到加锁效果。
  2. 判断信号量的值 `dispatch_semaphore_wait()` 如果大于0，则可以继续往下执行（同时信号量的值减去 1），如果信号量的值为 0，则线程进入休眠状态等待（此方法的第二个参数就是设置要等多久，一般是使用永久 `DISPATCH_TIME_FOREVER`）。
  3. 释放信号量 `dispatch_semaphore_signal()` 同时使信号量的值加上 1.

## 示例代码
```objective-c
#import "ViewController.h"

@interface ViewController ()

@property (nonatomic, assign) NSInteger sum;
@property (nonatomic, strong) dispatch_semaphore_t semaphore;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.sum = 0;
    self.semaphore = dispatch_semaphore_create(1);
    
    dispatch_queue_t global_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    
    __weak typeof(self) _self = self;
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        dispatch_semaphore_wait(self.semaphore, DISPATCH_TIME_FOREVER);
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        dispatch_semaphore_signal(self.semaphore);
        NSLog(@"🍐🍐🍐 %ld", (long)self.sum);
    });
    
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        dispatch_semaphore_wait(self.semaphore, DISPATCH_TIME_FOREVER);
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        dispatch_semaphore_signal(self.semaphore);
        NSLog(@"🍎🍎🍎 %ld", (long)self.sum);
    });
}

#pragma mark - dealloc
- (void)dealloc {
    NSLog(@"🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...");
}

@end

// 打印结果:
🍐🍐🍐 10000
🍎🍎🍎 20000
🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...
```

**参考链接:🔗**
[iOS 锁 部分三](https://www.jianshu.com/p/b6509683876c)

