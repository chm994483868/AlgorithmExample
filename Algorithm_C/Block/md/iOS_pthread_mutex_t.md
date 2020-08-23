#  iOS_pthread_mutex_t

## 特点
1. 跨平台使用的 `API`，等待锁的线程会处于休眠状态。
  当使用递归锁时：允许同一个线程重复进行加锁（另一个线程访问时就会等待），这样可以保证多线程时访问共用资源的安全性。
2. 使用时首先要引入头文件: `#import <pthread.h>`。

### 为什么会出现死锁？
当两个线程中其中一个线程获得锁并加锁后，如果使用完没有解锁，则另一个线程就会一直处于等待解锁状态，就会形成死锁。
同一个线程连续获得锁并加锁会导致当前线程死锁，递归锁除外，但是如果在另外的线程获取未解锁的递归锁依然会导致死锁。
**所以我们日常使用一定牢记加锁和解锁配对执行。**

> 死锁验证
  `OSSpinLock` 同一线程连续加锁会导致死锁
  `os_unfair_lock` 同一线程连续加锁会直接 `crash`
  `pthread_mutex_t`（`PTHREAD_MUTEX_DEFAULT` 属性）同一线程连续加锁会导致死锁
  
  在下面示例中，如果在主线程执行 `pthread_mutex_lock(&self->_lock);` 并且不进行解锁，则直接导致下面两条并行队列中的两条异步线程死锁。
    
  在主线程里连续 `lock` 递归锁不会导致主线程死锁，
  然后在异步线程里面再 `lock` 该递归锁时，则会导致死锁。
  即因为主线递归锁没有执行解锁，又在另外的队列异步获取锁则直接导致死锁。
  
  **异步线程的死锁，虽然 block 里面用的 __weak 的self 依然导致 ViewController 不会执行 dealloc！**


