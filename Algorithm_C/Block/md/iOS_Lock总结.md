#  iOS_Lock总结

> 锁是常用的同步工具。一段代码段在同一个时间只能允许被有限个线程访问，比如一个线程 A 进入需要保护代码之前添加简单的互斥锁，另一个线程 B 就无法访问这段保护代码了，只有等待前一个线程 A 执行完被保护的代码后解锁，B 线程才能访问被保护的代码段。

## iOS 中的八大锁
```
// NSLocking 协议，下面提到锁的类型只要是 NS 开头的都会遵守此协议
@protocol NSLocking
- (void)lock;
- (void)unlock;
@end
```
1. NSLock:
  继承自 NSObject 并遵循 NSLocking 协议，lock 方法加锁，unlock 方法解锁，tryLock 尝试并加锁，如果返回 true 表示加锁成功，返回 false 表示加锁失败，谨记返回的 BOOL 表示加锁动作的成功或失败，并不是能不能加锁，即使加锁失败也会不会阻塞当前线程。lockBeforeDate: 是在指定的 Date 之前尝试加锁，如果在指定的时间之前都不能加锁，则返回 NO，且会阻塞当前线程。大概可以使用在：先预估上一个临界区的代码执行完毕需要多少时间，然后在这个时间之后为另一个代码段来加锁。
  ```objective-c
  __weak typeof(self) _self = self;
  // 线程 1 
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
  
  sleep(1); // 睡 1 秒，保证先让线程 1 加锁成功 
  
  // 线程 2 
  dispatch_async(global_queue, ^{
      __strong typeof(_self) self = _self;
      if (!self) return;
      
      // 如果此处加锁失败，则阻塞当前线程，下面的代码不会执行，
      // 直到等到 lock 被其他线程释放了，它可以加锁了，才会接着执行下面的代码
      [self.lock lock];
      for (unsigned int i = 0; i < 10000; ++i) {
          self.sum++;
      }
      [self.lock unlock];
      
      // 如果使用 tryLock 的话，即使加锁失败也不会阻塞当前线程
      if ([self.lock tryLock]) {
        for (unsigned int i = 0; i < 10000; ++i) {
          self.sum++;
        }
        [self.lock unlock];
      } else {
        NSLog(@"tryLock 失败，会直接来到这里，不会阻塞当前线程");
      }
  }
      NSLog(@"😵😵😵 %ld", (long)self.sum);
  });
  ```
  示例中代码，线程 1 首先获得 lock  并上锁，所以线程 2 中的 lock 加锁失败，此时它会阻塞线程 2，一直等待下去，直到 线程 1 执行了 unlock，线程 2 就立即加锁成功，开始执行线程 2 中的后续代码。
  如果是用 tryLock 的话则不阻塞线程 2，如果 tryLock 返回 true 则执行 if 里面的代码，否则执行 else 里面的代码不会阻塞当前线程。（不管返回 true 或者 false 都会继续执行后续代码）
  ```objective-c
      __weak typeof(self) _self = self;
      // 线程 1
      dispatch_async(global_queue, ^{
          __strong typeof(_self) self = _self;
          if (!self) return;

          [self.lock lock];
          for (unsigned int i = 0; i < 10000; ++i) {
              self.sum++;
          }
          sleep(3);
          [self.lock unlock];
          NSLog(@"👿👿👿 %ld", (long)self.sum);
      });
      
      // 线程 2
      dispatch_async(global_queue, ^{
          __strong typeof(_self) self = _self;
          if (!self) return;
          sleep(1); // 保证让线程 1 先获得锁
          
          // 如果此处用 1，则在这个时间点不能获得锁
          // 如果是用大于 2 的数字，则能获得锁
          // 且这个 if 函数是会阻塞当前线程的
          if ([self.lock lockBeforeDate: [NSDate dateWithTimeIntervalSinceNow:1]]) {
              for (unsigned int i = 0; i < 10000; ++i) {
                  self.sum++;
              }
              [self.lock unlock];
          } else {
              NSLog(@"lockBeforeDate 失败，会直接来到这里吗，会不阻塞当前线程吗？");
          }
          
          NSLog(@"😵😵😵 %ld", (long)self.sum);
      });
  ```
  `[self.lock lockBeforeDate: [NSDate dateWithTimeIntervalSinceNow:1]]`，lockBeforeDate: 方法会在指定 Date 之前尝试加锁，且这个过程是会阻塞线程 2 的，如果在指定时间之前都不能加锁，则返回 false，在指定时间之前能加锁，则返回 true。
  _priv 和 name，检测各个阶段，_priv 一直是 NULL。name 是用来标识的，用来输出的时候作为 lock 的名称。
  如果是三个线程，那么一个线程在加锁的时候，其余请求锁的的线程将形成一个等待队列，按先进先出原则，这个结果可以通过修改线程优先级进行测试得出。
  
