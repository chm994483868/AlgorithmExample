#  iOS底层-isa结构(isa_t)
上一节学习了 isa 的指向。那么 isa 的结构具体是什么样的呢？从源码中来着手研究。
## 一、位域
isa 的结构是一个联合体+位域的形式。
示例：
坦克大战，定义方向有上下左右状态。
> 常见写法：添加 4 个变量。
```
@interface HHTank : NSObject

@property (nonatomic, assign) BOOL left;
@property (nonatomic, assign) BOOL right;
@property (nonatomic, assign) BOOL top;
@property (nonatomic, assign) BOOL bottom;

@end
```
> 位域的方式
定义联合体，只需要一个 char 的长度就可以表示 4 个方向
```
@interface HHTank : NSObject {
    @public
    union {
        uintptr_t direction;
        
        struct {
            uintptr_t left : 1;
            uintptr_t right : 1;
            uintptr_t top : 5; //这里定义为5  只是想说  长度可以根据不同的需求去自定义
            uintptr_t bottom : 1;
        };
        
    } _tankDirection;
}

@end
```
这样只需要对 left/right 等进行相应的赋值就可以满足需求.
具体赋值方法：
```
HHTank *tank = [HHTank new];

// 方法1：
tank->_tankDirection.direction = 0x81;
tank->_tankDirection.direction = 0b01111101;

// 方法2：
// tank->_tankDirection.left = YES; // YES 强转之后为 1，
// tank->_tankDirection.top = 31; // 0 ～ 32
// tank->_tankDirection.bottom = 0b1; // 二进制赋值

NSLog(@"🐶🐶🐶 left = %@ top = %@ right = %@ bottom = %@", @(tank->_tankDirection.left), @(tank->_tankDirection.top), @(tank->_tankDirection.right), @(tank->_tankDirection.bottom));

// 打印:
🐶🐶🐶 left = 1 top = 31 right = 0 bottom = 0
```
联合体的优势:
1. 联合体和结构体写法上有些类似，但是注意区分。
2. 联合体的所有信息公用一块内存，起到节省内存的作用。
位域的作用：
直观的表达取值范围，可以直接拿到相应的值。

## 二、isa 结构
可以从源码中找到相关内容:
位置 `Project Headers/objc-private.h/struct objc_object/initIsa`



