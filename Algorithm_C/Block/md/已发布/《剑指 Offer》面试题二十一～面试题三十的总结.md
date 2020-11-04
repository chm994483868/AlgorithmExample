# 《剑指 Offer》面试题二十一～面试题三十的总结

> &emsp;上一篇是 11～20 题，本篇是 21～30 题。⛽️⛽️

## 面试题 21:调整数组顺序使奇数位于偶数前面
&emsp;题目：输入一个整数数组，实现一个函数来调整该数组中数字的顺序，使得所有奇数位于数组的前半部分，所有偶数位于数组的后半部分。
```c++
namespace ReorderArray_1 {

void reorder(int* pData, unsigned int length, bool(*func)(int));
bool isEven(int n);
void reorderOddEven_2(int* pData, unsigned int length);

}

void ReorderArray::reorder(int* pData, unsigned int length, bool(*func)(int)) {
    if (pData == nullptr || length <= 0 || func == nullptr) {
        return;
    }
    
    int* pBegin = pData;
    int* pEnd = pData + length - 1;
    
    while (pBegin < pEnd) {
        while (pBegin < pEnd && (*func)(*pBegin)) {
            ++pBegin;
        }
        
        while (pBegin < pEnd && !(*func)(*pEnd)) {
            --pEnd;
        }
        
        if (pBegin < pEnd) {
            int temp = *pBegin;
            *pBegin = *pEnd;
            *pEnd = temp;
        }
    }
}

bool ReorderArray::isEven(int n) {
    return n & 0x1; // 返回 true 表示是奇数，返回 false 表示偶数
}

void ReorderArray::reorderOddEven_2(int* pData, unsigned int length) {
    reorder(pData, length, isEven);
}
```
## 面试题 22:链表中倒数第k个结点
&emsp;题目：输入一个链表，输出该链表中倒数第k个结点。为了符合大多数人的习惯，本题从 1 开始计数，即链表的尾结点是倒数第1个结点。例如一个链表有 6 个结点，从头结点开始它们的值依次是 1、2、3、4、5、6。这个链表的倒数第 3 个结点是值为 4 的结点。
```c++
namespace KthNodeFromEnd {

// 求链表的中间节点。如果链表中的节点总数为奇数，则返回中间节点；
// 如果节点总数是偶数，则返回中间两个节点的任意一个。
// 可以定义两个指针，同时从链表的头节点出发，一个指针一次走一步
// 另一个指针一次走两步。当走的快的指针走到链表的末尾时，走的慢的指针正好
// 在链表的中间。

ListNode* findKthToTail(ListNode* pListHead, unsigned int k);
ListNode* findMddleNode(ListNode* pListHead);

}

ListNode* KthNodeFromEnd::findKthToTail(ListNode* pListHead, unsigned int k) {
    if (pListHead == nullptr || k <= 0) {
        return nullptr;
    }
    
    ListNode* pAHead = pListHead;
    unsigned int i = 0;
    for (; i < k - 1; ++i) {
        pAHead = pAHead->m_pNext;
        
        if (pAHead == nullptr) {
            return nullptr;
        }
    }
    
    ListNode* pBehind = pListHead;
    while (pAHead->m_pNext != nullptr) {
        pAHead = pAHead->m_pNext;
        pBehind = pBehind->m_pNext;
    }
    
    return pBehind;
}

ListNode* KthNodeFromEnd::findMddleNode(ListNode* pListHead) {
    if (pListHead == nullptr) {
        return nullptr;
    }

    ListNode* pFast = pListHead;
    ListNode* pSlow = pListHead;
    
//    while (pFast != nullptr && pFast->m_pNext != nullptr) {
    while (pFast->m_pNext != nullptr && pFast->m_pNext->m_pNext != nullptr) {
        pSlow = pSlow->m_pNext;
        
        pFast = pFast->m_pNext;
        if (pFast != nullptr) {
            pFast = pFast->m_pNext;
        }
    }
    
    return pSlow;
}
```
## 面试题 23:链表中环的入口结点
&emsp;题目：一个链表中包含环，如何找出环的入口结点？
```c++
namespace EntryNodeInListLoop {

ListNode* meetingNode(ListNode* pHead);
ListNode* entryNodeOfLoop(ListNode* phead);

}

ListNode* EntryNodeInListLoop::meetingNode(ListNode* pHead) {
    if (pHead == nullptr) {
        return nullptr;
    }
    
    ListNode* pSlow = pHead->m_pNext;
    if (pSlow == nullptr) {
        return nullptr;
    }
    
    ListNode* pFast = pSlow->m_pNext;
    while (pFast != nullptr && pSlow != nullptr) {
        if (pFast == pSlow) {
            return pFast;
        }
        
        pSlow = pSlow->m_pNext;
        pFast = pFast->m_pNext;
        if (pFast != nullptr) {
            pFast = pFast->m_pNext;
        }
    }
    
    return nullptr;
}

ListNode* EntryNodeInListLoop::entryNodeOfLoop(ListNode* pHead) {
    if (pHead == nullptr) {
        return nullptr;
    }
    
    ListNode* pMeetingNode = meetingNode(pHead);
    if (pMeetingNode == nullptr) {
        return nullptr;
    }
    
    unsigned int nNodesOfLoop = 1;
    ListNode* pNode1 = pMeetingNode;
    while (pNode1->m_pNext != pMeetingNode) {
        pNode1 = pNode1->m_pNext;
        ++nNodesOfLoop;
    }
    
    pNode1 = pHead;
    for (unsigned int i = 0; i < nNodesOfLoop; ++i) {
        pNode1 = pNode1->m_pNext;
    }
    
    ListNode* pNode2 = pHead;
    while (pNode1 != pNode2) {
        pNode1 = pNode1->m_pNext;
        pNode2 = pNode2->m_pNext;
    }
    
    return pNode1;
}
```
## 面试题 24:反转链表
&emsp;题目：定义一个函数，输入一个链表的头结点，反转该链表并输出反转后链表的头结点。
```c++
namespace ReverseList {

ListNode* reverseList(ListNode* pHead);

}

ListNode* ReverseList::reverseList(ListNode* pHead) {
    ListNode* pReverseHead = nullptr;
    ListNode* pNode = pHead;
    ListNode* pPrev = nullptr;
    
    while (pNode != nullptr) {
        ListNode* pNext = pNode->m_pNext;
        
        if (pNext == nullptr) {
            pReverseHead = pNode;
        }
        
        pNode->m_pNext = pPrev;
        pPrev = pNode;
        pNode = pNext;
    }
    
    return pReverseHead;
}
```
## 面试题 25:合并两个排序的链表
&emsp;题目：输入两个递增排序的链表，合并这两个链表并使新链表中的结点仍然是按照递增排序的。
```c++
namespace MergeSortedLists {

ListNode* merge(ListNode* pHead1, ListNode* pHead2);

}

ListNode* MergeSortedLists::merge(ListNode* pHead1, ListNode* pHead2) {
    if (pHead1 == nullptr) {
        return pHead2;
    }
    
    if (pHead2 == nullptr) {
        return pHead1;
    }
    
    ListNode* pMergeHead = nullptr;
    if (pHead1->m_nValue < pHead2->m_nValue) {
        pMergeHead = pHead1;
        pMergeHead->m_pNext = merge(pHead1->m_pNext, pHead2);
    } else {
        pMergeHead = pHead2;
        pMergeHead->m_pNext = merge(pHead1, pHead2->m_pNext);
    }
    
    return pMergeHead;
}
```
## 面试题 26:树的子结构
&emsp;题目：输入两棵二叉树 A 和 B，判断 B 是不是 A 的子结构。
```c++
namespace SubstructureInTree {

struct BinaryTreeNode {
    double m_dbValue;
    BinaryTreeNode* m_pLeft;
    BinaryTreeNode* m_pRight;
};

bool doesTree1HaveTree2(BinaryTreeNode* pRoot1, BinaryTreeNode* pRoot2);
bool equal(double num1, double num2);
bool hasSubtree(BinaryTreeNode* pRoot1, BinaryTreeNode* pRoot2);

}

bool SubstructureInTree::doesTree1HaveTree2(BinaryTreeNode* pRoot1, BinaryTreeNode* pRoot2) {
    if (pRoot2 == nullptr) {
        return true;
    }
    
    if (pRoot1 == nullptr) {
        return false;
    }
    
    if (!equal(pRoot1->m_dbValue, pRoot2->m_dbValue)) {
        return false;
    }
    
    return doesTree1HaveTree2(pRoot1->m_pLeft, pRoot2->m_pLeft) && doesTree1HaveTree2(pRoot1->m_pRight, pRoot2->m_pRight);
}

bool SubstructureInTree::equal(double num1, double num2) {
    if (num1 - num2 > -0.0000001 && num1 - num2 < 0.0000001) {
        return true;
    } else {
        return false;
    }
}

bool SubstructureInTree::hasSubtree(BinaryTreeNode* pRoot1, BinaryTreeNode* pRoot2) {
    bool result = false;
    
    if (pRoot1 != nullptr && pRoot2 != nullptr) {
        if (equal(pRoot1->m_dbValue, pRoot2->m_dbValue)) {
            result = doesTree1HaveTree2(pRoot1, pRoot2);
        }
        
        if (!result) {
            result = hasSubtree(pRoot1->m_pLeft, pRoot2);
        }
        
        if (!result) {
            result = hasSubtree(pRoot1->m_pRight, pRoot2);
        }
    }
    
    return result;
}
```