2. NSConditionLock
  NSConditionLock 和 NSLock 类似，同样是继承自 NSObject 和遵循 NSLocking 协议，加解锁 try 等方法都类似，只是多了一个 condition 属性，以及每个操作都多了一个关于 condition 属性的方法，例如 tryLock 和 tryLockWhenCondition:，NSConditionLock 可以称为条件锁。只有 condition 参数与初始化的时候相等或者上次解锁后设置的 condition 相等，lock 才能正确的进行加锁操作。unlockWithCondition: 并不是当 condition 符合条件时才解锁，而是解锁之后，修改 condition 的值为入参，当使用 unlock 解锁时， condition 的值保持不变。如果初始化用 init，则 condition 默认值为 0。`lockWhenCondition:` 和 `lock` 方法类似，加锁失败会阻塞当前线程，一直等下去，直到能加锁成功。`tryLockWhenCondition:` 和 `tryLock` 类似，表示尝试加锁，即使加锁失败也不会阻塞当前线程，但是同时满足 lock 是空闲状态并且 condition 符合条件才能尝试加锁成功。从上面看出，NSConditionLock 还可以实现任务之间的依赖。
  
3. NSRecursiveLock
  NSRecursiveLock 是递归锁，和 NSLock 的区别在于，它可以在同一个线程中重复加锁也不会导致死锁。NSRecursiveLock 会记录加锁和解锁的次数，当二者次数相等时，此线程才会释放锁，其它线程才可以上锁成功。
  ```objective-c
  NSRecursiveLock *lock = [[NSRecurseive alloc] init];
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    static void (^RecursieveBlock)(int);
    RecursiveBlock = ^(int value) {
        [lock lock];
        if (value > 0) {
            NSLog(@"value: %d", value);
            RecursiveBlock(value - 1);
        }
        [lock unlock];
    };
    RecursiveBlock(2);
  });
  ```
  如上示例，如果用 NSLock 的话，lock 先锁上，但未执行解锁的时候，就会进入递归的下一层，并再次请求上锁，阻塞了该线程，线程被阻塞了，自然后面的解锁代码就永远不会执行，而形成了死锁。而 NSRecursiveLock 递归锁就是为了解决这个问题。

4. NSCondition
    ```objective-c
    @interface NSCondition : NSObject <NSLocking> {
    @private
        void *_priv;
    }

    - (void)wait;
    - (BOOL)waitUntilDate:(NSDate *)limit;
    - (void)signal;
    - (void)broadcast;

    @property (nullable, copy) NSString *name NS_AVAILABLE(10_5, 2_0);

    @end
    ```
    NSCondition 的对象实际上作为一个锁和一个线程检查器，锁上之后其它线程也能上锁，而之后可以根据条件决定是否继续运行线程，即线程是否要进入 waiting 状态，经测试，NSCondition 并不会像上文的那些锁一样，先轮询，而是直接进入 waiting 状态，当其它线程中的该锁执行 signal 或者 broadcast 方法时，线程被唤醒，继续运行之后的方法。
    也就是使用 NSCondition 的模型为:
    
    锁定条件对象。
    测试是否可以安全的履行接下来的任务。
    
    如果布尔值为假，调用条件对象的 wait 或者 waitUntilDate: 方法来阻塞线程。再从这些方法返回，则转到步骤 2 重新测试你的布尔值。（继续等待信号和重新测试，直到可以安全的履行接下来的任务，waitUntilDate: 方法有个等待时间限制，指定的时间到了，则返回 NO，继续运行接下来的任务）
    
     （如果数组为空，则执行 wait 阻塞线程，并将锁让出，这里会将锁让出去，所以下面的 addObj 线程才能获得锁，当接收到信号时会再次加锁，并从 wait 那里继续向下执行。）
     
     如果布尔值为真，执行接下来的任务。
     当任务完成后，解锁条件对象。
     
     而步骤 3 说的等待信号，既线程 2 执行 [lock signal] 发送的信号。
     其中 signal 和 broadcast 方法的区别在于，signal 只是一个信号量，只能唤醒一个等待的线程，想唤醒多个就得**多次调用**，而 broadcast 可以唤醒所有在等待的线程，如果没有等待的线程，这两个方法都没有作用。
    
5. @synchronized 
  ```objective-c
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
          @synchronized(self) {
              sleep(2);
              NSLog(@"线程1");
          }
          NSLog(@"线程1解锁成功");
  });
      
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
          sleep(1);
          @synchronized(self) {
              NSLog(@"线程2");
          }
  });
  // 打印:
  线程1
  线程1解锁成功
  线程2
  ```
  @synchronized(object) 指令使用的 object 为该锁的唯一标识，只有当标识相同时，才满足互斥，所以如果线程 2 中的 @synchronized(self) 改为 @synchronized(self.view)，则线程 2 就不会被阻塞，@synchronized 指令实现锁的优点就是我们不需要在代码中显式的创建锁对象，便可以实现锁的机制，但作为一种预防措施，@synchronized 块会隐式的添加一个异常处理例程来保护代码，**该处理例程会在异常抛出的时候自动释放互斥锁**。@synchronized 还有一个好处就是不用担心忘记解锁了。
  如果在 @synchronized(object) {} 内部 object 被释放或被设为 nil，从测试结果来看，不会产生问题，但如果 object 一开始就是 nil，则失去了加锁的功能。不过虽然 nil 不行，但是 [NSNull null] 是可以的。 
  
  iOS 12 之后是底层封装 `os_unfair_recursive_lock` 的递归锁。

