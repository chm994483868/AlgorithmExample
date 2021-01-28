#  iOS《Concurrency Programming Guide-Operation Queues》官方文档

## Operation Queues
&emsp;Cocoa operations 是一种面向对象的方法，用于封装要异步执行的工作。operations 被设计为与 operation queue 一起使用，或者单独使用。因为它们是基于 Objective-C 的，所以 operations 在 OS X 和 iOS 中最常用于 Cocoa-based 的应用程序。

&emsp;本章介绍如何定义和使用 operations。

### About Operation Objects（关于 Operation 对象）
&emsp;operation 对象是 NSOperation 类（在 Foundation framework 中）的实例，用于封装希望应用程序执行的工作。NSOperation 类本身是一个抽象基类，必须对其进行子类化才能执行任何有用的工作。尽管这个类是抽象的，但它确实提供了大量的 infrastructure，以尽量减少你在自己的子类中必须完成的工作量。此外，Foundation framework 提供了两个具体的子类，你可以在现有代码中使用它们。表 2-1 列出了这些类，以及如何使用每个类的摘要。

&emsp;Table 2-1  Operation classes of the Foundation framework

| Class | Description |
| --- | --- |
| NSInvocationOperation | 使用的类是基于应用程序中的对象和选择器创建 operation 对象。如果你具有已经执行所需任务的现有方法，则可以使用此类。因为它不需要子类化，所以还可以使用此类以更动态的方式创建 operation 对象。 有关如何使用此类的信息，请参见下面的 Creating an NSInvocationOperation Object |
| NSBlockOperation | as-is 使用的类用于同时执行一个或多个 block 对象。因为一个 block operation 对象可以执行多个块，所以它使用 group 语义进行操作；只有当所有相关的 blocks 都执行完毕时，operation 本身才被视为已完成。有关如何使用此类的信息，请参见下面的 Creating an NSBlockOperation Object。此类在 OS X v10.6 和更高版本中可用。有关 blocks 的更多信息，请参见 Blocks Programming Topics。 |
| NSOperation | 用于定义自定义 operation 对象的基类。子类化 NSOperation 使你能够完全控制自己操作的实现，包括更改操作执行和报告其状态的默认方式。有关如何定义自定义 operation 对象的信息，请参见 Defining a Custom Operation Object。 |

&emsp;所有 operation 对象都支持以下关键功能：

+ 支持在 operation 对象之间建立 graph-based 的依赖关系。这些依赖关系阻止给定的 operation 在其所依赖的所有 operations 完成运行之前运行。有关如何配置依赖项的信息，请参见 Configuring Interoperation Dependencies。
+ 支持可选的完成 block，该 block 在 operation 的主任务完成后执行。（仅限 OS X v10.6 及更高版本）有关如何设置完成 block 的信息，请参见 Setting Up a Completion Block。
+ 支持使用 KVO 通知监视对 operations 执行状态的更改。有关如何观察 KVO 通知的信息，请参见 Key-Value Observing Programming Guide。
+ 支持对 operations 进行优先级排序，从而影响它们的相对执行顺序。有关详细信息，请参见 Changing an Operation’s Execution Priority。
+ 支持取消语义，允许你在 operation 执行时停止操作。有关如何取消 operations 的信息，请参见 Canceling Operations。有关如何在自己的 operations 中支持取消的信息，请参见 Responding to Cancellation Events。

&emsp;Operations 旨在帮助你提高应用程序中的并发等级。Operations 也是将应用程序的行为组织和封装为简单的离散块的好方法。你可以将一个或多个 operation 对象提交到队列，并让相应的工作在一个或多个单独的线程上异步执行，而不是在应用程序的主线程上运行一些代码。

