# 《剑指 Offer》面试题三十一～面试题四十的总结

> &emsp;上一篇是 21～30 题，本篇是 31～40 题。⛽️⛽️

## 面试题 31:栈的压入、弹出序列
&emsp;题目：输入两个整数序列，第一个序列表示栈的压入顺序，请判断第二个序列是否为该栈的弹出顺序。假设压入栈的所有数字均不相等。例如序列 1、2、3、4、5 是某栈的压栈序列，序列 4、5、3、2、1 是该压栈序列对应的一个弹出序列，但 4、3、5、1、2 就不可能是该压栈序列的弹出序列。
```c++
namespace StackPushPopOrder {
bool isPopOrder(const int* pPush, const int* pPop, int nLength);
}

bool StackPushPopOrder::isPopOrder(const int* pPush, const int* pPop, int nLength) {
    bool bPossible = false;
    
    if (pPush != nullptr && pPop != nullptr && nLength > 0) {
        const int* pNextPush = pPush;
        const int* pNextPop = pPop;
        
        stack<int> stackData;
        
        while (pNextPop - pPop < nLength) {
            while (stackData.empty() || stackData.top() != *pNextPop) {
                if (pNextPush - pPush == nLength) {
                    break;
                }
                
                stackData.push(*pNextPush);
                ++pNextPush;
            }
            
            if (stackData.top() != *pNextPop) {
                break;
            }
            
            stackData.pop();
            ++pNextPop;
        }
        
        if (stackData.empty() && pNextPop - pPop == nLength) {
            bPossible = true;
        }
    }
    
    return bPossible;
}
```
## 32:(一)不分行从上往下打印二叉树
&emsp;题目：从上往下打印出二叉树的每个结点，同一层的结点按照从左到右的顺序打印。
```c++
namespace PrintTreeFromTopToBottom {
void printFromTopToBottom(BinaryTreeNode* pRoot);
}

void PrintTreeFromTopToBottom::printFromTopToBottom(BinaryTreeNode* pRoot) {
    if (pRoot == nullptr) {
        return;
    }
    
    deque<BinaryTreeNode*> dequeTreeNode;
    dequeTreeNode.push_back(pRoot);
    
    while (!dequeTreeNode.empty()) {
        BinaryTreeNode* node = dequeTreeNode.front();
        dequeTreeNode.pop_front();
        
        printf("%d\t", node->m_nValue);
        
        if (node->m_pLeft != nullptr) {
            dequeTreeNode.push_back(node->m_pLeft);
        }
        
        if (node->m_pRight != nullptr) {
            dequeTreeNode.push_back(node->m_pRight);
        }
    }
}
```
## 32:(二)分行从上到下打印二叉树
&emsp;题目：从上到下按层打印二叉树，同一层的结点按从左到右的顺序打印，每一层打印到一行。
```c++
namespace PrintTreesInLines {
// 32（二）：分行从上到下打印二叉树
// 题目：从上到下按层打印二叉树，同一层的结点按从左到右的顺序打印，每一层
// 打印到一行。
void print(BinaryTreeNode* pRoot);
}

void PrintTreesInLines::print(BinaryTreeNode* pRoot) {
    if (pRoot == nullptr) {
        return;
    }
    
    deque<BinaryTreeNode*> dequeTreeNode;
    dequeTreeNode.push_back(pRoot);
    int nextLevel = 0;
    int toBePrinted = 1;
    while (!dequeTreeNode.empty()) {
        BinaryTreeNode* node = dequeTreeNode.front();
        printf("%d ", node->m_nValue);
        
        if (node->m_pLeft != nullptr) {
            dequeTreeNode.push_back(node->m_pLeft);
            ++nextLevel;
        }
        
        if (node->m_pRight != nullptr) {
            dequeTreeNode.push_back(node->m_pRight);
            ++nextLevel;
        }
        
        dequeTreeNode.pop_front();
        --toBePrinted;
        if (toBePrinted == 0) {
            printf("\n");
            toBePrinted = nextLevel;
            nextLevel = 0;
        }
    }
}
```
## 32:(三)之字形打印二叉树
&emsp;题目：请实现一个函数按照之字形顺序打印二叉树，即第一行按照从左到右的顺序打印，第二层按照从右到左的顺序打印，第三行再按照从左到右的顺序打印，其他行以此类推。
```c++
namespace PrintTreesInZigzag {
void print(BinaryTreeNode* pRoot);
}

void PrintTreesInZigzag::print(BinaryTreeNode* pRoot) {
    if (pRoot == nullptr) {
        return;
    }
    
    stack<BinaryTreeNode*> levels[2];
    int current = 0;
    int next = 1;
    
    levels[current].push(pRoot);
    
    while (!levels[0].empty() || !levels[1].empty()) {
        BinaryTreeNode* node = levels[current].top();
        levels[current].pop();
        
        printf("%d ", node->m_nValue);
        
        if (current == 0) {
            if (node->m_pLeft != nullptr) {
                levels[next].push(node->m_pLeft);
            }
            
            if (node->m_pRight != nullptr) {
                levels[next].push(node->m_pRight);
            }
        } else {
            if (node->m_pRight != nullptr) {
                levels[next].push(node->m_pRight);
            }
            
            if (node->m_pLeft != nullptr) {
                levels[next].push(node->m_pLeft);
            }
        }
        
        if (levels[current].empty()) {
            printf("\n");
            current = 1 - current;
            next = 1 - next;
        }
    }
}
```
## 面试题 33:二叉搜索树的后序遍历序列
&emsp;题目：输入一个整数数组，判断该数组是不是某二叉搜索树的后序遍历的结果。如果是则返回 true，否则返回 false。假设输入的数组的任意两个数字都互不相同。
```c++
namespace SquenceOfBST {
bool verifySquenceOfBST(int sequence[], int length);
}

bool SquenceOfBST::verifySquenceOfBST(int sequence[], int length) {
    if (sequence == nullptr || length <= 0) {
        return false;
    }
    
    int nRootValue = sequence[length - 1];
    unsigned int nLeftIndexEnd = 0;
    for (; nLeftIndexEnd < length - 1; ++nLeftIndexEnd) {
        if (sequence[nLeftIndexEnd] > nRootValue) {
            break;
        }
    }

    unsigned int nRightStart = nLeftIndexEnd;
    for (; nRightStart < length - 1; ++nRightStart) {
        if (sequence[nRightStart] < nRootValue) {
            return false;
        }
    }

    bool bLeft = true;
    if (nLeftIndexEnd > 0) {
        bLeft = verifySquenceOfBST(sequence, nLeftIndexEnd);
    }

    bool bRight = true;
    if (nLeftIndexEnd < length - 1) {
        bRight = verifySquenceOfBST(sequence + nLeftIndexEnd, length - nLeftIndexEnd - 1);
    }

    return bLeft && bRight;
}
```
## 面试题 34:二叉树中和为某一值的路径
&emsp;题目：输入一棵二叉树和一个整数，打印出二叉树中结点值的和为输入整数的所有路径。从树的根结点开始往下一直到叶结点所经过的结点形成一条路径。
```c++
namespace PathInTree {
void findPath(BinaryTreeNode* pRoot, int expectedsum, std::vector<int>& path, int& currentSum);
void findPath(BinaryTreeNode* pRoot, int expectedSum);
}

void PathInTree::findPath(BinaryTreeNode* pRoot, int expectedsum, std::vector<int>& path, int& currentSum) {
    currentSum += pRoot->m_nValue;
    path.push_back(pRoot->m_nValue);
    
    bool isLeaf = pRoot->m_pLeft == nullptr && pRoot->m_pRight == nullptr;
    if (currentSum == expectedsum && isLeaf) {
        std::vector<int>::iterator iter = path.begin();
        for (; iter != path.end() ; ++iter) {
            printf("%d\t", *iter);
        }
        
        printf("\n");
    }
    
    if (pRoot->m_pLeft != nullptr) {
        findPath(pRoot->m_pLeft, expectedsum, path, currentSum);
    }
    
    if (pRoot->m_pRight != nullptr) {
        findPath(pRoot->m_pRight, expectedsum, path, currentSum);
    }
    
    currentSum -= pRoot->m_nValue;
    path.pop_back();
}

void PathInTree::findPath(BinaryTreeNode* pRoot, int expectedSum) {
    if (pRoot == nullptr) {
        return;
    }
    
    std::vector<int> path;
    int currentSum = 0;
    findPath(pRoot, expectedSum, path, currentSum);
}
```
## 完结撒花🎉🎉，感谢陪伴！
