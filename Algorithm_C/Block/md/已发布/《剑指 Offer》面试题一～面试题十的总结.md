# 《剑指 Offer》面试题一～面试题十的总结

> &emsp;《剑指 Offer-名企面试官精讲典型编程题》这本书之前已经读过两遍了，这次是第三次阅读。本篇文章准备把每道题目的核心知识点记录下来方便以后进行复习回顾。

## 面试题 1:赋值运算符函数
&emsp;题目：如下为类型 CMyString 的声明，请为该类型添加赋值运算符函数。
```c++
namespace AssignmentOperator {

// 1：赋值运算符函数
// 题目：如下为类型 CMyString 的声明，请为该类型添加赋值运算符函数。

class CMyString {
public:
    CMyString(const char* pData = nullptr); // 构造函数，pData 参数默认为 nullptr
    CMyString(const CMyString& str); // 复制构造函数，参数的类型为常量引用
    ~CMyString(void); // 析构函数

    CMyString& operator = (const CMyString& str);

    void Print();

private:
    char* m_pData;
};

}

AssignmentOperator::CMyString::CMyString(const char* pData) {
    if (pData == nullptr) {
        // 同时如果这里抛了错，但我们还没有修改原来实例的状态，因此实例的状态还是有效的，这也保证了异常安全性
        m_pData = new char[1];
        m_pData[0] = '\0';
    } else{
        unsigned long length = strlen(pData);
        
        // +1 是给空字符留位置
        m_pData = new char[length + 1];
        
        strcpy(m_pData, pData);
    }
}

AssignmentOperator::CMyString::CMyString(const CMyString& str) {
    unsigned long length = strlen(str.m_pData);
    m_pData = new char[length + 1];
    strcpy(m_pData, str.m_pData);
}

AssignmentOperator::CMyString::~CMyString() {
    delete [] m_pData;
}

AssignmentOperator::CMyString& AssignmentOperator::CMyString::operator=(const CMyString& str) {

    // 一些思考：
    // char* n; n 是一个指针变量，直接打印 n 显示的是 n 这个指针指向的地址，
    // 打印 *n 显示的是 n 指向的地址里面存储的内容，打印 &n 显示的是 n 这个指针变量自己的地址。
    
    // int a = 2; int& m = a; m 是 a 的引用。
    // 引用 m 可以直接理解为 a 的一个别名。直接打印 m 时显示的是 2 即 a 的值，打印 &m 时显示的是 a 的地址，和 &a 是完成一致的。
    
    // 所以这个 if 里面，就可以得到合理的解释了
    // this 表示的是当前变量的地址，&str 表示的是参数的地址
    // 返回值是 CMyString 类型的引用，所以需要返回当前变量的值即: *this，就比如上面的 m 初始化时是直接用的 a。
    
    // 一：初级解法
    // 1. 如果是同一个 CMySting 变量，则直接返回原值
//    if (this == &str) {
//        return *this;
//    }
//
//    // 2. 释放原始值
//    delete [] m_pData;
//    m_pData = nullptr;
//
//    // 3. 为 m_pData 开辟空间，并把 str.m_pData 复制给 m_pData
//    m_pData = new char[strlen(str.m_pData) + 1];
//    strcpy(m_pData, str.m_pData);
//
//    return *this;

    // 二：考虑异常安全性的解法
    if (this != &str) {
        // 1. 调用上面的复制构造函数 CMyString(const CMyString& str) ，创建一个 CMyString 的临时变量
        CMyString strTemp(str);
        
        // 2. 取出临时变量的 m_pData 等待赋值给 this->m_pData
        char* pTemp = strTemp.m_pData;
        
        // 3. 把原始的 m_pData 赋给临时的 tempStr，待出了下面的右边花括号 tempStr 释放时一起带着原始的 m_pData 释放。
        strTemp.m_pData = m_pData;
        
        // 4. 把 str 的 m_pData 赋值给 this 的 m_pData
        m_pData = pTemp;
    }
    
    return *this;
}

void AssignmentOperator::CMyString::Print() {
    printf("%s", m_pData);
}
```
## 3:(一)找出数组中重复的数字
&emsp;题目：在一个长度为 n 的数组里的所有数字都在 0 到 n - 1 的范围内。数组中某些数字是重复的，但不知道有几个数字重复了，也不知道每个数字重复了几次。请找出数组中任意一个重复的数字。例如，如果输入长度为7的数组 {2, 3, 1, 0, 2, 5, 3}，那么对应的输出是重复的数字 2 或者 3。
```c++
namespace FindDuplication {

/// 找出数组中重复的数字，返回值：true 输入有效，并且数组中存在重复的数字 false 输入无效，或者数组中没有重复的数字
/// @param numbers 数组
/// @param length 数组长度
/// @param duplication （输出）数组中的一个重复的数字
bool duplicate(int numbers[], int length, int *duplication);

}

bool FindDuplication::duplicate(int numbers[], int length, int *duplication) {
    if (numbers == nullptr || length <= 0) {
        return false;
    }
    
    int i = 0;
    // 判断数组中的数字是否都是在 0 到 length - 1 的范围内，如果不是则直接返回 false 结束函数执行
    for (; i < length; ++i) {
        if (numbers[i] < 0 || numbers[i] > length - 1) {
            return false;
        }
    }
    
    // 循环把数组中的每个数字都放在与自己值相等的下标位置处
    for (i = 0; i < length; ++i) {
        
        // 如果取出 numbers[i] 不等于 i，表示该元素目前还没有在对应的下标处
        while (numbers[i] != i) {
            
            if (numbers[i] == numbers[numbers[i]]) {
                // 如果相等，则表示发现了一个重复的数字
                *duplication = numbers[i];
                return true;
            }
            
            // 交换元素位置，一个元素最多被交换两次就能放在自己对应的下标处
            int temp = numbers[i];
            numbers[i] = numbers[temp];
            numbers[temp] = temp;
        }
    }
    
    return false;
}
```
## 3:(二)不修改数组找出重复的数字
&emsp;题目：在一个长度为 n+1 的数组里的所有数字都在 1 到 n 的范围内，所以数组中至少有一个数字是重复的。请找出数组中任意一个重复的数字，但不能修改输入的数组。例如，如果输入长度为 8 的数组 {2, 3, 5, 4, 3, 2, 6, 7}，那么对应的输出是重复的数字2或者3。
```c++
namespace FindDuplicationNoEdit {

int countRange(const int* numbers, int length, int start, int end);

/// 不修改数组找出重复的数字, 正数 输入有效，并且数组中存在重复的数字，返回值即为重复的数字
/// @param number 数组
/// @param length 数组长度
int getDuplication(const int* number, int length);

}

// 统计数组中的元素落在指定数值范围内的次数
int FindDuplicationNoEdit::countRange(const int* numbers, int length, int start, int end) {
    if (numbers == nullptr) {
        return 0;
    }
    
    int count = 0;
    
    // 统计 numbers 中元素值在 [start, end] 区间内的数字个数
    for (int i = 0; i < length; ++i) {
        if (numbers[i] >= start && numbers[i] <= end) {
            ++count;
        }
    }
    
    return count;
}

int FindDuplicationNoEdit::getDuplication(const int* numbers, int length) {
    if (numbers == nullptr || length <= 0) {
        return -1;
    }
    
    // 采用类似二分查找的思想，统计数组中落在指定区间内的元素个数，
    // 如果某个区间长度内出现的元素个数超过了这个区间长度，则表明在该区间内一定存在重复出现的数字。
    
    // 这里的 start 和 end 对应上面的题目条件："在一个长度为 n+1 的数组里的所有数字都在 1 到 n 的范围内"
    int start = 1;
    int end = length - 1;
    
    while (start <= end) {
    
        // 从区间中间开始
        int middle = ((end - start) >> 1) + start;
        
        // 统计 [start, middle] 中元素的个数
        int count = countRange(numbers, length, start, middle);
        
        // 结束查找
        if (start == end) {
            if (count > 1) {
                return start;
            } else {
                break;
            }
        }
        
        if (count > (middle - start + 1)) {
        
            // 如果左边区间内元素出现的次数大于区间长度，则更新 end
            end = middle;
            
        } else {
            
            // 如果右边
            start = middle + 1;
        }
    }
    
    return -1;
}

```
## 面试题 4:二维数组中的查找
&emsp;题目：在一个二维数组中，每一行都按照从左到右递增的顺序排序，每一列都按照从上到下递增的顺序排序。请完成一个函数，输入这样的一个二维数组和一个整数，判断数组中是否含有该整数。
```c++
namespace FindInPartiallySortedMatrix {

bool find(int* matrix, int rows, int columns, int number);

}

bool FindInPartiallySortedMatrix::find(int* matrix, int rows, int columns, int number) {
    bool found = false;
    
    if (matrix != nullptr && rows > 0 && columns > 0) {
//        // 从右上角开始（第一行和最后一列）
//        int row = 0;
//        int column = columns - 1;
//
//        // 循环结束的条件是行数达到最大，列数达到最小
//        while (row < rows && column >= 0) {
//            // 取出值
//            int current = matrix[row * columns + column];
//
//            if (current == number) {
//                // 如果相等，即找到了
//                found = true;
//                break;
//            } else if (current > number) {
//                // 如果大于要找的值，则缩小列
//                --column;
//            } else {
//                // 如果小于要找的值，则扩大行
//                ++row;
//            }
//        }
        
        // 从左下角开始（最后一行和第一列）
        int row = rows - 1;
        int column = 0;
        
        // 循环结束的条件是列数达到最大，行数达到最小
        while (row >= 0 && column < columns) {
            // 取出值
            int current = matrix[row * columns + column];
            
            if (current == number) {
                // 如果相等，即找到了
                found = true;
                break;
            } else if (current > number) {
                // 如果大于要找的值，则缩小行
                --row;
            } else {
                // 如果小于要找的值，则扩大列
                ++column;
            }
        }
    }
    
    return found;
}
```
## 面试题 5:替换空格
&emsp;题目：请实现一个函数，把字符串中的每个空格替换成 "%20"。例如输入 “We are happy.”，则输出 “We%20are%20happy.”。
```c++
namespace ReplaceSpaces {

void replaceBlank(char str[], int length);
string replaceSpace(string s);

}

void ReplaceSpaces::replaceBlank(char str[], int length) {
    if (str == nullptr || length <= 0) {
        return;
    }
    
    int originalLength = 0;
    int numberOfBlank = 0;
    int i = 0;
    
    // while 循环分别统计字符串中空格和字符的个数
    while (str[i] != '\0') {
        if (str[i] == ' ') {
            // 统计空格个数
            ++numberOfBlank;
        }
        // 统计字符个数
        ++originalLength;
        ++i;
    }
    
    // 把空格替换为 %20 后字符串的实际长度。（一个空格增加 2）
    int newLength = originalLength + numberOfBlank * 2;
    
    if (newLength > length) {
        // 如果新字符串的长度超过了字符串的原始长度，则说明目前字符串的内存空间保存不下替换 20% 后的字符串内容，直接 return
        return;
    }
    
    // indexOfOriginal 和 indexOfNew 可分别看作原始字符串和新字符串的末尾，
    // 下面的 while 循环从后往前开始依次替换空格
    
    int indexOfOriginal = originalLength;
    int indexOfNew = newLength;
    
    while (indexOfOriginal >= 0 && indexOfNew > indexOfOriginal) {
        
        if (str[indexOfOriginal] == ' ') {
        
            // 遇到空格则替换为 20%
            str[indexOfNew--] = '0';
            str[indexOfNew--] = '2';
            str[indexOfNew--] = '%';
        } else {
        
            // 如果不是空格则挪动原始字符
            str[indexOfNew--] = str[indexOfOriginal];
        }
        
        // 指示原始字符串的下标依次往前走
        --indexOfOriginal;
    }
}

string ReplaceSpaces::replaceSpace(string s) {

    // 记录字符串原始长度
    unsigned long originalLength = s.length() - 1;
    
    int i = 0;
    // 每遇到一个空格字符串后面就拼接一个 "00"
    for (; i < s.length(); ++i) {
        if (s[i] == ' ') {
            s += "00";
        }
    }
    
    // 记录新字符串的长度
    unsigned long newLength = s.length() - 1;
    
    // 同上个循环，从后向前替换空格为 20%
    while (originalLength >= 0 && newLength > originalLength) {
        if (s[originalLength] == ' ') {
            s[newLength--] = '0';
            s[newLength--] = '2';
            s[newLength--] = '%';
        } else {
            s[newLength--] = s[originalLength];
        }

        --originalLength;
    }
    
    return s;
}
```
## 面试题 6:从尾到头打印链表
&emsp;题目：输入一个链表的头节点，从尾到头反过来打印出每个节点的值。
```c++
namespace PrintListInReversedOrder {

// 开局的两个小题
// 往链表末尾添加一个节点
void addToTail(ListNode** pHead, int value);
// 在链表中找到第一个含有某值的节点并删除该节点的代码
void removeNode(ListNode** pHead, int value);

// 6：从尾到头打印链表
// 题目：输入一个链表的头节点，从尾到头反过来打印出每个节点的值。
void printListReversingly_Iteratively(ListNode* pHead);
void printListReversingly_Recursively(ListNode* pHead);

}

// 往链表末尾添加一个节点
void PrintListInReversedOrder::addToTail(ListNode** pHead, int value) {
    if (pHead == nullptr) {
        return;
    }
    
    // 根据 int value 构建一个新节点
    ListNode* pNew = new ListNode();
    pNew->m_nValue = value;
    pNew->m_pNext = nullptr;
    
    if (*pHead == nullptr) {
        // 如果入参头节点为空，则新建的节点就是头节点了
        *pHead = pNew;
    } else {
        // 其它情况下则 while 循环找到链表的最后一个节点，把新节点放在最后一个节点的 m_pNext
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
    
    // 准备一个临时节点 pToBeDeleted 用保存找到的待删除的节点
    ListNode* pToBeDeleted = nullptr;
    
    if ((*pHead)->m_nValue == value)  {
        // 如果要删除头节点，用 pToBeDeleted 记录
        pToBeDeleted = *pHead;
        
        // 更新头节点为它的下一个节点
        *pHead = (*pHead)->m_pNext;
    } else {
        
        // while 循环开始从链表头节点开始找第一个节点的 m_nValue 的值等于 value
        ListNode* pNode = *pHead;
        while (pNode->m_pNext != nullptr && pNode->m_pNext->m_nValue != value) {
            pNode = pNode->m_pNext;
        }
        
        if (pNode->m_pNext != nullptr && pNode->m_pNext->m_nValue == value) {
            // 如果找到了，则 pToBeDeleted 记录该节点
            pToBeDeleted = pNode->m_pNext;
            // 更新原始 pNode->m_pNext 为下下一个节点
            pNode->m_pNext = pNode->m_pNext->m_pNext;
        }
    }
    
    // 如果 pToBeDeleted 不为空，则进行删除
    if (pToBeDeleted != nullptr) {
        delete pToBeDeleted;
        pToBeDeleted = nullptr;
    }
}

// 迭代的方式从尾到头打印链表，准备一个 stack 从头到尾存放每个节点，然后依次出栈打印
void PrintListInReversedOrder::printListReversingly_Iteratively(ListNode* pHead) {
    if (pHead == nullptr) {
        return;
    }
    
    // 准备一个存放节点的 stack
    std::stack<ListNode*> nodes;
    
    // 从链表头节点开始，遍历每个节点并放入 stack 中
    ListNode* pNode = pHead;
    while (pNode != nullptr) {
        nodes.push(pNode);
        pNode = pNode->m_pNext;
    }
    
    // 依次出栈打印节点
    while (!nodes.empty()) {
        ListNode* node = nodes.top();
        printf("%d\t", node->m_nValue);
        nodes.pop();
    }
}

// 递归的方式从尾到头打印链表
void PrintListInReversedOrder::printListReversingly_Recursively(ListNode* pHead) {
    if (pHead != nullptr) {
        
        // 只要 m_pNext 不为 nullptr 则一直进行递归
        if (pHead->m_pNext != nullptr) {
            printListReversingly_Recursively(pHead->m_pNext);
        }
        
        // 当 m_pNext 为 nullptr 后开始执行下面的打印语句
        printf("%d\t", pHead->m_nValue);
    }
}
```
## 面试题 7:重建二叉树
&emsp;题目：输入某二叉树的前序遍历和中序遍历的结果，请重建出该二叉树。假设输入的前序遍历和中序遍历的结果中都不含重复的数字。例如输入前序遍历序列 {1, 2, 4, 7, 3, 5, 6, 8} 和中序遍历序列 {4, 7, 2, 1, 5, 3, 8, 6}，则重建出如下所示的二叉树并输出它的头结点。
              1
           /     \
          2       3
         /       / \
        4       5   6
         \         /
          7       8

