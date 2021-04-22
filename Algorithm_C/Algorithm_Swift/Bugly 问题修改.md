#  Bugly 问题修改

1. **更新视频观看信息记录:** 在 bugly 上发现数据库操作崩溃比较多，因为本身只有一个更新任务，完全没必要在 Transaction 里面做，这里把它抽了出来，并放在 try catch 里面捕获异常。有时还发现退到桌面再返回 APP  会发现数据库被关闭，这里加了一个 open 判断。致于要不要加锁和能不能放到子线程去做因为业务牵涉太多，暂时还不确定，还没有加。  
```c++
// 1. 更新视频观看信息记录
// 🌈🌈🌈 这里加了数据库同步队列操作
- (void)updateVideoLookInfoRecord:(VideoLookInfo *)videoLookInfo {
    NSLog(@"🌈🌈🌈 %@", NSStringFromSelector(_cmd));
    
//    NSString * sql = @"UPDATE videolook set uploadLookTime = ?, speed = ?, videoPlayeProgress = ? WHERE courseId = ? AND userId = ?";
//    [database beginTransaction];
//    BOOL isRollBack = NO;
//    @try {
//        BOOL result = [database executeUpdate:sql, [NSNumber numberWithDouble:videoLookInfo.uploadLookTime], [NSNumber numberWithDouble:videoLookInfo.speed], [NSNumber numberWithDouble:videoLookInfo.videoPlayeProgress], [NSNumber numberWithInteger:videoLookInfo.courseId], videoLookInfo.userId];
//
//        if(!result) {
//            NSLog(@"updateIntoDB 更新失败");
//        }else {
//            NSLog(@"更新视频观看时长: %f,倍速：%f,当前播放进度：%f,视频类型:%ld,视频id:%ld,用户ID:%@", videoLookInfo.uploadLookTime, videoLookInfo.speed, videoLookInfo.videoPlayeProgress,videoLookInfo.type,videoLookInfo.courseId,videoLookInfo.userId);
//        }
//    } @catch (NSException *exception) {
//        isRollBack = YES;
//        [database rollback];
//    } @finally {
//        if(!isRollBack) {
//            [database commit];
//        }
//    }
    
    if (![database isOpen]) {
        [database open];
    }
    
    if (![database isOpen]) {
        NSLog(@"🌈🌈🌈 %@ db open 失败", NSStringFromSelector(_cmd));
        return;
    }
    
    NSString * sql = @"UPDATE videolook set uploadLookTime = ?, speed = ?, videoPlayeProgress = ? WHERE courseId = ? AND userId = ?";
    @try {
        BOOL result = [database executeUpdate:sql, [NSNumber numberWithDouble:videoLookInfo.uploadLookTime], [NSNumber numberWithDouble:videoLookInfo.speed], [NSNumber numberWithDouble:videoLookInfo.videoPlayeProgress], [NSNumber numberWithInteger:videoLookInfo.courseId], videoLookInfo.userId];
        
        if (!result) {
            NSLog(@"🌈🌈🌈 %@ updateIntoDB 更新失败", NSStringFromSelector(_cmd));
        } else {
            NSLog(@"🌈🌈🌈 %@ 更新视频观看时长: %f,倍速：%f,当前播放进度：%f,视频类型:%ld,视频id:%ld,用户ID:%@", NSStringFromSelector(_cmd), videoLookInfo.uploadLookTime, videoLookInfo.speed, videoLookInfo.videoPlayeProgress,videoLookInfo.type,videoLookInfo.courseId,videoLookInfo.userId);
        }
    } @catch (NSException *exception) {
        NSLog(@"🌈🌈🌈：%@ %@, %@", NSStringFromSelector(_cmd), [exception name], [exception reason]);
    } @finally {
        //
    }
}
```
2. **更新视频当前下载状态:** 处理方式基本同上。
```c++
// 2. 更新视频当前下载状态
// 🌈🌈🌈 这里先不加数据库同步队列操作
-(void)updateVideoStatus:(NSString*)sql params:(NSArray*)argArr
{
    NSLog(@"🌈🌈🌈 %@", NSStringFromSelector(_cmd));
    
//    [database beginTransaction];
//    BOOL isRollBack = NO;
//    @try {
//        BOOL a = [database executeUpdate:sql withArgumentsInArray:argArr];
//        if (!a) {
//            //NSLog(@"Goods 删除失败");
//        }
//    }
//    @catch (NSException *exception) {
//        isRollBack = YES;
//        [database rollback];
//    }
//    @finally {
//        if (!isRollBack) {
//            [database commit];
//        }
//    }
    
    if (![database isOpen]) {
        [database open];
    }
    
    if (![database isOpen]) {
        NSLog(@"🌈🌈🌈 %@ db open 失败", NSStringFromSelector(_cmd));
        return;
    }
    
    @try {
        BOOL a = [database executeUpdate:sql withArgumentsInArray:argArr];
        if (!a) NSLog(@"🌈🌈🌈 %@ 执行失败", NSStringFromSelector(_cmd));
    } @catch (NSException *exception) {
        NSLog(@"🌈🌈🌈：%@ %@, %@", NSStringFromSelector(_cmd), [exception name], [exception reason]);
    } @finally {
        //
    }
}
```
3. **根据视频ID获取视频观看信息:** 崩溃特多，且多次调用来自 RN ，这里做了加锁处理。
```c++
// 3. 根据视频ID获取视频观看信息
// 🌈🌈🌈 这里加了数据库同步队列操作
- (VideoLookInfo *)getVideoLookInfoFromVideoId:(NSInteger) videoId UserId:(NSString *)userId {
    NSLog(@"🌈🌈🌈 %@", NSStringFromSelector(_cmd));
    
    if (userId == nil || userId.length <= 0) return nil;
    
    if (![database isOpen]) {
        [database open];
    }
    
    if (![database isOpen]) {
        NSLog(@"🌈🌈🌈 %@ db open 失败", NSStringFromSelector(_cmd));
        return nil;
    }
    
    VideoLookInfo * videoLookInfo;
    NSString * sql = [NSString stringWithFormat:@"SELECT * FROM videolook WHERE userId = '%@' AND courseId = '%ld'", userId, videoId];
    
    [self.lock lock];
    FMResultSet * set = [database executeQuery:sql];
    while ([set next]) {
        videoLookInfo = [[VideoLookInfo alloc] init];
        videoLookInfo.userId = [set stringForColumn:@"userId"];
        videoLookInfo.courseId = [set intForColumn:@"courseId"];
        videoLookInfo.uploadLookTime = [set doubleForColumn:@"uploadLookTime"];
        videoLookInfo.type = [set intForColumn:@"type"];
        videoLookInfo.speed = [set doubleForColumn:@"speed"];
        videoLookInfo.videoPlayeProgress = [set doubleForColumn:@"videoPlayeProgress"];
        break;
    }
    
    if(videoLookInfo == nil) {
        [set close];
    }
    [self.lock unlock];
    
    return videoLookInfo;
}
```

