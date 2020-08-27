#  iOS_NSLock

## 特点
1. 基于 `mutex` 基本锁的封装，更加面向对象，等待锁的线程会处于休眠状态。
2. 遵守 `NSLocking` 协议，`NSLocking` 协议中两个方法如下:
  ```objective-c
  - (void)lock;
  - (void)unlock;
  ```
3. 可能会用到的方法。
  1. 初始化跟其他 `OC` 对象一样，直接进行 `alloc` 和 `init` 操作。
  2. `- (void)lock;` 加锁。
  3. `- (void)unlock;` 解锁。
  4. `- (BOOL)tryLock;` 尝试加锁。
  5. `- (BOOL)lockBeforeDate:(NSDate *)limit;` 在某一个时间点之前等待加锁。
4. 在主线程连续连续调用 `[self.lock lock];` 会导致主线程死锁。
5. 在主线程没有获取 `Lock` 的情况下和在获取 `Lock` 的情况下，连续两次 ` [self.lock unlock];` 都不会发生异常也不会死锁。（其他的锁可能连续解锁的情况下会导致 `crash`，还没有来的及测试）
6. 在子线程连续 `[self.lock lock];` 会导致死锁，同时别的子线获取 `self.lock` 则会一直等待下去。
7. 同时子线程死锁会导致 `ViewController` 不释放。

## 使用示例
```objective-c
@interface ViewController ()

@property (nonatomic, assign) NSInteger sum;
@property (nonatomic, strong) NSLock *lock;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.sum = 0;
    self.lock = [[NSLock alloc] init];
    dispatch_queue_t global_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    
    __weak typeof(self) _self = self;
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        [self.lock lock];
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        [self.lock unlock];
        
        NSLog(@"👿👿👿 %ld", (long)self.sum);
    });
    
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        [self.lock lock];
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        [self.lock unlock];
        
        NSLog(@"😵😵😵 %ld", (long)self.sum);
    });
}

#pragma mark - dealloc
- (void)dealloc {
    NSLog(@"🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...");
}

@end
// 打印结果:
😵😵😵 20000
👿👿👿 10000
🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...
```
## NSLock.h 文件解析
```
#import <Foundation/NSObject.h>

@class NSDate;

NS_ASSUME_NONNULL_BEGIN

@protocol NSLocking // 看到 NSLocking 协议只有加锁和解锁两个协议方法

- (void)lock;
- (void)unlock;

@end

@interface NSLock : NSObject <NSLocking> { // NSLock 是继承自 NSObject 并遵守 NSLocking 协议
@private
    void *_priv;
}

// 测试 tryLock
// NSLog(@"%d", [self.lock tryLock]);
// [self.lock unlock];
// NSLog(@"%d", [self.lock tryLock]);
// [self.lock unlock];
// NSLog(@"%d", [self.lock tryLock]);
// [self.lock unlock];

- (BOOL)tryLock; // 尝试加锁，返回 true 表示加锁成功
- (BOOL)lockBeforeDate:(NSDate *)limit; // 在某个 NSDate 之前加锁

@property (nullable, copy) NSString *name API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));

@end

@interface NSConditionLock : NSObject <NSLocking> { // 继承自 NSObject 并遵守 NSLocking 协议
@private
    void *_priv;
}

- (instancetype)initWithCondition:(NSInteger)condition NS_DESIGNATED_INITIALIZER;

@property (readonly) NSInteger condition; // 只读的 condition 属性
- (void)lockWhenCondition:(NSInteger)condition; // 根据 condition 值加锁, 如果值不满足, 则不加;
- (BOOL)tryLock;
- (BOOL)tryLockWhenCondition:(NSInteger)condition; 
- (void)unlockWithCondition:(NSInteger)condition; //  解锁, 并设定 condition 的值;
- (BOOL)lockBeforeDate:(NSDate *)limit; // 在某一个时间点之前等待加锁
- (BOOL)lockWhenCondition:(NSInteger)condition beforeDate:(NSDate *)limit;

@property (nullable, copy) NSString *name API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));

@end

@interface NSRecursiveLock : NSObject <NSLocking> { // 继承自 NSObject 并遵守 NSLocking 协议
@private
    void *_priv;
}

- (BOOL)tryLock; // 尝试加锁，返回 true 表示加锁成功
- (BOOL)lockBeforeDate:(NSDate *)limit; // 在某个 NSDate 之前加锁

@property (nullable, copy) NSString *name API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));

@end

API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0))
@interface NSCondition : NSObject <NSLocking> { // 继承自 NSObject 并遵守 NSLocking 协议
@private
    void *_priv;
}

- (void)wait; // 添加等待，线程休眠，并将锁让出
- (BOOL)waitUntilDate:(NSDate *)limit; // 某个 NSDate 
- (void)signal; // 发送信号，告知等待的线程，条件满足了
- (void)broadcast; // 通知所有符合条件的线程，（通知所有在等待的线程）

@property (nullable, copy) NSString *name API_AVAILABLE(macos(10.5), ios(2.0), watchos(2.0), tvos(9.0));

@end

NS_ASSUME_NONNULL_END
```

**参考链接:🔗**
+ [iOS 锁 部分二](https://www.jianshu.com/p/d0fd5a5869e5)
