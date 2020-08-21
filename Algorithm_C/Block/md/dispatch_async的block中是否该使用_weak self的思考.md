#  dispatch_async的block中是否该使用_weak self的思考

> blcok 中捕获 self，一定会延长 self 的生命周期。如果 self 同时持有 block，则会导致循环引用。循环引用之外的延长 self 的生命周期是很容易忽略的一个点。

## 延长 self 的生命周期
```objective-c
dispatch_async(globalQueue_DEFAULT, ^{
    // do something
    
    // 下面在主队列里面要执行的 block 捕获了 self，self 的生命周期被延长，
    // 直到 block 被释放后才会释放被其 retain 的 self
    dispatch_async(dispatch_get_main_queue(), ^{
        self.view.backgroundColor = [UIColor redColor];
    });
});
```
```objective-c
// 下面在并行队列里面要执行的 block 没有 retain self
__weak typeof(self) _self = self;
dispatch_async(globalQueue_DEFAULT, ^{
    __strong typeof(_self) self = _self; // 保证在下面的执行过程中 self 不会被释放，执行结束后 self 会被释放
    if (!self) return;
    // do something
    // ...
    dispatch_async(dispatch_get_main_queue(), ^{
        // 此时如果能进来，表示此时 self 是存在的
        self.view.backgroundColor = [UIColor redColor];
    });
});
```
当在 `dispatch_async` 的异步线程的 `block` 中捕获到 `self` 时，`self` 会被 `retained`，当 `block` 执行完毕后 `block` 释放销毁，同时才会释放它所 `retain` 的 `self`。这意味着：当 `block` 执行完毕后，`self` 如果没有别的强引用时它的生命周期才会结束。
上例中的第二 个 `block` 是在主队列中，它保证了 `self` 一直存活着当这个 `block` 被执行的时候。

而此时在程序中存在潜在危险的就是：**延长了 `self` 的生命周期。**

如果你明确的不希望延长 `UIViewController` 对象的生命周期，而是当 `block` 被执行的时候去检查 `UIViewController` 对象到底是否存在。你可以使用 `_weak typedef(self) _self = self;` 防止 `self` 被 `block`  `reatain` 。

同时需要注意的是 `block` 最后都会被执行，不管 `UIViewController` 是否存活。

## 在并行队列的异步操作 block 内部，释放 retain 的 UI 对象

```objective-c
dispatch_async(globalQueue_DEFAULT, ^{
    // self 假如在此处捕获的 self 是一个 UI 对象，且此 block 是该 UI 对象的最后一个持有者，一些操作使该 UI 对象被释放，由于此时在非主线程，且 此时 UI 对象的 dealloc 里面有一些 UI 操作，由于 UI 操作必须在主线程进行，但是此时是在非主线程，所以会导致 crash (怎么才能模拟出这种场景呢😖)
};
```

**参考链接:🔗**

[dispatch_async的block中是否该使用_weak self](https://www.jianshu.com/p/c374b7727d79)
[dispatch_async的block里面需要__weak self 吗？ #41](https://github.com/ibireme/YYKit/issues/41)
[线程安全类的设计](https://objccn.io/issue-2-4/)