4. **更新下载信息:** 处理方式大概同 1 2。
```c++
// 4. 更新下载信息
// 🌈🌈🌈 这里先不加数据库同步队列操作
-(void)updateDownloadInfo:(NSString *)sql params:(NSArray *)argArr{
    NSLog(@"🌈🌈🌈 %@", NSStringFromSelector(_cmd));
    
//    [database beginTransaction];
//    BOOL isRollBack = NO;
//    @try {
//        BOOL a = [database executeUpdate:sql withArgumentsInArray:argArr];
//        if (!a) {
//            //NSLog(@"Goods 删除失败");
//        }
//    }
//    @catch (NSException *exception) {
//        isRollBack = YES;
//        [database rollback];
//    }
//    @finally {
//        if (!isRollBack) {
//            [database commit];
//        }
//    }
    
    if (![database isOpen]) {
        [database open];
    }
    
    if (![database isOpen]) {
        NSLog(@"🌈🌈🌈 %@ db open 失败", NSStringFromSelector(_cmd));
        return;
    }
    
    @try {
        BOOL a = [database executeUpdate:sql withArgumentsInArray:argArr];
        if (!a) NSLog(@"🌈🌈🌈 %@ 执行失败", NSStringFromSelector(_cmd));
    } @catch (NSException *exception) {
        NSLog(@"🌈🌈🌈：%@ %@, %@", NSStringFromSelector(_cmd), [exception name], [exception reason]);
    } @finally {
        //
    }
}

```

