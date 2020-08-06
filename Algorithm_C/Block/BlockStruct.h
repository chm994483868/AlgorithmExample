//
//  BlockStruct.h
//  Block
//
//  Created by CHM on 2020/8/6.
//  Copyright © 2020 CHM. All rights reserved.
//

#ifndef BlockStruct_h
#define BlockStruct_h

#include <stdio.h>

// 这个结构体是复用的
struct __block_impl {
    void* isa;
    int Flags;
    int Reserved;
    void* FuncPtr;
};

void *_NSConcreteStackBlock[32];

struct __main_block_impl_0 {
    struct __block_impl impl;
    struct __main_block_desc_0* Desc;
    
    __main_block_impl_0(void* fp, struct __main_block_desc_0* desc = nullptr, int flags = 0) {
        impl.isa = &_NSConcreteStackBlock;
        impl.Flags = flags;
        impl.FuncPtr = fp;
        Desc = desc;
    }
};

static void __main_block_func_0(struct __main_block_impl_0* __cself) {
    printf("Block 内部打印 \n");
}

static struct __main_block_desc_0 {
    size_t reserved;
    size_t Block_size;
} __main_block_desc_0_DATA = { 0, sizeof(struct __main_block_impl_0)};

void invoke() {
    
}

#endif /* BlockStruct_h */
