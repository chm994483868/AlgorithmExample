# iOS 多线程知识体系构建(十二)：GCD 源码：dispatch_barrier_async、dispatch_barrier_sync 函数

> &emsp;

## dispatch_barrier_async
&emsp;`dispatch_barrier_async` 提交 barrier block 以在调度队列上异步执行。（同 `dispatch_async` 不会阻塞当前线程，直接返回执行接下来的语句，但是后添加的 block 则是等到 barrier block 执行完成后才会开始执行。）

&emsp;将一个 block 提交到诸如 `dispatch_async` 之类的调度队列中，但将该 block 标记为 barrier（`DC_FLAG_BARRIER`）屏障（仅与 `DISPATCH_QUEUE_CONCURRENT` 并发队列相关）。

&emsp;`dq` 参数是 block 提交到的目标调度队列。系统将在目标队列上保留引用，直到该 block 执行完成为止。

&emsp;`work` 参数是提交到目标调度队列的 block（该函数内部代表调用者执行 `Block_copy` 和 `Block_release`）。
```c++
#ifdef __BLOCKS__
void
dispatch_barrier_async(dispatch_queue_t dq, dispatch_block_t work)
{
    dispatch_continuation_t dc = _dispatch_continuation_alloc();
    
    // dc_flags 中添加 DC_FLAG_BARRIER 标记，标记此 work 是一个屏障 block，
    // 然后函数内部的别的内容都和 dispatch_async
    uintptr_t dc_flags = DC_FLAG_CONSUME | DC_FLAG_BARRIER;
    
    dispatch_qos_t qos;

    qos = _dispatch_continuation_init(dc, dq, work, 0, dc_flags);
    _dispatch_continuation_async(dq, dc, qos, dc_flags);
}
#endif
```
&emsp;看到 `dispatch_barrier_async` 函数内部出了 `dc_flags` 赋值和 `dispatch_async` 不同，其它调用完全如出一辙。
## dispatch_barrier_sync
```c++
void
dispatch_barrier_sync(dispatch_queue_t dq, dispatch_block_t work)
{
    // dc_flags 里面添加了 DC_FLAG_BARRIER 标记
    uintptr_t dc_flags = DC_FLAG_BARRIER | DC_FLAG_BLOCK;
    
    if (unlikely(_dispatch_block_has_private_data(work))) {
        return _dispatch_sync_block_with_privdata(dq, work, dc_flags);
    }
    
    _dispatch_barrier_sync_f(dq, work, _dispatch_Block_invoke(work), dc_flags);
}
```


## 参考链接
**参考链接:🔗**
+ [libdispatch苹果源码](https://opensource.apple.com/tarballs/libdispatch/)
+ [GCD源码分析1 —— 开篇](http://lingyuncxb.com/2018/01/31/GCD源码分析1%20——%20开篇/)
+ [扒了扒libdispatch源码](http://joeleee.github.io/2017/02/21/005.扒了扒libdispatch源码/)
+ [GCD源码分析](https://developer.aliyun.com/article/61328)
+ [关于GCD开发的一些事儿](https://www.jianshu.com/p/f9e01c69a46f)
+ [GCD 深入理解：第一部分](https://github.com/nixzhu/dev-blog/blob/master/2014-04-19-grand-central-dispatch-in-depth-part-1.md)
+ [dispatch_once 详解](https://www.jianshu.com/p/4fd27f1db63d)
+ [透明联合类型](http://nanjingabcdefg.is-programmer.com/posts/23951.html)
+ [变态的libDispatch结构分析-dispatch_object_s](https://blog.csdn.net/passerbysrs/article/details/18228333?utm_source=blogxgwz2)
+ [深入浅出 GCD 之基础篇](https://xiaozhuanlan.com/topic/9168375240)
+ [从源码分析Swift多线程—DispatchGroup](http://leevcan.com/2020/05/30/从源码分析Swift多线程—DispatchGroup/)
+ [GCD源码分析（一）](https://www.jianshu.com/p/bd629d25dc2e)
+ [GCD-源码分析](https://www.jianshu.com/p/866b6e903a2d)
+ [GCD底层源码分析](https://www.jianshu.com/p/4ef55563cd14)
+ [GCD源码吐血分析(1)——GCD Queue](https://blog.csdn.net/u013378438/article/details/81031938)
+ [c/c++:计算可变参数宏 __VA_ARGS__ 的参数个数](https://blog.csdn.net/10km/article/details/80760533)
+ [OC底层探索(二十一)GCD异步、GCD同步、单例、信号量、调度组、栅栏函数等底层分析](https://blog.csdn.net/weixin_40918107/article/details/109520980)
+ [iOS源码解析: GCD的信号量semaphore](https://juejin.cn/post/6844904122370490375)