### Concurrent Versus Non-concurrent Operations（并行与非并行操作）
&emsp;尽管通常通过将 operations 添加到 operation queue 来执行操作，但不需要这样做。也可以通过调用 operation 对象的 start 方法手动执行 operation 对象，但这样做并不保证 operation 与其余代码同时运行。NSOperation 类的 isConcurrent 方法告诉你操作相对于调用其 start 方法的线程是同步运行还是异步运行。默认情况下，此方法返回 NO，这意味着 operation 在调用线程中同步运行。

&emsp;如果要实现一个并发 operation，也就是相对于调用线程异步运行的 operations，则必须编写额外的代码以异步启动该 operation。例如，你可以调度一个单独的线程、调用一个异步系统函数或执行任何其他 operations，以确保 start 方法启动任务并立即返回，而且很可能在任务完成之前返回。

&emsp;大多数开发人员不应该需要实现并发 operation 对象。如果总是将 operations 添加到 operation queue 中，则不需要实现并发 operations。将非并发 operation 提交到 operation queue 时，队列本身会创建一个线程来运行 operation。因此，将非并发 operations 添加到 operation queue 仍然会导致 operation 对象代码的异步执行。只有在需要异步执行 operations 而不将其添加到 operation queue 的情况下，才需要定义并发操作。

&emsp;有关如何创建并发操作（concurrent operation）的信息，请参见 NSOperation Class Reference 中的 Configuring Operations for Concurrent Execution。

### Creating an NSInvocationOperation Object（创建 NSInvocationOperation 对象）
&emsp;NSInvocationOperation 类是 NSOperation 的一个具体子类，在运行时，它会调用在指定对象上指定的选择器。使用此类可以避免为应用程序中的每个任务定义大量自定义 operation 对象；尤其是在你正在修改现有应用程序并且已经拥有执行必要任务所需的对象和方法的情况下。如果要调用的方法可能会根据情况发生更改，也可以使用它。例如，可以使用调用操作来执行基于用户输入动态选择的选择器。

&emsp;创建 invocation operation 的过程很简单。创建并初始化类的新实例，将要执行的所需对象和选择器传递给初始化方法。清单 2-1 显示了一个自定义类中的两个方法，它们演示了创建过程。taskWithData: 方法创建一个新的 invocation 对象，并为其提供另一个方法的名称，该方法包含任务实现。

&emsp;Listing 2-1  Creating an NSInvocationOperation object
```c++
@implementation MyCustomClass
- (NSOperation*)taskWithData:(id)data {
    NSInvocationOperation* theOp = [[NSInvocationOperation alloc] initWithTarget:self selector:@selector(myTaskMethod:) object:data];
    return theOp;
}
 
// This is the method that does the actual work of the task.
- (void)myTaskMethod:(id)data {
    // Perform the task.
}
@end
```

### Creating an NSBlockOperation Object（创建 NSBlockOperation 对象）
&emsp;NSBlockOperation 类是 NSOperation 的一个具体子类，它充当一个或多个 block 对象的包装器。此类为已经在使用 operation queues 并且不想同时创建 dispatch queues 的应用程序提供了面向对象的包装器。你还可以使用 block operations 来利用操作依赖项、KVO 通知和 dispatch queues 中可能不可用的其他功能。

&emsp;创建 block operation 时，通常在初始化时至少添加一个 block；以后可以根据需要添加更多 blocks。当执行一个 NSBlockOperation 对象时，该对象将其所有的 blocks 提交到默认优先级并发 dispatch queue。然后，对象等待所有 blocks 执行完毕。当最后一个 block 完成执行时，operation 对象将自己标记为已完成。因此，可以使用 block operation 来跟踪一组正在执行的 blocks，就像使用线程连接来合并多个线程的结果一样。区别在于，由于 block operation 本身在单独的线程上运行，所以应用程序的其他线程可以在等待 block operation 完成时继续工作。

&emsp;清单 2-2 展示了如何创建 NSBlockOperation 对象的简单示例。block 本身没有参数，也没有显著的返回结果。

