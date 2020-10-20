//
//  main.cpp
//  OfferReview
//
//  Created by HM C on 2020/7/26.
//  Copyright © 2020 CHM. All rights reserved.
//

#include <iostream>

#include "BubbleSort.hpp"
#include "InsertSort.hpp"
#include "ShellSort.hpp"
#include "SelectSort.hpp"
#include "QuickSort.hpp"
#include "MergeSort.hpp"
#include "HeapSort.hpp"

#include "AssignmentOperator.hpp"
#include "FindDuplication.hpp"
#include "FindDuplicationNoEdit.hpp"
#include "FindInPartiallySortedMatrix.hpp"
#include "ReplaceSpaces.hpp"
#include "PrintListInReversedOrder.hpp"
#include "ConstructBinaryTree.hpp"
#include "NextNodeInBinaryTrees.hpp"
#include "QueueWithTwoStacks.hpp"
#include "StackWithTwoQueues.hpp"
#include "Fibonacci.hpp"
#include "MinNumberInRotatedArray.hpp"
#include "StringPathInMatrix.hpp"
#include "RobotMove.hpp"
#include "NumberOf1InBinary.hpp"
#include "CPower.hpp"
#include "Print1ToMaxOfNDigits.hpp"
#include "DeleteNodeInList.hpp"
#include "DeleteDuplicatedNode.hpp"
#include "RegularExpressions.hpp"
#include "NumericStrings.hpp"
#include "ReorderArray.hpp"
#include "KthNodeFromEnd.hpp"
#include "EntryNodeInListLoop.hpp"
#include "ReverseList.hpp"
#include "MergeSortedLists.hpp"
#include "SubstructureInTree.hpp"
#include "MirrorOfBinaryTree.hpp"
#include "SymmetricalBinaryTree.hpp"
#include "StackWithMin.hpp"
#include "StackPushPopOrder.hpp"
#include "PrintTreeFromTopToBottom.hpp"
#include "PrintTreesInLines.hpp"
#include "PrintTreesInZigzag.hpp"
#include "SquenceOfBST.hpp"
#include "PathInTree.hpp"
#include "CopyComplexList.hpp"
#include "ConvertBinarySearchTree.hpp"
#include "SerializeBinaryTrees.hpp"
#include "MoreThanHalfNumber.hpp"
#include "KLeastNumbers.hpp"
#include "StreamMedian.hpp"
#include "GreatestSumOfSubarrays.hpp"
#include "NumberOf1.hpp"
#include "DigitsInSequence.hpp"
#include "TranslateNumbersToStrings.hpp"
#include "MaxValueOfGifts.hpp"
using namespace std;

//// 定义类型别名
//#define DSting std::string // 不建议使用
//typedef std::string TString; // 使用 typedef 的方式
//using UString = std::string; // 使用 using typeName_Self = stdTypeName;
//// 定义函数指针
//typedef void (*tFunc)(string);
//using uFunc = void (*)(string);
//
//void tempFunc(string parm) {
//    std::cout << "🎉🎉" << parm << std::endl;
//}

class Base {
public:
    Base(){}
    ~Base(){};
    void func1() {
        std::cout << "1⃣️ func1 被调用" << std::endl;
    }
    
    void func2() {
        std::cout << "2⃣️ func2 被调用" << std::endl;
    }
};

class Sub: private Base {
public:
    using Base::func1;
    
    void func2Invoke() {
        // Base 的 func2 函数只能在 Sub 定义内部使用，
        // 外界只能通过 Sub 的 func2Invoke 来间接调用 func2 函数
        this->func2();
    }
};

template <typename N>
class TLClass {
public:
    TLClass(N parm): mV(parm) {}
    ~TLClass(){}
    
    void func() {
        std::cout << "🎉🎉🎉 " << mV << std::endl;
    }
private:
    N mV;
};

using UTLCass = TLClass<int>;

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "《Sword of offer》Review starting 🎉🎉🎉 \n";
    
//    Sub sub;
//    sub.func1();
//    sub.func2Invoke();
//    // sub.func2(); // 报错：'func2' is a private member of 'Base'
//
//    UTLCass cls(20);
//    cls.func();
    
//    DSting ds("define string");
//    TString ts("typedef string");
//    UString us("using string");
//
//    tFunc funcPtr = tempFunc;
//    (*funcPtr)(ts);
//
//    uFunc funcPtr2 = tempFunc;
//    (*funcPtr2)(us);
//    (*funcPtr2)(ds);
    
