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

```


