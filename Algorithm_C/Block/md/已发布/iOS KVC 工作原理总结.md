# iOS KVC 工作原理总结

> &emsp;

## About Key-Value Coding
&emsp;Key-value coding（键值编码）是由 NSKeyValueCoding 非正式协议启用的一种机制，对象采用这种机制来提供对其属性的间接访问。当一个对象符合键值编码时，它的属性可以通过一个简洁、统一的消息传递接口（setValue:forKey:）通过字符串参数寻址。这种间接访问机制补充了实例变量（自动生成的 _属性名 ）及其相关访问器方法（Getter 方法）提供的直接访问。




## 参考链接
**参考链接:🔗**
+ [Key-Value Coding Programming Guide](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/KeyValueCoding/index.html#//apple_ref/doc/uid/10000107i)
