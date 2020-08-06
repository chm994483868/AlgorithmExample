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

//struct __block_impl {
//    void* isa;
//    int Flags;
//    int Reserved;
//    void* FuncPtr;
//};
//
//struct __main_block_impl_0 {
//    struct __block_impl impl;
//    struct __main_block_desc_0* Desc;
//
//    __main_block_impl_0(void* fp, struct __main_block_desc_0* desc, int flags = 0) {
////        impl.isa = &_NSConcreteStackBlock;
//        impl.Flags = flags;
//        impl.FuncPtr = fp;
//        Desc = Desc;
//    }
//};
//
//static void __main_block_func_0(struct __main_block_impl_0* __cself) {
//    printf("Block\n");
//}
//
//static struct __main_block_desc_0 {
//    size_t reserved;
//    size_t Block_size;
//} __main_block_desc_0_DATA = { 0, sizeof(struct __main_block_impl_0)};
//
//void invoke() {
//    void (*blk)(void) = ((void (*)(void))&__main_block_impl_0((void*)__main_block_func_0, &__main_block_desc_0_DATA));
//
//    ((void (*)(__block_impl*))((__block_impl*)blk)->FuncPtr)((__block_impl*)blk);
//
//}

#endif /* BlockStruct_h */