5. `swRemoveObjectsInRange` 按 range 移除数组中内容被收集到多次，这里使用方法交换，防止 range 的 location 和 length 越界。
```c++
- (void)swRemoveObjectsInRange:(NSRange)range {
    if (range.location < 0 || range.location > self.count) {
        // 首先保证 range 的 location 的位置在数组下标内(下标可以到长度的最后一个)
        NSLog(@"******* 🌈🌈🌈 range 的 location 越界 %s *******",__func__);
        NSLog(@"%@",self);
        
        return;
    }
    
    // 可以超过最后一个标，到长度的边界
    if ((range.location + range.length) > self.count) {
        NSLog(@"******* 🌈🌈🌈 range 的 location 和 length 的和超过数组长度 %s *******",__func__);
        NSLog(@"%@",self);
        
        return;
    }
    
    [self swRemoveObjectsInRange:range];
}
```
6. `swInsertObject` 数组插入导致崩溃。
```c++
-(void)swInsertObject:(id)anObject atIndex:(NSUInteger)index{
    if(anObject == nil){
        NSLog(@"******* 🌈🌈🌈 数组元素为nil %s *******",__func__);
        NSLog(@"%@",self);
        
        return;
    }
    
    if(self.count < index){
        NSLog(@"******* 🌈🌈🌈 数组越界 index:%ld %s",index,__func__);
        NSLog(@"%@",self);
        
        return;
    }
    
    [self swInsertObject:anObject atIndex:index];
    
}
```
7. 字典 key  和 value 为 nil 导致崩溃。
```c++
-(void)swSetObject:(id)anObject forKey:(id<NSCopying>)aKey{
    if(anObject == nil){
        NSLog(@"🌈🌈🌈 *******字典 Value 为 nil %@:%@,%s ********",anObject,aKey,__func__);
        NSLog(@"%@",self);
        
        return;
    }
    
    // 🌈🌈🌈 这里 key 值为 nil 导致了很多次 crash
    if (aKey == nil) {
        return;
    }
    
    [self swSetObject:anObject forKey:aKey];
}

-(void)swSetValue:(id)value forKey:(NSString *)key{
    if(value == nil){
        NSLog(@"🌈🌈🌈 ******* 字典 Value 为 nil %@:%@,%s ********",value,key,__func__);
        NSLog(@"🌈🌈🌈 %@",self);
        
        return;
    }
    
    // 🌈🌈🌈 这里 key 值为 nil 导致了很多次 crash
    if (key == nil) {
        NSLog(@"🌈🌈🌈 ******* 字典 Key 为 nil %@:%@,%s ********",value,key,__func__);
        NSLog(@"🌈🌈🌈 %@",self);
        
        return;
    }
    
    [self swSetValue:value forKey:key];
}
```
8. 下载失败回调导致的崩溃。
```c++
[ZBDownloadManager shareInstance].failBlock = ^(ZBDownloadModel * _Nullable model) {
    if(![model.courseId isEqualToString:self.courseId]){
        return;
    }
    NSString * key;
    UITableView * tableView;
    if(model.type.intValue == 5){
        key = [NSString stringWithFormat:@"ware%@", model.lessonId];
        tableView = (UITableView *)[self.scrollView viewWithTag:201];
    }
    
    // 🌈🌈🌈 这里 key 值为 nil 导致了很多次 crash
    if (key == nil) return;
    
    [weakSelf.downloadDic setObject:model forKey:key];
    [tableView reloadData];
    
    if(weakSelf.bottomView.number > 0){
        weakSelf.bottomView.number -= 1;
    }
};
```
9. `praseUrl` 导致的崩溃，这里尽可能加了所有异常判断。
```c++
-(void)praseUrl:(NSString *)urlStr uuid:(NSString*)uuid{
    if (urlStr == nil || urlStr.length <= 0 || uuid == nil || uuid.length <= 0) {
        if (self.delegate != nil && [self.delegate respondsToSelector:@selector(praseM3U8Failed:)]) {
            [self.delegate praseM3U8Failed:self];
        }
        
        return;
    }
    
    // 判断是否是HTTP连接
    if (!([urlStr hasPrefix:@"http://"] || [urlStr hasPrefix:@"https://"])) {
        if (self.delegate != nil && [self.delegate respondsToSelector:@selector(praseM3U8Failed:)]) {
            [self.delegate praseM3U8Failed:self];
        }
        
        return;
    }
    
    //解析出视频Id
    NSArray *separatedArray = [urlStr componentsSeparatedByString:@"/"];
    
    // 这里添加一个长度判断，长度大于 3 以后才能取下标 3
    if (separatedArray.count > 3) {
        self.playList.lessonId = separatedArray[3];
    }
    
    //解析出M3U8
    NSError *error = nil;
    NSStringEncoding encoding;
    NSString *m3u8Str = [[NSString alloc] initWithContentsOfURL:[NSURL URLWithString:urlStr] usedEncoding:&encoding error:&error];// 注意这一步是耗时操作，要在子线程中进行
    self.oriM3U8Str = m3u8Str;
    if (m3u8Str == nil || m3u8Str.length <= 0) {
        if (self.delegate != nil && [self.delegate respondsToSelector:@selector(praseM3U8Failed:)]) {
            [self.delegate praseM3U8Failed:self];
        }
        
        return;
    }
    
    // 解析TS文件
    NSRange segmentRange = [m3u8Str rangeOfString:@"#EXTINF:"];
    if (segmentRange.location == NSNotFound) {
        //M3U8里没有TS文件
        if (self.delegate != nil && [self.delegate respondsToSelector:@selector(praseM3U8Failed:)]) {
            [self.delegate praseM3U8Failed:self];
        }
        
        return;
    }
    
    if (self.segmentArray.count > 0) {
        // Bugly 上收集到这里会崩溃，暂时加 try catch
        @try {
            [self.segmentArray removeAllObjects];
        } @catch (NSException *exception) {
            NSLog(@"🌈🌈🌈 [self.segmentArray removeAllObjects]: %@, %@", [exception name], [exception reason]);
        } @finally {
            //
        }
    }
    
    // 🌈🌈🌈 这里 Bugly 捕捉到多个 crash，加 try catch
    @try {
        // 逐个解析TS文件，并存储
        while (segmentRange.location != NSNotFound) {
            // 声明一个model存储TS文件链接和时长的model
            M3U8SegmentModel *model = [[M3U8SegmentModel alloc] init];
            // 读取TS片段时长
            NSRange commaRange = [m3u8Str rangeOfString:@","];
            if (commaRange.location == NSNotFound) {
                break;
            }
    //        NSUInteger tempLocation = segmentRange.location + [@"#EXTINF:" length];
    //        NSUInteger tempLength = commaRange.location - (segmentRange.location + [@"#EXTINF:" length]);
            NSString* value = [m3u8Str substringWithRange:NSMakeRange(segmentRange.location + [@"#EXTINF:" length], commaRange.location - (segmentRange.location + [@"#EXTINF:" length]))];
            model.duration = [value integerValue];
            
            // 截取M3U8
            m3u8Str = [m3u8Str substringFromIndex:commaRange.location];
            // 获取TS下载链接,这需要根据具体的M3U8获取链接，可以更具自己公司的需求
            NSRange linkRangeBegin = [m3u8Str rangeOfString:@","];
            NSRange linkRangeEnd = [m3u8Str rangeOfString:@".ts"];
            NSString* linkUrl = [m3u8Str substringWithRange:NSMakeRange(linkRangeBegin.location + 2, (linkRangeEnd.location + 3) - (linkRangeBegin.location + 2))];
            model.locationUrl = linkUrl;
            [self.segmentArray addObject:model];
            
            m3u8Str = [m3u8Str substringFromIndex:(linkRangeEnd.location + 3)];
            segmentRange = [m3u8Str rangeOfString:@"#EXTINF:"];
        }
    } @catch (NSException *exception) {
        NSLog(@"🌈🌈🌈：%@, %@", [exception name], [exception reason]);
        
        if (self.delegate != nil && [self.delegate respondsToSelector:@selector(praseM3U8Failed:)]) {
            [self.delegate praseM3U8Failed:self];
        }
        
        // 如果抛错了就直接 return
        return;
    } @finally {
        NSLog(@"🌈🌈🌈：finally");
    }

    // 已经获取了所有TS片段，继续打包数据
    [self.playList initWithSegmentArray:self.segmentArray];
    [dataSingle updateVideoStatus:@"update zbdownload set splitLength=? where sourceId=?" params:[NSArray arrayWithObjects:[NSString stringWithFormat:@"%ld",self.segmentArray.count], uuid, nil]];
    
    self.playList.uuid = uuid;
    
    // 到此数据TS解析成功，通过代理发送成功消息
    if (self.delegate != nil && [self.delegate respondsToSelector:@selector(praseM3U8Finished:)]) {
        [self.delegate praseM3U8Finished:self];
    }
}

```
