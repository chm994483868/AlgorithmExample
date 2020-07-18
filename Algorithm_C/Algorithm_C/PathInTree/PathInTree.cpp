//
//  PathInTree.cpp
//  Algorithm_C
//
//  Created by HM C on 2020/7/18.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "PathInTree.hpp"

void findPathTree(BinaryTreeNode* pRoot, int expectedSum, std::vector<int>& path, int& currentSum);

void findPathTree(BinaryTreeNode* pRoot, int expectedSum) {
    if (pRoot == nullptr)
        return;
    
    // 1. 准备一个 vector
    std::vector<int> path;
    // 2. 记录当前的和
    int currentSum = 0;
    // 3. 调用细分函数，有 4 个参数，分别是: 1⃣️ 根节点 2⃣️ 期望的路径和 3⃣️ 表示当前路径的 vector 4⃣️ 当前的和
    findPathTree(pRoot, expectedSum, path, currentSum);
}

void findPathTree(BinaryTreeNode* pRoot, int expectedSum, std::vector<int>& path, int& currentSum) {
    // 4. 把节点的值加起来
    currentSum += pRoot->m_nValue;
    // 5. 把节点的值放进 vector，等到满足路径和时需要被打印出来
    path.push_back(pRoot->m_nValue);
    
    // 6. 如果是叶结点，并且路径上节点的和等于输入的值，则打印这条路径
    bool isLeaf = pRoot->m_pLeft == nullptr && pRoot->m_pRight == nullptr; // 左右均为空表示叶结点
    if (currentSum == expectedSum && isLeaf) { // 和相等并且是叶结点
        printf("A path is found: ");
        std::vector<int>::iterator iter = path.begin();
        for (; iter != path.end(); ++iter)
            printf("%d\t", *iter);
        
        printf("\n");
    }
    
    // 7. 如果不是叶结点，则遍历它的子结点，分别递归调用左右子树
    if (pRoot->m_pLeft != nullptr)
        findPathTree(pRoot->m_pLeft, expectedSum, path, currentSum);
    
    if (pRoot->m_pRight != nullptr)
        findPathTree(pRoot->m_pRight, expectedSum, path, currentSum);
    
    // 8. 在返回到父结点之前，在路径上删除当前结点，并在 currentSum 中减去当前结点的值
    currentSum -= pRoot->m_nValue;
    path.pop_back();
}

void findPathTree_Review(BinaryTreeNode* pRoot, int expectedSum, std::vector<int>& path, int& currentSum) {
    currentSum += pRoot->m_nValue;
    path.push_back(pRoot->m_nValue);
    
    bool isLeaf = pRoot->m_pLeft == nullptr && pRoot->m_pRight == nullptr;
    if (currentSum == expectedSum && isLeaf) {
        printf("A path is found: ");
        std::vector<int>::iterator iter = path.begin();
        for (; iter < path.end(); ++iter)
            printf("%d", *iter);
        
        printf("\n");
    }
    
    if (pRoot->m_pLeft != nullptr)
        findPathTree_Review(pRoot->m_pLeft, expectedSum, path, currentSum);
    
    if (pRoot->m_pRight != nullptr)
        findPathTree_Review(pRoot->m_pRight, expectedSum, path, currentSum);
    
    currentSum -= pRoot->m_nValue;
    path.pop_back();
}

void findPathTree_Review(BinaryTreeNode* pRoot, int expectedSum) {
    if (pRoot == nullptr)
        return;
    
    std::vector<int> path;
    int currentSum = 0;
    findPathTree_Review(pRoot, expectedSum, path, currentSum);
}

BinaryTreeNode* CreateBinaryTreeNode(int value) {
    BinaryTreeNode* pNode = new BinaryTreeNode();
    pNode->m_nValue = value;
    pNode->m_pLeft = nullptr;
    pNode->m_pRight = nullptr;

    return pNode;
}

void ConnectTreeNodes(BinaryTreeNode* pParent, BinaryTreeNode* pLeft, BinaryTreeNode* pRight) {
    if (pParent != nullptr) {
        pParent->m_pLeft = pLeft;
        pParent->m_pRight = pRight;
    }
}

void DestroyTree(BinaryTreeNode* pRoot) {
    if (pRoot != nullptr) {
        BinaryTreeNode* pLeft = pRoot->m_pLeft;
        BinaryTreeNode* pRight = pRoot->m_pRight;

        delete pRoot;
        pRoot = nullptr;

        DestroyTree(pLeft);
        DestroyTree(pRight);
    }
}

void Test_FindPathTree(char* testName, BinaryTreeNode* pRoot, int expectedSum)
{
    if(testName != nullptr)
        printf("%s begins:\n", testName);

    findPathTree(pRoot, expectedSum);

    printf("\n");
}

