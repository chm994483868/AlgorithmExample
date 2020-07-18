//
//  PrintTreesInLines.cpp
//  Algorithm_C
//
//  Created by CHM on 2020/7/17.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "PrintTreesInLines.hpp"

void printTreesInLines(BinaryTreeNode* pRoot) {
    if (pRoot == nullptr)
        return;
    
    std::queue<BinaryTreeNode*> nodes;
    nodes.push(pRoot);
    
    int nextLevel = 0; // 层级
    int toBePrinted = 1; // 本层待打印的节点数
    
    while (!nodes.empty()) {
        BinaryTreeNode* pNode = nodes.front();
        printf("%d ", pNode->m_nValue);
        
        if (pNode->m_pLeft != nullptr) {
            nodes.push(pNode->m_pLeft);
            ++nextLevel;
        }
        
        if (pNode->m_pRight != nullptr) {
            nodes.push(pNode->m_pRight);
            ++nextLevel;
        }
        
        nodes.pop();
        --toBePrinted;
        
        if (toBePrinted == 0) {
            printf("\n");
            toBePrinted = nextLevel;
            nextLevel = 0;
        }
    }
}

void printTreesInLines_Review(BinaryTreeNode* pRoot) {
    if (pRoot == nullptr)
        return;
    
    std::queue<BinaryTreeNode*> nodes;
    nodes.push(pRoot);
    
    int nextLevel = 0;
    int toBePrinted = 1;
    
    while (nodes.empty()) {
        BinaryTreeNode* pNode = nodes.front();
        printf("%d ", pNode->m_nValue);
        
        if (pNode->m_pLeft != nullptr) {
            nodes.push(pNode->m_pLeft);
            ++nextLevel;
        }
        
        if (pNode->m_pRight != nullptr) {
            nodes.push(pNode->m_pRight);
            ++nextLevel;
        }
        
        nodes.pop();
        --toBePrinted;
        
        if (toBePrinted == 0) {
            printf("\n");
            toBePrinted = nextLevel;
            nextLevel = 0;
        }
    }
}
