#  iOS_NSConditionLock 条件锁

## 特点
> 死锁分析:
  1. `[self.lock unlock];` 执行后 `condition` 保持不变，依然是初始化的值或者是上次执行 `lockWhenCondition:` 时的值。 
  2. A B C 3 条线程必须都执行加锁和解锁后 `ViewController` 才能正常释放，除了最后一条线程可以直接使用 `unlock` 执行解锁外，前两条线程 `unlockWithCondition:` 的 `condition` 的必须后后续的线程的 `lockWhenCondition:` 的 `condition` 的值匹配起来。保证每条线程都 `lock` 和 `unlock`，无法正常执行时都会导致线程死锁，`ViewController` 不会释放。
  3. 在同一线程连续 `[self.lock lockWhenCondition:1];` 会直接死锁，不管用的 `condition` 是否和当前锁的 `condition` 相等，都会导致死锁。

1. 基于 `NSCondition` 的进一步封装，可以更加高级的设置条件值。
  > 假设有这样的场景，三个线程 A B C，执行完 A  线程后才能执行 B，执行完 B 线程后执行 C，就是为线程之间的执行添加依赖，`NSConditionLock` 可以方便的完成这个功能。
2. 遵守 `NSLocking` 协议，`NSLocking` 协议中有这两个方法如下：
  ```objective-c
  - (void)lock;
  - (void)unlock;
  ```
3. 可能用到的方法：
  1. 初始化跟其他 OC 对象一样，直接 `alloc` 和 `initWithCondition:(NSInteger)condition` 操作；（如果使用 `init` 方法，则 `condition` 默认为 0）。
  2. 有一个属性是 `@property(readonly) NSInteger condition;` 用来设置条件值，如果不设定，则默认为零。
  3. `- (void)lock;` 直接加锁。
  4. `- (void)lockWhenCondition:(NSInteger)condition` 根据 `condition` 值加锁，如果值不满足则不加。
  5. `- (BOOL)tryLock;` 尝试加锁。
  6. `- (BOOL)lockBeforeDate:(NSDate *)limit;` 在某一个时间点之前等待加锁。
  
## 示例代码
```objective-c
#import "ViewController.h"

@interface ViewController ()

@property (nonatomic, strong) NSConditionLock *lock;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.lock = [[NSConditionLock alloc] initWithCondition:0];
    [self createThreads];
}

#pragma mark - Private Methods

- (void)createThreads {
    // 需要执行的顺序为 A-B-C，但是因为在子线程中所以我们不能确定谁先执行，添加 sleep 使问题更突出点
    NSThread *c = [[NSThread alloc] initWithTarget:self selector:@selector(threadC) object:nil];
    [c start];
    sleep(0.2);
    
    NSThread *b = [[NSThread alloc] initWithTarget:self selector:@selector(threadB) object:nil];
    [b start];
    sleep(0.2);
    
    NSThread *a = [[NSThread alloc] initWithTarget:self selector:@selector(threadA) object:nil];
    [a start];
}

- (void)threadA {
    NSLog(@"A begin");
    [self.lock lockWhenCondition:0];
    NSLog(@"A threadExcute");
    [self.lock unlockWithCondition:1];
    // [self unlock]; // 如果此处使用 unlock，则导致 B C 线程死锁，且导致 ViewController 不释放
}

- (void)threadB {
    NSLog(@"B begin");
    [self.lock lockWhenCondition:1];
    NSLog(@"B threadExcute");
    [self.lock unlockWithCondition:2];
}

- (void)threadC {
    NSLog(@"C begin");
    [self.lock lockWhenCondition:2];
    NSLog(@"C threadExcute");
    [self.lock unlock];
}

#pragma mark - dealloc
- (void)dealloc {
    NSLog(@"🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...");
}

// 打印结果:
C begin
B begin
A begin
A threadExcute
B threadExcute
C threadExcute
🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...
```

**参考链接:🔗**
+ [iOS 锁 部分二](https://www.jianshu.com/p/d0fd5a5869e5)