//            10
//         /      \
//        5        12
//       /\
//      4  7
// ”–¡ΩÃı¬∑æ∂…œµƒΩ·µ„∫ÕŒ™22
void Test1_FindPathTree()
{
    BinaryTreeNode* pNode10 = CreateBinaryTreeNode(10);
    BinaryTreeNode* pNode5 = CreateBinaryTreeNode(5);
    BinaryTreeNode* pNode12 = CreateBinaryTreeNode(12);
    BinaryTreeNode* pNode4 = CreateBinaryTreeNode(4);
    BinaryTreeNode* pNode7 = CreateBinaryTreeNode(7);

    ConnectTreeNodes(pNode10, pNode5, pNode12);
    ConnectTreeNodes(pNode5, pNode4, pNode7);

    printf("Two paths should be found in Test1.\n");
    Test_FindPathTree("Test1", pNode10, 22);

    DestroyTree(pNode10);
}

//            10
//         /      \
//        5        12
//       /\
//      4  7
// √ª”–¬∑æ∂…œµƒΩ·µ„∫ÕŒ™15
void Test2_FindPathTree()
{
    BinaryTreeNode* pNode10 = CreateBinaryTreeNode(10);
    BinaryTreeNode* pNode5 = CreateBinaryTreeNode(5);
    BinaryTreeNode* pNode12 = CreateBinaryTreeNode(12);
    BinaryTreeNode* pNode4 = CreateBinaryTreeNode(4);
    BinaryTreeNode* pNode7 = CreateBinaryTreeNode(7);

    ConnectTreeNodes(pNode10, pNode5, pNode12);
    ConnectTreeNodes(pNode5, pNode4, pNode7);

    printf("No paths should be found in Test2.\n");
    Test_FindPathTree("Test2", pNode10, 15);

    DestroyTree(pNode10);
}

//               5
//              /
//             4
//            /
//           3
//          /
//         2
//        /
//       1
// ”–“ªÃı¬∑æ∂…œ√ÊµƒΩ·µ„∫ÕŒ™15
void Test3_FindPathTree()
{
    BinaryTreeNode* pNode5 = CreateBinaryTreeNode(5);
    BinaryTreeNode* pNode4 = CreateBinaryTreeNode(4);
    BinaryTreeNode* pNode3 = CreateBinaryTreeNode(3);
    BinaryTreeNode* pNode2 = CreateBinaryTreeNode(2);
    BinaryTreeNode* pNode1 = CreateBinaryTreeNode(1);

    ConnectTreeNodes(pNode5, pNode4, nullptr);
    ConnectTreeNodes(pNode4, pNode3, nullptr);
    ConnectTreeNodes(pNode3, pNode2, nullptr);
    ConnectTreeNodes(pNode2, pNode1, nullptr);

    printf("One path should be found in Test3.\n");
    Test_FindPathTree("Test3", pNode5, 15);

    DestroyTree(pNode5);
}

// 1
//  \
//   2
//    \
//     3
//      \
//       4
//        \
//         5
// √ª”–¬∑æ∂…œ√ÊµƒΩ·µ„∫ÕŒ™16
void Test4_FindPathTree()
{
    BinaryTreeNode* pNode1 = CreateBinaryTreeNode(1);
    BinaryTreeNode* pNode2 = CreateBinaryTreeNode(2);
    BinaryTreeNode* pNode3 = CreateBinaryTreeNode(3);
    BinaryTreeNode* pNode4 = CreateBinaryTreeNode(4);
    BinaryTreeNode* pNode5 = CreateBinaryTreeNode(5);

    ConnectTreeNodes(pNode1, nullptr, pNode2);
    ConnectTreeNodes(pNode2, nullptr, pNode3);
    ConnectTreeNodes(pNode3, nullptr, pNode4);
    ConnectTreeNodes(pNode4, nullptr, pNode5);

    printf("No paths should be found in Test4.\n");
    Test_FindPathTree("Test4", pNode1, 16);

    DestroyTree(pNode1);
}

//  ˜÷–÷ª”–1∏ˆΩ·µ„
void Test5_FindPathTree()
{
    BinaryTreeNode* pNode1 = CreateBinaryTreeNode(1);

    printf("One path should be found in Test5.\n");
    Test_FindPathTree("Test5", pNode1, 1);

    DestroyTree(pNode1);
}

//  ˜÷–√ª”–Ω·µ„
void Test6_FindPathTree()
{
    printf("No paths should be found in Test6.\n");
    Test_FindPathTree("Test6", nullptr, 0);
}

void startTest_FindPathTree() {
    Test1_FindPathTree();
    Test2_FindPathTree();
    Test3_FindPathTree();
    Test4_FindPathTree();
    Test5_FindPathTree();
    Test6_FindPathTree();
}
