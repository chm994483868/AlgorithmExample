//
//  SquenceOfBST.cpp
//  Algorithm_C
//
//  Created by HM C on 2020/7/18.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "SquenceOfBST.hpp"

bool verifySquenceOfBST(int sequence[], int length) {
    if (sequence == nullptr || length <= 0)
        return false;
    
    // 1. 取得根节点。
    int root = sequence[length - 1];
    
    // 2. 在序列中找到所有左子树的节点
    // 在二叉搜索树中左子树的节点小于根节点
    int i = 0;
    for (; i < length - 1; ++i) {
        if (sequence[i] > root) { // i 的值是根节点下标的下一个下标
            break;;
        }
    }
    
    // 在二叉搜索树中右子树的结点大于根节点
    int j = i;
    for(; j < length - 1; ++j) {
        if (sequence[j] < root) {
            return false;
        }
    }
    
    // 判断左子树是不是二叉搜索树
    bool left = true;
    if (i > 0)
        left = verifySquenceOfBST(sequence, i);
    
    // 判断右子树是不是二叉搜索树
    bool right = true;
    if (i < length - 1)
        right = verifySquenceOfBST(sequence + i, length - i - 1);
    
    return (left && right);
}
