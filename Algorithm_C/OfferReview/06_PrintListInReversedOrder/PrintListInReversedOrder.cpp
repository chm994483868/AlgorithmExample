//
//  PrintListInReversedOrder.cpp
//  OfferReview
//
//  Created by CHM on 2020/7/27.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "PrintListInReversedOrder.hpp"

// 往链表末尾添加一个节点
void PrintListInReversedOrder::addToTail(ListNode** pHead, int value) {
    if (pHead == nullptr) {
        return;
    }
    
    ListNode* pNew = new ListNode();
    pNew->m_nValue = value;
    pNew->m_pNext = nullptr;
    
    if (*pHead == nullptr) {
        *pHead = pNew;
    } else {
        ListNode* pNode = *pHead;
        while (pNode->m_pNext != nullptr) {
            pNode = pNode->m_pNext;
        }
        
        pNode->m_pNext = pNew;
    }
}

// 在链表中找到第一个含有某值的节点并删除该节点的代码
void PrintListInReversedOrder::removeNode(ListNode** pHead, int value) {
    if (pHead == nullptr || *pHead == nullptr) {
        return;
    }
    
    ListNode* pToBeDeleted = nullptr;
    if ((*pHead)->m_nValue == value)  {
        pToBeDeleted = *pHead;
        *pHead = (*pHead)->m_pNext;
    } else {
        ListNode* pNode = *pHead;
        while (pNode->m_pNext != nullptr && pNode->m_pNext->m_nValue != value) {
            pNode = pNode->m_pNext;
        }
        
        if (pNode->m_pNext != nullptr && pNode->m_pNext->m_nValue == value) {
            pToBeDeleted = pNode->m_pNext;
            pNode->m_pNext = pNode->m_pNext->m_pNext;
        }
    }
    
    if (pToBeDeleted != nullptr) {
        delete pToBeDeleted;
        pToBeDeleted = nullptr;
    }
}

void PrintListInReversedOrder::printListReversingly_Iteratively(ListNode* pHead) {
    if (pHead == nullptr) {
        return;
    }
    
    std::stack<ListNode*> nodes;
    ListNode* pNode = pHead;
    while (pNode != nullptr) {
        nodes.push(pNode);
        pNode = pNode->m_pNext;
    }
    
    while (!nodes.empty()) {
        ListNode* node = nodes.top();
        printf("%d\t", node->m_nValue);
        nodes.pop();
    }
}

void PrintListInReversedOrder::printListReversingly_Recursively(ListNode* pHead) {
    if (pHead != nullptr) {
        if (pHead->m_pNext != nullptr) {
            printListReversingly_Recursively(pHead->m_pNext);
        }
        
        printf("%d\t", pHead->m_nValue);
    }
}


// 辅助函数
PrintListInReversedOrder::ListNode* PrintListInReversedOrder::CreateListNode(int value) {
    ListNode* pNode = new ListNode();
    pNode->m_nValue = value;
    pNode->m_pNext = nullptr;
    
    return pNode;
}

void PrintListInReversedOrder::ConnectListNodes(ListNode* pCurrent, ListNode* pNext) {
    if (pCurrent == nullptr) {
        printf("Error to connect two nodes.\n");
        exit(1);
    }

    pCurrent->m_pNext = pNext;
}

void PrintListInReversedOrder::PrintListNode(ListNode* pNode) {
    if (pNode == nullptr) {
        printf("The node is nullptr\n");
    } else {
        printf("The key in node is %d.\n", pNode->m_nValue);
    }
}

void PrintListInReversedOrder::PrintList(ListNode* pHead) {
    printf("PrintList starts.\n");
    
    ListNode* pNode = pHead;
    while(pNode != nullptr) {
        printf("%d\t", pNode->m_nValue);
        pNode = pNode->m_pNext;
    }

    printf("\nPrintList ends.\n");
}

void PrintListInReversedOrder::DestroyList(ListNode* pHead) {
    ListNode* pNode = pHead;
    while(pNode != nullptr) {
        // 先取得下一个节点
        pHead = pHead->m_pNext;
        // 释放当前节点
        delete pNode;
        // 更新当前节点
        pNode = pHead;
    }
}

// 测试代码
void PrintListInReversedOrder::Test(ListNode* pHead) {
    PrintList(pHead);
    printListReversingly_Iteratively(pHead);
    printf("\n");
    printListReversingly_Recursively(pHead);
}

// 1->2->3->4->5
void PrintListInReversedOrder::Test1() {
    printf("Test1 begins.\n");

    ListNode* pNode1 = CreateListNode(1);
    ListNode* pNode2 = CreateListNode(2);
    ListNode* pNode3 = CreateListNode(3);
    ListNode* pNode4 = CreateListNode(4);
    ListNode* pNode5 = CreateListNode(5);

    ConnectListNodes(pNode1, pNode2);
    ConnectListNodes(pNode2, pNode3);
    ConnectListNodes(pNode3, pNode4);
    ConnectListNodes(pNode4, pNode5);

    Test(pNode1);

    DestroyList(pNode1);
}

// 只有一个结点的链表: 1
void PrintListInReversedOrder::Test2() {
    printf("\nTest2 begins.\n");

    ListNode* pNode1 = CreateListNode(1);

    Test(pNode1);

    DestroyList(pNode1);
}

// 空链表
void PrintListInReversedOrder::Test3() {
    printf("\nTest3 begins.\n");

    Test(nullptr);
}

void PrintListInReversedOrder::Test() {
    Test1();
    Test2();
    Test3();
}
