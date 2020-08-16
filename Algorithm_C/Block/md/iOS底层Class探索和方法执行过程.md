#  iOS底层Class探索和方法执行过程
## 1、类（class）结构
在源码中查看类信息:
⚠️⚠️⚠️ 错误追踪: `usr/include/objc/runtime.h` 中的定义是错误的
而且已经过时，且已被标注为不可见 `OBJC2_UNAVAILABLE`:
```
struct objc_class {
    Class _Nonnull isa  OBJC_ISA_AVAILABILITY;

#if !__OBJC2__
    Class _Nullable super_class                              OBJC2_UNAVAILABLE;
    const char * _Nonnull name                               OBJC2_UNAVAILABLE;
    long version                                             OBJC2_UNAVAILABLE;
    long info                                                OBJC2_UNAVAILABLE;
    long instance_size                                       OBJC2_UNAVAILABLE;
    struct objc_ivar_list * _Nullable ivars                  OBJC2_UNAVAILABLE;
    struct objc_method_list * _Nullable * _Nullable methodLists                    OBJC2_UNAVAILABLE;
    struct objc_cache * _Nonnull cache                       OBJC2_UNAVAILABLE;
    struct objc_protocol_list * _Nullable protocols          OBJC2_UNAVAILABLE;
#endif

} OBJC2_UNAVAILABLE;
/* Use `Class` instead of `struct objc_class *` */
```
✅✅✅  正确追踪：在 `objc4` 开源库中，`Project Headers/objc-runtime-new.h` 才是对的，正确是继承自 `objc_object` 的结构体，如下:
```
struct objc_class : objc_object {
// Class ISA; // 继承自 objc_object isa_t isa;
Class superclass; // 8 个字节
cache_t cache;             // formerly cache pointer and vtable // 16 个字节
class_data_bits_t bits;    // class_rw_t * plus custom rr/alloc flags

class_rw_t *data() const { // 非常重要的数据
    return bits.data();
}
void setData(class_rw_t *newData) {
    bits.setData(newData);
}

void setInfo(uint32_t set) {
    ASSERT(isFuture()  ||  isRealized());
    data()->setFlags(set);
}

void clearInfo(uint32_t clear) {
    ASSERT(isFuture()  ||  isRealized());
    data()->clearFlags(clear);
}

// set and clear must not overlap
void changeInfo(uint32_t set, uint32_t clear) {
    ASSERT(isFuture()  ||  isRealized());
    ASSERT((set & clear) == 0);
    data()->changeFlags(set, clear);
}

....

};

```
类对象继承 `objc_object`:
```
struct objc_object {
private:
    isa_t isa;

public:

    // ISA() assumes this is NOT a tagged pointer object
    Class ISA();
    
    ...
};
```
再看下 `objc_class` 中非常重要的数据信息：
```
class_rw_t *data() const {
    return bits.data();
}
```
```
struct class_rw_t {
    // Be warned that Symbolication knows the layout of this structure.
    uint32_t flags;
    uint16_t witness;
#if SUPPORT_INDEXED_ISA
    uint16_t index;
#endif

    explicit_atomic<uintptr_t> ro_or_rw_ext;

    Class firstSubclass;
    Class nextSiblingClass;
    
    ...
    
    const class_ro_t *ro() const {
        auto v = get_ro_or_rwe();
        if (slowpath(v.is<class_rw_ext_t *>())) {
            return v.get<class_rw_ext_t *>()->ro;
        }
        return v.get<const class_ro_t *>();
    }
    
    const method_array_t methods() const {
        auto v = get_ro_or_rwe();
        if (v.is<class_rw_ext_t *>()) {
            return v.get<class_rw_ext_t *>()->methods;
        } else {
            return method_array_t{v.get<const class_ro_t *>()->baseMethods()};
        }
    }
    
    const property_array_t properties() const {
        auto v = get_ro_or_rwe();
        if (v.is<class_rw_ext_t *>()) {
            return v.get<class_rw_ext_t *>()->properties;
        } else {
            return property_array_t{v.get<const class_ro_t *>()->baseProperties};
        }
    }

    const protocol_array_t protocols() const {
        auto v = get_ro_or_rwe();
        if (v.is<class_rw_ext_t *>()) {
            return v.get<class_rw_ext_t *>()->protocols;
        } else {
            return protocol_array_t{v.get<const class_ro_t *>()->baseProtocols};
        }
    }
```
问题1:为什么实例对象的方法要存在类对象中？
想象一下，如果每生成一个实例都会将所有的方法实现拷贝过去，那将会占用很大的内存，所以类生成实例的时候将实例的isa指向自己，调用函数时在isa指向的类中去执行该调用哪个方法的逻辑。

