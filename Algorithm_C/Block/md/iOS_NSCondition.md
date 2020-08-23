#  iOS_NSCondition 条件锁

## 特点
1. 基于 `mutex` 基础锁和 `cont` 条件的封装，所以它是互斥锁且自带条件，等待锁的线程休眠。
2. 遵守 `NSLocking` 协议，`NSLocking` 协议中两个方法如下：
```objective-c
- (void)lock;
- (void)unlock;
```
3. 可能会用到的方法
  1. 初始化跟其它 OC 对象一样，直接进行 alloc 和 init 操作。
  2. `- (void)lock;` 加锁
  3. `- (void)unlock;` 解锁
  4. `- (BOOL)tryLock;` 尝试加锁
  5. `- (BOOL)lockBeforeDate:(NSDate *)limit;` 在某一个时间点之前等待加锁
  6. `- (void)wait;` 等待条件（进入休眠的同时放开锁，被唤醒的同时再次加锁）
  7. `- (void)signal;` 发送信号激活等待该条件的线程，切记线程收到后是从 wait 下开始的
  8. `- (void)broadcast;` 发送广播信号激活等待该条件的所有线程，切记线程收到后是从 wait 下开始的

## 测试代码
```
#import "ViewController.h"

@interface ViewController ()

@property (nonatomic, strong) NSMutableArray *dataArr;
@property (nonatomic, strong) NSCondition *condition;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    // 初始化数组
    self.dataArr = [NSMutableArray array];
    self.condition = [[NSCondition alloc] init];
    dispatch_queue_t global_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    
    __weak typeof(self) _self = self;
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        [self deleteObj];
    });
    
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        [self deleteObj];
    });
    
    // sleep 0.5 秒，确保删除元素的操作先取得锁
    sleep(0.5);
    
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        [self addObj];
    });
}

#pragma mark - Private Methods

- (void)deleteObj {
    [self.condition lock];
    NSLog(@"🧑‍💻🧑‍💻🧑‍💻 delete begin");
    // 添加判断，如果没有数据则添加条件
    if (self.dataArr.count < 1) {
        // 添加条件，如果数组为空，则添加等待线程休眠，将锁让出，这里会将锁让出去，所以下面的 addObj 线程才能获得锁
        // 接收到信号时会再次加锁，然后继续向下执行
        NSLog(@"下面是进入 wait...");
        [self.condition wait];
        
        // 当 broadcast 过来的时候还是继续往下执行，
        // 切记不是从 deleteObj 函数头部开始的，是从这里开始的
        // 所以当第一个异步删除数组元素后，第二个异步进来时数组已经空了
        NSLog(@"接收到 broadcast 或 signal 后的函数起点");
    }
    NSLog(@"%@", self.dataArr);
    [self.dataArr removeLastObject];
    NSLog(@"🧑‍💻🧑‍💻🧑‍💻 数组执行删除元素操作");
    [self.condition unlock];
}

- (void)addObj {
    [self.condition lock];
    NSLog(@"🧑‍💻🧑‍💻🧑‍💻 add begin");
    [self.dataArr addObject:@"CHM"];
    // 发送信号，说明已经添加元素了
    // [self.condition signal];
    // 通知所有符合条件的线程
    [self.condition broadcast];
    NSLog(@"🧑‍💻🧑‍💻🧑‍💻 数组执行添加元素操作");
    [self.condition unlock];
}


#pragma mark - dealloc
- (void)dealloc {
    NSLog(@"🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...");
}
@end

// 打印结果:
🧑‍💻🧑‍💻🧑‍💻 delete begin
下面是进入 wait...
🧑‍💻🧑‍💻🧑‍💻 delete begin
下面是进入 wait...
🧑‍💻🧑‍💻🧑‍💻 add begin
🧑‍💻🧑‍💻🧑‍💻 数组执行添加元素操作
接收到 broadcast 或 signal 后的函数起点
2020-08-23 11:52:53.329603+0800 objc_Simple[26157:1148891] (
    CHM
)
🧑‍💻🧑‍💻🧑‍💻 数组执行删除元素操作
接收到 broadcast 或 signal 后的函数起点
(
)
🧑‍💻🧑‍💻🧑‍💻 数组执行删除元素操作
🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...
```

**参考链接:🔗**
[iOS 锁 部分二](https://www.jianshu.com/p/d0fd5a5869e5)