```c++
namespace ConstructBinaryTree {

struct BinaryTreeNode {
    int m_nValue;
    BinaryTreeNode* m_pLeft;
    BinaryTreeNode* m_pRight;
};

BinaryTreeNode* constructCore(int* startPreorder, int* endPreorder, int* startInorder, int* endInorder);
BinaryTreeNode* construct(int* preorder, int* inorder, int length);

// 辅助函数
BinaryTreeNode* CreateBinaryTreeNode(int value);
void ConnectTreeNodes(BinaryTreeNode* pParent, BinaryTreeNode* pLeft, BinaryTreeNode* pRight);
void PrintTreeNode(const BinaryTreeNode* pNode);
void PrintTree(const BinaryTreeNode* pRoot);
void DestroyTree(BinaryTreeNode* pRoot);

}

ConstructBinaryTree::BinaryTreeNode*  ConstructBinaryTree::constructCore(int* startPreorder, int* endPreorder, int* startInorder, int* endInorder) {

    // 前序遍历序列的第一个数字是根节点的值
    int rootValue = startPreorder[0];
    
    // 构建根节点
    BinaryTreeNode* root = new BinaryTreeNode();
    root->m_nValue = rootValue;
    root->m_pLeft = root->m_pRight = nullptr;
    
    // 判断是否是就一个根结点的树 (如果前序遍历序列头尾指针相同，中序遍历序列头尾指针相同，且四个指针的值都相同)
    if (startPreorder == endPreorder) {
        if (startInorder == endInorder && *startPreorder == *startInorder) {
            return root;
        } else {
            throw std::exception(); // 入参错误
        }
    }

    // 从中序序列的起点开始，在中序序列中找到根节点
    int* rootInorder = startInorder;
    while (rootInorder <= endInorder && *rootInorder != rootValue) {
        ++rootInorder;
    }

    // 如果一直遍历到中序序列的尾部后还是没有找到根节点，则入参错误
    if (rootInorder == endInorder && *rootInorder != rootValue) {
        throw std::exception(); // 入参错误，在中序序列中没有找到根节点
    }

    // 取得左子树的长度（根节点和中序序列第一个节点的距离便是根节点左子树的长度）
    long leftLength = rootInorder - startInorder;
    
    // 取得根节点左子树前序遍历的终点
    int* leftPreorderEnd = startPreorder + leftLength;

    // 如果左子树存在，则递归构建左子树
    if (leftLength > 0) {
        root->m_pLeft = constructCore(startPreorder + 1, leftPreorderEnd, startInorder, rootInorder - 1);
    }

    // 这里 endPreorder - startPreorder 得到的是左子树和右子树的总长度，如果大于左子树的长度，则表明一定存在右子树
    // 递归构建右子树
    if (leftLength < endPreorder - startPreorder) {
        root->m_pRight = constructCore(leftPreorderEnd + 1, endPreorder, rootInorder + 1, endInorder);
    }

    return root;
}

ConstructBinaryTree::BinaryTreeNode* ConstructBinaryTree::construct(int* preorder, int* inorder, int length) {
    if (preorder == nullptr || inorder == nullptr || length <= 0) {
        return nullptr;
    }
    
    // 重建二叉树
    return constructCore(preorder, preorder + length - 1, inorder, inorder + length - 1);
}

// 辅助函数

// 构建节点
ConstructBinaryTree::BinaryTreeNode* ConstructBinaryTree::CreateBinaryTreeNode(int value) {
    BinaryTreeNode* pNode = new BinaryTreeNode();
    pNode->m_nValue = value;
    pNode->m_pLeft = nullptr;
    pNode->m_pRight = nullptr;

    return pNode;
}

// 左右子节点赋值
void ConstructBinaryTree::ConnectTreeNodes(BinaryTreeNode* pParent, BinaryTreeNode* pLeft, BinaryTreeNode* pRight) {
    if(pParent != nullptr) {
        pParent->m_pLeft = pLeft;
        pParent->m_pRight = pRight;
    }
}

// 打印一个节点
void ConstructBinaryTree::PrintTreeNode(const BinaryTreeNode* pNode) {
    if(pNode != nullptr) {
        printf("value of this node is: %d\n", pNode->m_nValue);

        if(pNode->m_pLeft != nullptr)
            printf("value of its left child is: %d.\n", pNode->m_pLeft->m_nValue);
        else
            printf("left child is nullptr.\n");

        if(pNode->m_pRight != nullptr)
            printf("value of its right child is: %d.\n", pNode->m_pRight->m_nValue);
        else
            printf("right child is nullptr.\n");
    } else {
        printf("this node is nullptr.\n");
    }

    printf("\n");
}

// 前序打印二叉树
void ConstructBinaryTree::PrintTree(const BinaryTreeNode* pRoot) {
    PrintTreeNode(pRoot);

    if (pRoot != nullptr) {
        if(pRoot->m_pLeft != nullptr)
            PrintTree(pRoot->m_pLeft);

        if(pRoot->m_pRight != nullptr)
            PrintTree(pRoot->m_pRight);
    }
}

// 前序销毁二叉树
void ConstructBinaryTree::DestroyTree(BinaryTreeNode* pRoot) {
    if (pRoot != nullptr) {
        BinaryTreeNode* pLeft = pRoot->m_pLeft;
        BinaryTreeNode* pRight = pRoot->m_pRight;

        delete pRoot;
        pRoot = nullptr;

        DestroyTree(pLeft);
        DestroyTree(pRight);
    }
}
```
## 面试题 8:二叉树的下一个结点
&emsp;题目：给定一棵二叉树和其中的一个结点，如何找出中序遍历顺序的下一个结点？树中的结点除了有两个分别指向左右子结点的指针以外，还有一个指向父结点的指针。
```c++
namespace NextNodeInBinaryTrees {

struct BinaryTreeNode {
    int m_nValue;
    BinaryTreeNode* m_pParent;
    BinaryTreeNode* m_pLeft;
    BinaryTreeNode* m_pRight;
};

BinaryTreeNode* getNext(BinaryTreeNode* pNode);

}

NextNodeInBinaryTrees::BinaryTreeNode* NextNodeInBinaryTrees::getNext(BinaryTreeNode* pNode) {
    if (pNode == nullptr) {
        return nullptr;
    }
    
    // 分三种情况：
    BinaryTreeNode* pNext = nullptr;
    if (pNode->m_pRight != nullptr) {
        // 1. 一个节点的右节点不为 nullptr 时，即该节点存在右子树，那么它的下一个节点就是它的右子树的最左节点，
        // 如果它的右子节点是叶子节点的话，那么下一个节点就是它自己了，例如：左根右 （正对应了它这个右）
        
        BinaryTreeNode* pRight = pNode->m_pRight;
        
        // 一个向下循环，找到右子树的最左子节点
        while (pRight->m_pLeft != nullptr) {
            pRight = pRight->m_pLeft;
        }
        
        // 然后把找到的结果赋值给 pNext
        // （pNext 也有两种情况，1): 还是上面的 pNode->m_pRight 2): while 循环里面找到的一个 m_pLeft）
        pNext = pRight;
        
    } else if (pNode->m_pParent != nullptr) {
        // 2. 这里也分两种情况首先上面的 if 已经排除该节点是没有右子节点的，然后它可能的就是：
        // 1): 该节点是它父节点的左子节点，此时根据 "左根右" 可知下一个节点就是它的父节点
        // 2): 该节点是它父节点的右子节点，此时情况则比较复杂一点，依然是根据 "左根右"，此时我们向上找节点，
        //     它的直接父节点肯定已经使用过了，然后再往上找父节点，如果这个父节点是它自己父节点的左子节点，
        //     则这下一个节点就是这个父节点的父节点了。
        
        // pCurrent 记录当前节点
        BinaryTreeNode* pCurrent = pNode;
        // 记录当前节点的父节点
        BinaryTreeNode* pParent = pNode->m_pParent;
        
        // 如果父节点不为空，且该节点是父节点的左子节点，则不用进入 while 循环了，要找的下一个节点就是这个父节点，
        // 下面直接把 pParent 赋值给 pNext 返回就好了。
        while (pParent != nullptr && pCurrent == pParent->m_pRight) {
            // 如果进入了循环，就向上找第一个父节点是它父节点的左子节点的节点，如果找到了就会结束 while 循环
            
            // 更新 pCurrent 为父节点
            pCurrent = pParent;
            // 更新 pParent 节点为下一个父节点
            pParent = pParent->m_pParent;
        }
        
        // 这里
        pNext = pParent;
    }
    
    // 最后返回 pNext
    return pNext;
}
```
## 面试题 9:用两个栈实现队列
```c++

```