## 2、属性、变量、方法存储在哪里？
### 1、 结构方式探索（控制台打印）
```
@interface HHStaff : NSObject {
    NSString *_hhName;
}
@property (nonatomic, copy) NSString *_hhNick;
@end

// 在 main.m 中初始化这个类
Class cls = NSClassFromString(@"HHStaff");
```
通过 `x` 指令打印 cls 信息，一步一步找到最后的信息：
> 找成员/属性位置

> review
> 从实例对象的的前 8 字节内存中读出类对象：
>> ``` 
>> // 示例代码
>> LGPerson *objc2 = [[LGPerson alloc] init];
>> 
>> Class cls = NSClassFromString(@"LGPerson");
>> NSLog(@"👗👗👗 %p %@ %p", cls, cls, &cls);
>> NSLog(@"Hello, World! %@ - %@",objc1,objc2);
>> // 控制台打印:
>> 👗👗👗 0x1000021d0 LGPerson 0x7ffeefbff570
>> (lldb) p objc2 // 读出 objc2 指针指向的地址
>> (LGPerson *) $2 = 0x0000000101024cc0
>> (lldb) x $2 // 读取该地址里的内容，即当前系统为 LGPerson 对象分配的内存里面保存的数据
>> 0x101024cc0: d5 21 00 00 01 80 1d 00 00 00 00 00 00 00 00 00  .!..............
>> 0x101024cd0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
>> (lldb) p/x 0x001d8001000021d5 & 0x00007ffffffffff8 // 前 8 个字节的内容与 ISA_MASK 做 & 操作
>> (long) $3 = 0x00000001000021d0 // 看到与 👗👗👗 0x1000021d0 打印的 LGPerson 类对象地址完全一样
>> ```

**继续我们上面的 找成员/属性位置**
```
(lldb) x/5gx cls
0x1000021d0: 0x00000001000021a8 (isa) 0x00000001003ee140 (superclass)
0x1000021e0: 0x00000001012180b0 0x0001802400000003 (这 16 位是 cache_t)
0x1000021f0: 0x0000000101217a34 (bits)

// 拿到 bits 地址执行 p 命令
(lldb) p 0x1000021f0
(long) $1 = 4294975984

// 通过强制转换再执行 p 命令
(lldb) p (class_data_bits_t *)0x1000021f0
(class_data_bits_t *) $2 = 0x00000001000021f0

// 想要得到 data() 里面的信息，通过 bits.data() 方法
(lldb) p $2->data()
(class_rw_t *) $3 = 0x0000000101217a30 // (class_rw_t *)

// 查看 (class_rw_t *) $3 里面有哪些信息
(lldb) p *$3

(class_rw_t) $4 = {
  flags = 2148007936
  version = 0
  witness = 1
  ro = 0x0000000100002088
  methods = {
    list_array_tt<method_t, method_list_t> = {
       = {
        list = 0x00000001000020d0
        arrayAndFlag = 4294975696
      }
    }
  }
  properties = {
    list_array_tt<property_t, property_list_t> = {
       = {
        list = 0x0000000100002168
        arrayAndFlag = 4294975848
      }
    }
  }
  protocols = {
    list_array_tt<unsigned long, protocol_list_t> = {
       = {
        list = 0x0000000000000000
        arrayAndFlag = 0
      }
    }
  }
  firstSubclass = nil
  nextSiblingClass = NSUUID
  demangledName = 0x0000000100000f76 "LGPerson"
}
// 查看 ro 里面的信息
(lldb) p $3->ro
(const class_ro_t *) $5 = 0x0000000100002088

// 看看 $5 里面是什么内容
(lldb) p *$5
(const class_ro_t) $6 = {
  flags = 388
  instanceStart = 8
  instanceSize = 24
  reserved = 0
  ivarLayout = 0x0000000100000f7f "\x02"
  name = 0x0000000100000f76 "LGPerson"
  baseMethodList = 0x00000001000020d0
  baseProtocols = 0x0000000000000000
  ivars = 0x0000000100002120
  weakIvarLayout = 0x0000000000000000
  baseProperties = 0x0000000100002168
  _swiftMetadataInitializer_NEVER_USE = {}
}
// 查看对象的信息
(lldb) p $6.ivars
(const ivar_list_t *const) $7 = 0x0000000100002120
// 查看 $7 的信息
(const ivar_list_t) $8 = {
  entsize_list_tt<ivar_t, ivar_list_t, 0> = {
    entsizeAndFlags = 32
    count = 2 // 成员变量 2 个 // 另外一个应该是 isa，但是这里没打印出来
    first = {
      offset = 0x0000000100002198
      name = 0x0000000100000ee5 "_hhName" // 我们定义时添加的成员变量
      type = 0x0000000100000f81 "@\"NSString\""
      alignment_raw = 3
      size = 8
    }
  }
}
// 有了 $6 之后 可以 p 出属性
(lldb) p $6.baseProperties
(property_list_t *const) $9 = 0x0000000100002168

// 查看 $9 信息
(lldb) p *$9
(property_list_t) $10 = {
  entsize_list_tt<property_t, property_list_t, 0> = {
    entsizeAndFlags = 16
    count = 1 // 属性数量
    first = (name = "hhNick", attributes = "T@\"NSString\",C,N,V_hhNick")
  }
}
```
这里看到了成员变量的信息 count  = 2 第一个成员变量是 first = { _hhName }
属性的信息 count = 1 第一个成员变量是 first = { hhNick }

