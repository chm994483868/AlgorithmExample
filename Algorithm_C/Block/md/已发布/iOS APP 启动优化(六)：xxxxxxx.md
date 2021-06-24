#  

**之所以没有进alloc而是进了objc_alloc，查资料说的是在编译期的时候如果符号绑定失败了就会触发一个这样的修复操作，调用fixupMessageRef方法，明显的能看到 if (msg->sel == SEL_alloc) , msg->imp = (IMP)&objc_alloc，将alloc方法和objc_alloc方法进行交换。**