&emsp;Listing 2-2  Creating an NSBlockOperation object
```c++
NSBlockOperation* theOp = [NSBlockOperation blockOperationWithBlock: ^{
   NSLog(@"Beginning operation.\n");
   // Do some work.
}];
```
&emsp;创建 block operation 对象后，可以使用 `addExecutionBlock:` 方法向其添加更多 blocks。如果需要串行执行 blocks，则必须将它们直接提交到所需的 dispatch queue。

### Defining a Custom Operation Object（定义自定义 Operation 对象）
&emsp;如果 block operation 和 invocation operation 对象不能完全满足应用程序的需要，可以直接将 NSOperation 子类化，并添加所需的任何行为。NSOperation 类为所有 operation 对象提供了一个通用的子类化点（general subclassing point）。该类还提供了大量的 infrastructure 来处理依赖项和 KVO 通知所需的大部分工作。但是，有时你可能仍需要补充现有的 infrastructure，以确保你的 operations 正常运行。你需要做的额外工作的数量取决于你是实现非并发 operation 还是并发 operation。

&emsp;定义非并发 operation 比定义并发 operation 简单得多。对于非并发 operation，你所要做的就是执行主任务并适当地响应取消事件；现有的类 infrastructure 为你完成所有其他工作。对于并发操作，必须用自定义代码替换一些现有的 infrastructure。以下部分将向你展示如何实现这两种类型的对象。

#### Performing the Main Task（执行主要任务）
&emsp;每个 operation 对象至少应实现以下方法：

+ 自定义初始化方法（A custom initialization method）
+ `main` 

&emsp;你需要一个自定义初始化方法来将 operation 对象置于已知状态（known state），并需要一个自定义 `main` 方法来执行任务。当然，你可以根据需要实现其他方法，例如：

+ 你计划从 `main` 方法的实现中调用的自定义方法
+ 用于设置数据值和访问 operation 结果的访问器方法（Accessor methods）
+ NSCoding 协议的方法，允许你 archive and unarchive operation 对象

&emsp;清单 2-3 显示了一个自定义 NSOperation 子类的启动模板。（此清单没有显示如何处理取消，但显示了通常使用的方法。有关处理取消的信息，请参见  Responding to Cancellation Events。）此类的初始化方法将单个对象作为数据参数，并在 operation 对象中存储对该对象的引用。在将结果返回给应用程序之前，`main` 方法表面上将对该数据对象起作用。