> 找方法存储位置
```
(lldb) p $5->baseMethodList
(method_list_t *const) $11 = 0x00000001000020d0
(lldb) p *$11
(method_list_t) $12 = {
  entsize_list_tt<method_t, method_list_t, 3> = {
    entsizeAndFlags = 26
    count = 3
    first = {
      name = "hhNick"
      types = 0x0000000100000f95 "@16@0:8"
      imp = 0x0000000100000df0 (KCObjc`-[LGPerson hhNick])
    }
  }
}
// 找到其他的方法
(lldb) p $12.get(1)
(method_t) $13 = {
  name = "setHhNick:"
  types = 0x0000000100000f9d "v24@0:8@16"
  imp = 0x0000000100000e20 (KCObjc`-[LGPerson setHhNick:])
}
(lldb) p $12.get(2)
(method_t) $14 = {
  name = ".cxx_destruct"
  types = 0x0000000100000f8d "v16@0:8"
  imp = 0x0000000100000db0 (KCObjc`-[LGPerson .cxx_destruct])
}
```
### 代码打印:
```
// 打印成员变量/属性
void testObjc_copyIvar_copyProperies(Class pClass) {
    unsigned int count = 0;
    Ivar *ivars = class_copyIvarList(pClass, &count);
    for (unsigned int i = 0; i < count; ++i) {
        Ivar const ivar = ivars[i];
        // 获取实例变量名
        const char* cName = ivar_getName(ivar);
        NSString *ivarName = [NSString stringWithUTF8String:cName];
        NSLog(@"class_copyIvarList:%@", ivarName);
    }
    free(ivars);
    
    unsigned int pCount = 0;
    objc_property_t *properties = class_copyPropertyList(pClass, &pCount);
    for (unsigned int i = 0; i < pCount; ++i) {
        objc_property_t const property = properties[i];
        // 获取属性名
        NSString *propertyName = [NSString stringWithUTF8String:property_getName(property)];
        // 获取属性值
        NSLog(@"class_copyProperiesList: %@", propertyName);
    }
    free(properties);
}

// 打印方法列表
// 如果传入类 就是打印 实例方法/静态方法
// 如果传入元类 就打印的是类方法
void testObjc_copyMethodList(Class pClass) {
    unsigned int count = 0;
    Method *methods = class_copyMethodList(pClass, &count);
    for (unsigned int i = 0; i < count; ++i) {
        Method const method = methods[i];
        // 获取方法名
        NSString *key = NSStringFromSelector(method_getName(method));
        NSLog(@"Method, name: %@", key);
    }
    free(methods);
}

void testInstanceMethod_classToMetaclass(Class pClass) {
    const char* className = class_getName(pClass);
    Class metaClass = objc_getMetaClass(className);
    
    Method method1 = class_getInstanceMethod(pClass, @selector(sayHello));
    Method method2 = class_getInstanceMethod(metaClass, @selector(sayHello));
    
    Method method3 = class_getInstanceMethod(pClass, @selector(sayHappy));
    Method method4 = class_getInstanceMethod(metaClass, @selector(sayHappy));
    
    NSLog(@"%p - %p - %p - %p", method1, method2, method3, method4);
    NSLog(@"%s", __func__);
}

