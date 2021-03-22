# KYY 知识点记录

## RN 与 Native 交互
1. RN 调用 OC
&emsp;遵循 RCTBridgeModule 协议，`RCT_EXPORT_METHOD(methodName:(NSInteger)param1 count:(NSInteger)param2 resolver:(RCTPromiseResolveBlock)resolver rejecter:(RCTPromiseRejectBlock)reject) { ... }` 形式调用。

2. OC 调用 RN
&emsp;继承 RCTEventEmitter，然后 `- (NSArray<NSString *> *)supportedEvents{ ... }` 返回可用的方法明，调用时则是 `- (void)sendEventWithName:(NSString *)eventName body:(id)body` 函数，`eventName` 是要调用的 RN 函数名，`body` 是参数。

3. RCTViewManager 和 RCT_EXPORT_VIEW_PROPERTY 的使用
&emsp;这个链接下 RCTViewManager 和 RCT_EXPORT_VIEW_PROPERTY 已经介绍的足够清晰：[React Native之创建iOS视图](https://blog.csdn.net/u014410695/article/details/51133727)。继承 RCTViewManager，然后实现 `- (UIView *)view` 函数，返回自己的自定义 View，然后 RCT_EXPORT_VIEW_PROPERTY(title, NSString *); 这样释放出 RN 可以直接使用的我们的自定义 View 的某些属性。

### 分析 AppDelegate 文件
1. AppDelegate.h 文件中添加了一个 AFNetworkReachabilityStatus 类型的 status 属性用于监听手机网络状态，这样写没必要，只需要在启动时开启 AFN 的网络监听即可，当网络变化时做出提示。
#### 分析 application:didFinishLaunchingWithOptions: 函数
1. initNotify 函数有对本地 plist 文件的操作，可以放在后台线程中执行，加快 App 启动速度。（initNotify 中的内容甚至放在第一个页面显示完毕后再执行都不晚，完全没必要放在 application... 函数中）
2. AVAudioSession 设置支持后台音频播放可以放在后面。
3. 添加了 canLandscape 通知，监听是否支持横屏。
4. bugly 的注册可以根据 DEBUG 的情况来进行注册和非注册。
5. ShanYanConfig SDK 的注册可以提取出来，在涉及到登录之前进行注册。
6. 注册友盟 SDK 使用的代码行数较多，可以抽取出来。
7. jsCodeLocation 变量表示当前运行平台的情况：模拟器、真机、
