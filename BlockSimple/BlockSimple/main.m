//
//  main.m
//  BlockSimple
//
//  Created by HM C on 2020/6/30.
//  Copyright © 2020 HM C. All rights reserved.
//

#import <Foundation/Foundation.h>

#include <stdio.h>

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        NSLog(@"🎉🎉🎉 Hello, World!");
        
        int dmy = 256;
        int val = 10;
        const char* fmt = "val = %d\n";
        
        void (^blk)(void) = ^{ printf(fmt, val); };

        val = 2;
        fmt = "These values were changed. val = %d\n";

        blk();
        
    }
    
    return 0;
}
