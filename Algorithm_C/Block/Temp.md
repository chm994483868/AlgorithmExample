

**小测试**
+ A:
```
void exampleA() {
    // ARC 和 MRC 下均为栈区 Block
    char a = 'A';
    NSLog(@"🔔🔔🔔 %@", ^{ printf("%c\n", a);});
}
// MRC: 🔔🔔🔔 <__NSStackBlock__: 0x7ffeefbff538>
// ARC: 🔔🔔🔔 <__NSStackBlock__: 0x7ffeefbff538>

void exampleA() {
    // ARC 和 MRC 下均为全局 Block
    NSLog(@"🔔🔔🔔 %@", ^{ printf("🟪🟪🟪");});
}
// ARC: 🔔🔔🔔 <__NSGlobalBlock__: 0x100002048>
// MRC: 🔔🔔🔔 <__NSGlobalBlock__: 0x100001038>
```
```
void exampleA() {
    // ARC 和 MRC 下均为栈区 Block
    char a = 'A';
    ^{
        printf("🔔🔔🔔 %c\n", a);
    }();
}

// MRC: 🔔🔔🔔 A
// ARC: 🔔🔔🔔 A
```
+ B:
```
void exampleB_addBlockToArray(NSMutableArray *array) {
    char b = 'B';
    // 原以为栈区 Block，ARC 下是堆区 Block
    // MRC 下估计是栈区 Block，执行的时候崩溃了
    [array addObject:^{
        printf("🔔🔔🔔 %c\n", b);
    }];
    NSLog(@"🔔🔔🔔 %@", array);
}

void exampleB() {
    NSMutableArray *array = [NSMutableArray array];
    exampleB_addBlockToArray(array);
    NSLog(@"🔔🔔🔔 %@", [array objectAtIndex:0]);
    
    void(^block)() = [array objectAtIndex:0];
    
    NSLog(@"🔔🔔🔔 %@", block);
    block();
}

// ARC: 🔔🔔🔔 ( "<__NSMallocBlock__: 0x102840050>")
        🔔🔔🔔 <__NSMallocBlock__: 0x100611690>
        🔔🔔🔔 <__NSMallocBlock__: 0x100611690>
        🔔🔔🔔 B
// MRC: 崩溃 ， 在 addObject 添加 block 时调用 copy 函数，能正常运行。
```
+ C:
```
void exampleC_addBlockToArray(NSMutableArray *array) {
  // 全局 Global 
  [array addObject:^{
    printf("🔔🔔🔔 C\n");
  }];
}

void exampleC() {
    NSMutableArray *array = [NSMutableArray array];
    exampleC_addBlockToArray(array);
    NSLog(@"🔔🔔🔔 %@", [array objectAtIndex:0]);
    void(^block)() = [array objectAtIndex:0];
    NSLog(@"🔔🔔🔔 %@", block);
    block();
}
// ARC: 🔔🔔🔔 <__NSGlobalBlock__: 0x100002068>
        🔔🔔🔔 <__NSGlobalBlock__: 0x100002068>
        🔔🔔🔔 C
// MRC: 🔔🔔🔔 <__NSGlobalBlock__: 0x100001078>
        🔔🔔🔔 <__NSGlobalBlock__: 0x100001078>
        🔔🔔🔔 C
```
+ D:
```
typedef void(^dBlock)();
dBlock exampleD_getBlock() {
    // ARC 栈区 block 作为函数返回值时会自动调用 copy
    // MAR 下编译直接报错: Returning block that lives on the local stack
    // 主动 block 尾部调 copy 可解决
    char d = 'D';
    return^{
        printf("🔔🔔🔔 %c\n", d);
    };
}

void exampleD() {
    NSLog(@"🔔🔔🔔 %@", exampleD_getBlock());
    exampleD_getBlock()();
}
// ARC: 🔔🔔🔔 <__NSMallocBlock__: 0x100500d00>
        🔔🔔🔔 D
```
+ E:
```
typedef void(^eBlock)();
eBlock exampleE_getBlock() {
    char e = 'E';
    void(^block)() = ^{
        printf("🔔🔔🔔 %c\n", e);
    };
    NSLog(@"🔔🔔🔔 %@", block);
    return block;
}

void exampleE() {
    NSLog(@"🔔🔔🔔 %@", exampleE_getBlock());
    eBlock block = exampleE_getBlock();
    NSLog(@"🔔🔔🔔 %@", block);
    block();
}
// MRC 下即使是栈区 Block 也正常执行了，且两次调用函数返回的是一样的地址
// MRC: 🔔🔔🔔 <__NSStackBlock__: 0x7ffeefbff508>
        🔔🔔🔔 <__NSStackBlock__: 0x7ffeefbff508>
        
        🔔🔔🔔 <__NSStackBlock__: 0x7ffeefbff508>
        🔔🔔🔔 <__NSStackBlock__: 0x7ffeefbff508>
        🔔🔔🔔 P
        
        // 两次地址不同
// ARC: 🔔🔔🔔 <__NSMallocBlock__: 0x100550d10>
        🔔🔔🔔 <__NSMallocBlock__: 0x100550d10>
        
        🔔🔔🔔 <__NSMallocBlock__: 0x100602d00>
        🔔🔔🔔 <__NSMallocBlock__: 0x100602d00>
        🔔🔔🔔 E
```
