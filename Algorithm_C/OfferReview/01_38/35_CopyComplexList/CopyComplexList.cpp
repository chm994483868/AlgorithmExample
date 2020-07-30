//
//  CopyComplexList.cpp
//  OfferReview
//
//  Created by CHM on 2020/7/30.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "CopyComplexList.hpp"

void CopyComplexList::cloneNodes(ComplexListNode* pHead) {
    ComplexListNode* pNode = pHead;
    while (pNode != nullptr) {
        ComplexListNode* pCloned = new ComplexListNode();
        pCloned->m_nValue = pNode->m_nValue;
        pCloned->m_pSibling = nullptr;
        pCloned->m_pNext = pNode->m_pNext;
        
        pNode->m_pNext = pCloned;
        pNode = pCloned->m_pNext;
    }
}

void CopyComplexList::connectSiblingNodes(ComplexListNode* pHead) {
    ComplexListNode* pNode = pHead;
    while (pNode != nullptr) {
        if (pNode->m_pSibling != nullptr) {
            pNode->m_pNext->m_pSibling = pNode->m_pSibling->m_pNext;
        }
        
        pNode = pNode->m_pNext->m_pNext;
    }
}

ComplexListNode* CopyComplexList::reconnectNodes(ComplexListNode* pHead) {
    ComplexListNode* pNode = pHead;
    ComplexListNode* pClonedHead = nullptr;
    ComplexListNode* pClonedNode = nullptr;
    
    if (pNode != nullptr) {
        pClonedHead = pClonedNode = pNode->m_pNext;
        pNode->m_pNext = pClonedNode->m_pNext;
        pNode = pNode->m_pNext;
    }
    
    while (pNode != nullptr) {
        pClonedNode->m_pNext = pNode->m_pNext;
        pClonedNode = pClonedNode->m_pNext;
        
        pNode->m_pNext = pClonedNode->m_pNext;
        pNode = pNode->m_pNext;
    }
    
    return pClonedHead;
}

ComplexListNode* CopyComplexList::clone(ComplexListNode* pHead) {
    cloneNodes(pHead);
    connectSiblingNodes(pHead);
    return reconnectNodes(pHead);
}

// 测试代码
void CopyComplexList::Test(const char* testName, ComplexListNode* pHead) {
    if(testName != nullptr)
        printf("%s begins:\n", testName);

    printf("The original list is:\n");
    PrintList(pHead);

    ComplexListNode* pClonedHead = clone(pHead);

    printf("The cloned list is:\n");
    PrintList(pClonedHead);
}

//          -----------------
//         \|/              |
//  1-------2-------3-------4-------5
//  |       |      /|\             /|\
//  --------+--------               |
//          -------------------------
void CopyComplexList::Test1() {
    ComplexListNode* pNode1 = CreateNode(1);
    ComplexListNode* pNode2 = CreateNode(2);
    ComplexListNode* pNode3 = CreateNode(3);
    ComplexListNode* pNode4 = CreateNode(4);
    ComplexListNode* pNode5 = CreateNode(5);

    BuildNodes(pNode1, pNode2, pNode3);
    BuildNodes(pNode2, pNode3, pNode5);
    BuildNodes(pNode3, pNode4, nullptr);
    BuildNodes(pNode4, pNode5, pNode2);

    Test("Test1", pNode1);
}

// m_pSibling指向结点自身
//          -----------------
//         \|/              |
//  1-------2-------3-------4-------5
//         |       | /|\           /|\
//         |       | --             |
//         |------------------------|
void CopyComplexList::Test2() {
    ComplexListNode* pNode1 = CreateNode(1);
    ComplexListNode* pNode2 = CreateNode(2);
    ComplexListNode* pNode3 = CreateNode(3);
    ComplexListNode* pNode4 = CreateNode(4);
    ComplexListNode* pNode5 = CreateNode(5);

    BuildNodes(pNode1, pNode2, nullptr);
    BuildNodes(pNode2, pNode3, pNode5);
    BuildNodes(pNode3, pNode4, pNode3);
    BuildNodes(pNode4, pNode5, pNode2);

    Test("Test2", pNode1);
}

// m_pSibling形成环
//          -----------------
//         \|/              |
//  1-------2-------3-------4-------5
//          |              /|\
//          |               |
//          |---------------|
void CopyComplexList::Test3() {
    ComplexListNode* pNode1 = CreateNode(1);
    ComplexListNode* pNode2 = CreateNode(2);
    ComplexListNode* pNode3 = CreateNode(3);
    ComplexListNode* pNode4 = CreateNode(4);
    ComplexListNode* pNode5 = CreateNode(5);

    BuildNodes(pNode1, pNode2, nullptr);
    BuildNodes(pNode2, pNode3, pNode4);
    BuildNodes(pNode3, pNode4, nullptr);
    BuildNodes(pNode4, pNode5, pNode2);

    Test("Test3", pNode1);
}

// 只有一个结点
void CopyComplexList::Test4() {
    ComplexListNode* pNode1 = CreateNode(1);
    BuildNodes(pNode1, nullptr, pNode1);

    Test("Test4", pNode1);
}

// 鲁棒性测试
void CopyComplexList::Test5() {
    Test("Test5", nullptr);
}

void CopyComplexList::Test() {
    Test1();
    Test2();
    Test3();
    Test4();
    Test5();
}