void testClassMethod_classToMetaclass(Class pClass) {
    const char* className = class_getName(pClass);
    Class metaClass = objc_getMetaClass(className);
    
    Method method1 = class_getClassMethod(pClass, @selector(sayHello));
    Method method2 = class_getClassMethod(metaClass, @selector(sayHello));
    
    Method method3 = class_getClassMethod(pClass, @selector(sayHappy));
    Method method4 = class_getClassMethod(metaClass, @selector(sayHappy));
    
    NSLog(@"%p - %p - %p - %p", method1, method2, method3, method4);
    NSLog(@"%s", __func__);
}

void testIMP_classToMetaclass(Class pClass) {
    const char* className = class_getName(pClass);
    Class metaClass = objc_getMetaClass(className);
    
    IMP imp1 = class_getMethodImplementation(pClass, @selector(sayHello));
    IMP imp2 = class_getMethodImplementation(metaClass, @selector(sayHello));
    
    IMP imp3 = class_getMethodImplementation(pClass, @selector(sayHappy));
    IMP imp4 = class_getMethodImplementation(metaClass, @selector(sayHappy));
    
    NSLog(@"%p - %p - %p - %p", imp1, imp2, imp3, imp4);
    NSLog(@"%s", __func__);
}
```
## 方法
在调试的过程中，最后打印出来的方法包含三个信息 `name`、`types`、`imp`，查看 objc_method 源码：
```
struct method_t {
    SEL name;
    const char *types;
    MethodListIMP imp; // using MethodListIMP = IMP;

    struct SortBySELAddress :
        public std::binary_function<const method_t&,
                                    const method_t&, bool>
    {
        bool operator() (const method_t& lhs,
                         const method_t& rhs)
        { return lhs.name < rhs.name; }
    };
};
```
分别代表: 方法名、方法类型（方法编码）、方法实现
方法名很简单，这里我们来研究方法类型（方法编码）究竟是怎样的含义
定义一个类 `Student`  并实现几个方法（在 .m 文件中实现，.h 中可以不定义）:
```
@implementation Student

- (NSString *)methodOne:(int)a str:(NSString *)str {
    return @"";
}

- (NSArray *)methodTwo:(NSArray *)a str:(NSString *)str count:(NSInteger)count {
    return [NSArray new];
}

- (void)redBook {
    
}

+ (NSInteger)methodForClass:(NSInteger)a time:(long)time {
    return 1;
}

- (void)methodInfo:(id)obj {
    unsigned int methodCount = 0;
    Method *methodList = class_copyMethodList([obj class], &methodCount);
    
    for (NSInteger i = 0; i < methodCount; ++i) {
        Method method = methodList[i];
        SEL methodName = method_getName(method);
        
        NSLog(@"✳️✳️✳️ 方法名：%@", NSStringFromSelector(methodName));
        
        // 获取方法的参数类型
        unsigned int argumentsCount = method_getNumberOfArguments(method);
        char argName[512] = {};
        for (unsigned int j = 0; j < argumentsCount; ++j) {
            method_getArgumentType(method, j, argName, 512);
            
            NSLog(@"第 %u 个参数类型为: %s", j, argName);
        }
        
        char returnType[512] = {};
        method_getReturnType(method, returnType, 512);
        NSLog(@"💠💠💠 返回值类型: %s", returnType);
        
        // type encoding
        NSLog(@"Ⓜ️Ⓜ️Ⓜ️ TypeEncoding: %s", method_getTypeEncoding(method));
    }
}

// 打印:
✳️✳️✳️ 方法名：methodInfo:
第 0 个参数类型为: @
第 1 个参数类型为: :
第 2 个参数类型为: @
💠💠💠 返回值类型: v
Ⓜ️Ⓜ️Ⓜ️ TypeEncoding: v24@0:8@16
✳️✳️✳️ 方法名：methodOne:str:
第 0 个参数类型为: @
第 1 个参数类型为: :
第 2 个参数类型为: i
第 3 个参数类型为: @
💠💠💠 返回值类型: @
Ⓜ️Ⓜ️Ⓜ️ TypeEncoding: @28@0:8i16@20
✳️✳️✳️ 方法名：methodTwo:str:count:
第 0 个参数类型为: @
第 1 个参数类型为: :
第 2 个参数类型为: @
第 3 个参数类型为: @
第 4 个参数类型为: q
💠💠💠 返回值类型: @
Ⓜ️Ⓜ️Ⓜ️ TypeEncoding: @40@0:8@16@24q32
✳️✳️✳️ 方法名：redBook
第 0 个参数类型为: @
第 1 个参数类型为: :
💠💠💠 返回值类型: v
Ⓜ️Ⓜ️Ⓜ️ TypeEncoding: v16@0:8
```