## 使用示例
### 互斥锁（`PTHREAD_MUTEX_DEFAULT` 或 `PTHREAD_MUTEX_NORMAL`）
```objective-c
#import "ViewController.h"
#import <pthread.h> // pthread_mutex_t

@interface ViewController ()

@property (nonatomic, assign) NSInteger sum;
@property (nonatomic, assign) pthread_mutex_t lock;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    self.sum = 0;
    dispatch_queue_t global_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

    // 1. 互斥锁，默认状态为互斥锁
    // 初始化属性
    pthread_mutexattr_t att;
    pthread_mutexattr_init(&att);
    // 设置属性，描述锁是什么类型
    pthread_mutexattr_settype(&att, PTHREAD_MUTEX_DEFAULT);
    // 初始化锁
    pthread_mutex_init(&self->_lock, &att);
    // 销毁属性
    pthread_mutexattr_destroy(&att);

    __weak typeof(self) _self = self;
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
    
        pthread_mutex_lock(&self->_lock);
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        pthread_mutex_unlock(&self->_lock);
        
        NSLog(@"😵😵😵 %ld", (long)self.sum);
    });
    
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        pthread_mutex_lock(&self->_lock);
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        pthread_mutex_unlock(&self->_lock);

        NSLog(@"👿👿👿 %ld", (long)self.sum);
    });
}

#pragma mark - dealloc

- (void)dealloc {
    NSLog(@"🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...");
    pthread_mutex_destroy(&self->_lock);
}

@end
// 打印 🖨️：
😵😵😵 10000
👿👿👿 20000
🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...
```
### 递归锁（`PTHREAD_MUTEX_RECURSIVE`）
```objective-c
#import "ViewController.h"
#import <pthread.h> // pthread_mutex_t

static int count = 3;
@interface ViewController ()

@property (nonatomic, assign) NSInteger sum;
@property (nonatomic, assign) pthread_mutex_t recursivelock;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.sum = 0;
    dispatch_queue_t global_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    
    // 2. 递归锁（PTHREAD_MUTEX_RECURSIVE）
    pthread_mutexattr_t recursiveAtt;
    pthread_mutexattr_init(&recursiveAtt);
    // 设置属性，描述锁是什么类型
    pthread_mutexattr_settype(&recursiveAtt, PTHREAD_MUTEX_RECURSIVE);
    // 初始化锁
    pthread_mutex_init(&self->_recursivelock, &recursiveAtt);
    // 销毁属性
    pthread_mutexattr_destroy(&recursiveAtt);
    
    __weak typeof(self) _self = self;
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        pthread_mutex_lock(&self->_recursivelock);
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        pthread_mutex_unlock(&self->_recursivelock);

        NSLog(@"😵😵😵 %ld", (long)self.sum);
    });
    
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        // 递归锁验证
        [self recursiveAction];
    });
    
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        pthread_mutex_lock(&self->_recursivelock);
        for (unsigned int i = 0; i < 10000; ++i) {
            self.sum++;
        }
        pthread_mutex_lock(&self->_recursivelock);
        
        NSLog(@"👿👿👿 %ld", (long)self.sum);
    });
    
    dispatch_async(global_queue, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        
        // 递归锁验证
        [self recursiveAction];
    });
}

#pragma mark - Private Methods
- (void)recursiveAction {
    pthread_mutex_lock(&self->_recursivelock);
    
    NSLog(@"😓😓😓 count = %d", count);
    if (count > 0) {
        count--;
        [self recursiveAction];
    }

    // else { // 如果是单线程的话，这里加一个递归出口没有任何问题
    // return;
    // }
     
    pthread_mutex_unlock(&self->_recursivelock);
    count = 3;
}

#pragma mark - dealloc
- (void)dealloc {
    NSLog(@"🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...");
    pthread_mutex_destroy(&self->_recursivelock);
}

@end

// 打印 🖨️:
😵😵😵 10000
😓😓😓 count = 3
😓😓😓 count = 2
😓😓😓 count = 1
😓😓😓 count = 0
👿👿👿 20000
😓😓😓 count = 3
😓😓😓 count = 2
😓😓😓 count = 1
😓😓😓 count = 0
🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...
```
### 条件锁
首先设定以下场景，两条线程 A  和 B，A 线程中执行删除数组元素，B 线程中执行添加数组元素，由于不知道哪个线程会先执行，所以需要加锁实现，只有在添加之后才能执行删除操作，为互斥锁添加条件可以实现。
通过此方法可以实现线程依赖。
```objective-c
#import "ViewController.h"

#import <pthread.h> // pthread_mutex_t

@interface ViewController ()

@property (nonatomic, strong) NSMutableArray *dataArr;
@property (nonatomic, assign) pthread_mutex_t lock;
@property (nonatomic, assign) pthread_cond_t condition;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // 初始化数组
    self.dataArr = [NSMutableArray array];
    // 初始化锁
    pthread_mutexattr_t att;
    pthread_mutexattr_init(&att);
    pthread_mutexattr_settype(&att, PTHREAD_MUTEX_DEFAULT);
    pthread_mutex_init(&self->_lock, &att);
    pthread_mutexattr_destroy(&att);

    // 初始化条件
    pthread_cond_init(&self->_condition, NULL);
    
    dispatch_queue_t global_DEFAULT = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_queue_t global_HIGH = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);

    __weak typeof(self) _self = self;
    
    dispatch_async(global_HIGH, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        pthread_mutex_lock(&self->_lock);
        NSLog(@"🧑‍💻🧑‍💻🧑‍💻 delete begin");
        if (self.dataArr.count < 1) {
            pthread_cond_wait(&self->_condition, &self->_lock);
        }
        [self.dataArr removeLastObject];
        NSLog(@"数组执行删除元素操作");
        pthread_mutex_unlock(&self->_lock);
    });
    
    dispatch_async(global_DEFAULT, ^{
        __strong typeof(_self) self = _self;
        if (!self) return;
        pthread_mutex_lock(&self->_lock);
        NSLog(@"🧑‍💻🧑‍💻🧑‍💻 add begin");
        [self.dataArr addObject:@"CHM"];
        pthread_cond_signal(&self->_condition);
        NSLog(@"数组执行添加元素操作");
        pthread_mutex_unlock(&self->_lock);
    });

    NSThread *deThread = [[NSThread alloc] initWithTarget:self selector:@selector(deleteObj) object:nil];
    [deThread start];

    // sleep 1 秒，确保删除元素的线程先获得锁
    sleep(1);

    NSThread *addThread = [[NSThread alloc] initWithTarget:self selector:@selector(addObj) object:nil];
    [addThread start];
}

#pragma mark - Private Methods

- (void)deleteObj {
    pthread_mutex_lock(&self->_lock);

    NSLog(@"🧑‍💻🧑‍💻🧑‍💻 delete begin");
    // 添加判断，如果没有数据则添加条件
    if (self.dataArr.count < 1) {
        // 添加条件，如果数组为空，则添加等待线程休眠，将锁让出，这里会将锁让出去，所以下面的 addObj 线程才能获得锁
        // 接收到信号时会再次加锁，然后继续向下执行
        pthread_cond_wait(&self->_condition, &self->_lock);
    }
    [self.dataArr removeLastObject];
    NSLog(@"数组执行删除元素操作");

    pthread_mutex_unlock(&self->_lock);
}

- (void)addObj {
    pthread_mutex_lock(&self->_lock);

    NSLog(@"🧑‍💻🧑‍💻🧑‍💻 add begin");
    [self.dataArr addObject:@"HTI"];
    // 发送信号，说明已经添加元素了
    pthread_cond_signal(&self->_condition);
    NSLog(@"数组执行添加元素操作");

    pthread_mutex_unlock(&self->_lock);
}

#pragma mark - dealloc

- (void)dealloc {
    NSLog(@"🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...");
    
    pthread_mutex_destroy(&self->_lock);
    pthread_cond_destroy(&self->_condition);
}

@end
// 打印 🖨️:
🧑‍💻🧑‍💻🧑‍💻 delete begin
🧑‍💻🧑‍💻🧑‍💻 add begin
数组执行添加元素操作
数组执行删除元素操作
🧑‍🎤🧑‍🎤🧑‍🎤 dealloc 同时释放🔒...
```

**由于 `pthread.h` 文件过大，且完全没有注释，这边先把其他锁的使用都看完再分析 `pthread.h` 文件.**

**参考链接:🔗**
[iOS 锁 部分一](https://www.jianshu.com/p/8ce323dbc491)