&emsp;Listing 2-3  Defining a simple operation object
```c++
@interface MyNonConcurrentOperation : NSOperation

@property id (strong) myData;
- (id)initWithData:(id)data;

@end
 
@implementation MyNonConcurrentOperation

- (id)initWithData:(id)data {
   if (self = [super init])
      myData = data;
   return self;
}
 
- (void)main {
   @try {
      // Do some work on myData and report the results.
   }
   @catch(...) {
      // Do not rethrow exceptions.
   }
}

@end
```
&emsp;有关如何实现 NSOperation 子类的详细示例，请参见 [NSOperationSample](https://developer.apple.com/library/archive/samplecode/NSOperationSample/Introduction/Intro.html#//apple_ref/doc/uid/DTS10004184)。

#### Responding to Cancellation Events（响应取消事件）
&emsp;在一个 operation 开始执行之后，它继续执行它的任务，直到它完成或者直到你的代码显式取消该 operation 为止。取消可以在任何时候发生，甚至在 operation 开始执行之前。尽管 NSOperation 类为 clients 提供了取消 operation 的方法，但根据需要，识别取消事件是自愿的（recognizing the cancellation event is voluntary by necessity）。如果一个 operation 被完全终止，可能就没有办法回收已分配的资源。因此，operation 对象需要检查取消事件，并在它们发生在操作过程中时正常退出。

&emsp;要在 operation 对象中支持取消，你所要做的就是定期从自定义代码中调用对象的 `isCancelled` 方法，如果返回 YES，则立即返回。无论操作的持续时间如何，或者你是直接将 NSOperation 子类化还是使用其具体子类之一，支持取消操作都很重要。`isCancelled` 方法本身是非常轻量级的，可以频繁调用，而不会造成任何明显的性能损失。在设计 operation 对象时，应考虑在代码中的以下位置调用 `isCancelled` 方法：

+ 在执行任何实际工作之前
+ 在循环的每次迭代中至少一次，如果每次迭代相对较长，则更频繁
+ 在代码中相对容易中止操作的任何地方

&emsp;清单 2-4 提供了一个非常简单的示例，说明如何在 operation 对象的 `main` 方法中响应取消事件。在这种情况下，每次通过 while 循环调用 `isCancelled` 方法，允许在工作开始之前快速退出，并定期再次调用。

&emsp;Listing 2-4  Responding to a cancellation request
```c++
- (void)main {
   @try {
      BOOL isDone = NO;
 
      while (![self isCancelled] && !isDone) {
          // Do some work and set isDone to YES when finished
      }
   }
   @catch(...) {
      // Do not rethrow exceptions.
   }
}
```
&emsp;尽管前面的示例不包含清理代码，但是你自己的代码应该确保释放自定义代码分配的所有资源。

#### Configuring Operations for Concurrent Execution（为并发执行配置 Operations）
&emsp;默认情况下，Operation 对象以同步方式执行，也就是说，它们在调用其 `start` 方法的线程中执行任务。但是，由于 operation queues 为非并发 operations 提供线程，因此大多数 operations 仍然异步运行。但是，如果你计划手动执行 operations，但仍希望它们异步运行，则必须采取适当的操作以确保它们能够异步运行。你可以通过将 operations 对象定义为并发 operations 来实现这一点。

&emsp;表 2-2 列出了为实现并发 operation 而通常重写的方法。

&emsp;Table 2-2  Methods to override for concurrent operations

| Method | Description |
| --- | --- |
| start | （Required）所有并发 operations 都必须重写此方法，并用自己的自定义实现替换默认实现。要手动执行操作，可以调用其 start 方法。因此，此方法的实现是操作的起点，也是设置执行任务的线程或其他执行环境的地方。你的实现在任何时候都不能调用 super。 |
| main | （Optional）此方法通常用于实现与 operations 对象关联的任务。尽管可以在 start 方法中执行任务，但使用此方法实现任务可以使设置代码和任务代码更清晰地分离。 |
| isExecuting isFinished | （Required）并发 operations 负责设置其执行环境，并向外部 clients 报告该环境的状态。因此，并发 operations 必须维护一些状态信息，以便知道它何时执行任务以及何时完成任务。然后，它必须使用这些方法报告该状态。
这些方法的实现必须能够安全地同时从其他线程调用。更改这些方法报告的值时，还必须为预期的 key path 生成适当的 KVO 通知。 |
| isConcurrent |  |





























































## 参考链接
**参考链接:🔗**
+ [NSOperation](https://developer.apple.com/documentation/foundation/nsoperation?language=occ)
+ [NSBlockOperation](https://developer.apple.com/documentation/foundation/nsblockoperation?language=occ)
+ [NSInvocationOperation](https://developer.apple.com/documentation/foundation/nsinvocationoperation?language=occ)
+ [NSOperationQueue](https://developer.apple.com/documentation/foundation/nsoperationqueue?language=occ)
+ [Operation Queues](https://developer.apple.com/library/archive/documentation/General/Conceptual/ConcurrencyProgrammingGuide/OperationObjects/OperationObjects.html#//apple_ref/doc/uid/TP40008091-CH101-SW1)
+ [iOS 多线程：『NSOperation、NSOperationQueue』详尽总结](https://www.jianshu.com/p/4b1d77054b35)
+ [并发编程：API 及挑战](https://objccn.io/issue-2-1/)
