//
//  main.cpp
//  OfferReview
//
//  Created by HM C on 2020/7/26.
//  Copyright © 2020 CHM. All rights reserved.
//

#include <iostream>
#include "AssignmentOperator.hpp"
#include "FindDuplication.hpp"
#include "FindDuplicationNoEdit.hpp"
#include "FindInPartiallySortedMatrix.hpp"
#include "ReplaceSpaces.hpp"
#include "PrintListInReversedOrder.hpp"
#include "ConstructBinaryTree.hpp"

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "《Sword of offer》Review starting 🎉🎉🎉 \n";
    
    // 1. 赋值运算符函数
//    AssignmentOperator::Test();
    // 3.(一) 找出数组中重复的数字
//    FindDuplication::Test();
    // 3.(二) 不修改数组找出重复的数字
//    FindDuplicationNoEdit::Test();
    // 4. 二维数组中的查找
//    FindInPartiallySortedMatrix::Test();
    // 5. 替换空格
//    ReplaceSpaces::Test();
    // 6. 从尾到头打印链表
//    PrintListInReversedOrder::Test();
    // 7. 重建二叉树
    ConstructBinaryTree::Test();
    
    return 0;
}