//    char s[] = "Golden Global view";
//    char m[19];
//    printf("🎉🎉🎉 前：%s\n", s);
//    memmove(m, s, strlen(s) + 1);
//    printf("🎉🎉🎉 m：%s\n", m);
//    memmove(s, s + 7, strlen(s) + 1 - 7);
//
//    printf("🎉🎉🎉 后：%s\n", s);
    
    // 1. 冒泡排序
//    BubbleSort::Test();
    // 2. 插入排序
//    InsertSort::Test();
    // 3. 希尔排序
//    ShellSort::Test();
    // 4. 选择排序
//    SelectSort::Test();
    // 5. 快速排序
//    QuickSort::Test();
    // 6. 归并排序
    MergeSort::Test();
    // 7. 堆排序
//    HeapSort::Test();
    
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
//    ConstructBinaryTree::Test();
    // 8. 二叉树的下一个结点
//    NextNodeInBinaryTrees::Test();
    // 9. 用两个栈实现队列
//    QueueWithTwoStacks::Test();
    // 10. 用两个队列实现栈
//    StackWithTwoQueues::Test();
    // 11. 斐波那契数列
//    Fibonacci::Test();
    // 12. 旋转数组的最小数字。
//    MinNumberInRotatedArray::Test();
    // 13. 矩阵中的路径
//    StringPathInMatrix::Test();
    // 14. 机器人的运动范围
//    RobotMove::Test();
    // 15. 二进制中1的个数
//    NumberOf1InBinary::Test();
    // 16. 数值的整数次方
//    CPower::Test();
    // 17. 打印1到最大的n位数
//    Print1ToMaxOfNDigits::Test();
    // 18. 在O(1)时间删除链表结点。
//    DeleteNodeInList::Test();
    // 19. 删除链表中重复的结点。
//    DeleteDuplicatedNode::Test();
    // 20. 正则表达式匹配
//    RegularExpressions::Test();
    // 21. 表示数值的字符串
//    NumericStrings::Test();
    // 22. 调整数组顺序使奇数位于偶数前面
//    ReorderArray::Test();
    // 23. 链表中倒数第k个结点
//    KthNodeFromEnd::Test();
    // 24. 链表中环的入口结点
//    EntryNodeInListLoop::Test();
    // 25. 反转链表
//    ReverseList::Test();
    // 26. 合并排序的链表
//    MergeSortedLists::Test();
    // 27. 树的子结构
//    SubstructureInTree::Test();
    // 28. 二叉树的镜像
//    MirrorOfBinaryTree::Test();
    // 29. 对称的二叉树
//    SymmetricalBinaryTree::Test();
    // 30. 包含min函数的栈
//    StackWithMin::Test();
    // 31. 栈的压入、弹出序列
//    StackPushPopOrder::Test();
    // 32. 不分行从上往下打印二叉树
//    PrintTreeFromTopToBottom::Test();
    // 32. 分行从上到下打印二叉树
//    PrintTreesInLines::Test();
    // 32. 之字形打印二叉树
//    PrintTreesInZigzag::Test();
    // 33. 二叉搜索树的后序遍历序列
//    SquenceOfBST::Test();
    // 34. 二叉树中和为某一值的路径
//    PathInTree::Test();
    // 35. 复杂链表的复制
//    CopyComplexList::Test();
    // 36. 二叉搜索树与双向链表
//    ConvertBinarySearchTree::Test();
    // 37. 序列化二叉树
//    SerializeBinaryTrees::Test();
    // 39. 数组中出现次数超过一半的数字
//    MoreThanHalfNumber::Test();
    // 40. 最小的k个数
//    KLeastNumbers::Test();
    // 41. 数据流中的中位数
//    StreamMedian::Test();
    // 42. 连续子数组的最大和
//    GreatestSumOfSubarrays::Test();
    // 43. 从1到n整数中1出现的次数
//    NumberOf1::Test();
    // 44. 数字序列中某一位的数字
//    DigitsInSequence::Test();
    // 46. 把数字翻译成字符串
//    TranslateNumbersToStrings::Test();
    // 47. 礼物的最大价值
//    MaxValueOfGifts::Test();
    
    return 0;
}