6. dispatch_semaphore
  ```objective-c
  dispatch_semaphore_create(long value);
  dispatch_semaphore_wait(dispatch_semaphore_t dsema, dispatch_time_t timeout);
  dispatch_semaphore_signal(dispatch_semaphore_t dsema);
  ```
  dispatch_semaphore 是 GCD 用来同步的一种方式，与他相关的只有三个函数，一个是创建信号量，一个是等待信号量，一个是发送信号。
  dispatch_semaphore 和 NSCondition 类似，都是一种基于信号的同步方式，但 NSCondition 信号只能发送，不能保存（如果没有线程在等待，则发送的信号会失效）。而 dispatch_semaphore 能保存发送的信号。dispatch_semaphore 的核心是 dispatch_semaphore_t 类型的信号量。
  dispatch_semaphore_create(1) 方法可以创建一个 dispatch_semaphore_t 类型的信号量，设定信号量的初始化值为 1。注意，这里的传入参数必须大于等于 0，否则 dispatch_semaphore 会返回 NULL。
  dispatch_semaphore_wait(signal, overTime) 方法会判断 signal 的信号值是否大于 0，大于 0 不会阻塞线程，消耗掉一个信号，执行后续任务。如果信号值为 0，该线程会和 NSCondition 一样直接进入 waiting 状态，等待其他线程发送信号唤醒线程去执行后续任务，或者当 overTime 时限到了，也会执行后续任务。
  dispatch_semaphore_signal(signal) 发送信号，如果没有等待的线程调用信号，则使 signal 信号值加 1（做到对信号的保存）。
  一个 dispatch_semaphore_wait（signal, overTime）方法会去对应一个 dispatch_semaphore_signal(signal) 看起来像 NSLock 的 lock 和 unlock，其实可以这样理解，区别只在于有信号量这个参数，lock unlock 只能同一时间，一个线程访问被保护的临界区，而如果 dispatcch_semaphore 的信号量初始值为 x，则可以有 x 个线程同时访问被保护的临界区。

7. OSSpinLock 
  自旋锁，也只有加锁、解锁和尝试加锁三个方法。和 NSLock 不同的是 NSLock 请求加锁失败的话，会先轮询，但一秒后便会使线程进入 waiting 状态，等待唤醒。而 OSSpinLock 会一直轮询，等待时会消耗大量 CPU 资源，不适用于较长时间的任务。

8. pthread_mutex
  ```objective-c
  int pthread_mutex_init(pthread_mutex_t * __restrict, const pthread_mutexattr_t * __restrict);
  int pthread_mutex_lock(pthread_mutex_t *);
  int pthread_mutex_trylock(pthread_mutex_t *);
  int pthread_mutex_unlock(pthread_mutex_t *);
  int pthread_mutex_destroy(pthread_mutex_t *);
  int pthread_mutex_setprioceiling(pthread_mutex_t * __restrict, int, int * __restrict);
  int pthread_mutex_getprioceiling(const pthread_mutex_t * __restrict, int * __restrict);
  ```
  pthread_mutex 是 C 语言下多线程互斥锁的方式。
  首先是第一个方法，初始化一个锁，_restrict 为互斥锁的类型，传 NULL 为默认类型，一共四个类型：
  ```c++
  PTHREAD_MUTEX_NORMAL // 缺省类型，也就是普通类型，当一个线程加锁后，其余请求锁的线程将形成一个队列，并在解锁后先进先出原则获得锁。
  PTHREAD_MUTEX_ERRORCHECK // 检错锁，如果同一个线程请求同一个锁，则返回 EDEADLK，否则与普通锁类型动作相同。这样就保证当不允许多次加锁时不会出现嵌套情况下的死锁
  PTHREAD_MUTEX_RECURSIVE //递归锁，允许同一个线程对同一锁成功获得多次，并通过多次 unlock 解锁。
  PTHREAD_MUTEX_DEFAULT // 适应锁，动作最简单的锁类型，仅等待解锁后重新竞争，没有等待队列。
  ```
  pthread_mutex_trylock 和 trylock 不同，trylock 返回的是 YES 和 NO，pthread_mutex_trylock 加锁成功返回的是 0，失败返回的是错误提示码。
  pthread_mutex_destroy 为释放资源。
  
**参考链接:🔗**
+ [iOS 常见知识点（三）：Lock](https://www.jianshu.com/p/ddbe44064ca4)
+ [iOS中保证线程安全的几种方式与性能对比](https://www.jianshu.com/p/938d68ed832c)
+ [关于 @synchronized，这儿比你想知道的还要多](http://yulingtianxia.com/blog/2015/11/01/More-than-you-want-to-know-about-synchronized/)
    
  
  
  
  
