#  iOS_NSLock

## 特点
1. 基于 `mutex` 基本锁的封装，更加面向对象，等待锁的线程会处于休眠状态。
2. 遵守 `NSLocking` 协议，`NSLocking` 协议中两个方法如下:
  ```
  
  ```
3. 可能会用到的方法。
  1. 初始化跟其他 `OC` 对象一样，直接进行 `alloc` 和 `init` 操作。
  2. `- (void)lock;` 加锁。
  3. `- (void)unlock;` 解锁。
  4. `- (BOOL)tryLock;` 尝试加锁。
  5. `- (BOOL)lockBeforeDate:(NSDate *)limit;` 在某一个时间点之前等待加锁。 


