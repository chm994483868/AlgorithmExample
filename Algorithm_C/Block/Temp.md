
// 这里针对 __block 变量解释一下：
// __block NSObject *object = [[NSObject alloc] init]; 
// __Block_byref_object_0 结构体
// 首先 NSObject 对象是处于堆区的，__block 结构体实例是处于栈区的。
// Block 发生 copy 操作从栈区到堆区时：原始的 NSObject 对象是不动的，是 __block 结构体实例被复制到了堆区。
// 且复制以后，栈区的 __block 结构体实例会断开对 NSObject 对象的引用。
