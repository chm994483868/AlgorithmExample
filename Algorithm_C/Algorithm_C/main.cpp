//
//  main.cpp
//  Algorithm_C
//
//  Created by CHM on 2020/7/3.
//  Copyright © 2020 CHM. All rights reserved.
//
//Could not connect to development server.
//
//Ensure the following:
//- Node server is running and available on the same network - run 'npm start' from react-native root
//- Node server URL is correctly set in AppDelegate
//- WiFi is enabled and connected to the same network as the Node Server
//
//URL: http://localhost:8081/index.bundle?platform=ios&dev=true&minify=false
//
//RCTFatal
//__28-[RCTCxxBridge handleError:]_block_invoke
//_dispatch_call_block_and_release
//_dispatch_client_callout
//_dispatch_main_queue_callback_4CF
//__CFRUNLOOP_IS_SERVICING_THE_MAIN_DISPATCH_QUEUE__
//__CFRunLoopRun
//CFRunLoopRunSpecific
//GSEventRunModal
//-[UIApplication _run]
//UIApplicationMain
//main
//start
//0x0

#include <iostream>
#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <stack>
#include <queue>
#include <list>
#include <algorithm>
#include <cmath>
#include <exception>

using namespace std;

class TreeNode {
public:
    int val;
    TreeNode *left;
    TreeNode *right;

    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
};

// 反转二叉树（递归）
TreeNode* invertTree(TreeNode* root) {
    if (root == nullptr) return root;
    
    swap(root->left, root->right);
    
    if (root->left != nullptr) invertTree(root->left);
    if (root->right != nullptr) invertTree(root->right);
    
    return root;
}

// 反转二叉树（栈）
TreeNode* invertTree2(TreeNode* root) {
    if (root == nullptr) return root;
    // 创建栈，并把根节点入栈
    stack<TreeNode*> treeStack;
    treeStack.push(root);
    
    while (!treeStack.empty()) {
        // 取出栈顶元素，交换左右子节点
        TreeNode* top = treeStack.top();
        treeStack.pop();
        swap(top->left, top->right);
        
        // 左右子节点入栈
        if (top->left != nullptr) treeStack.push(top->left);
        if (top->right != nullptr) treeStack.push(top->right);
    }
    
    return root;
}

// 反转二叉树（队列）
TreeNode* invertTree3(TreeNode* root) {
    if (root == nullptr) return root;
    // 创建队列，并把根节点入栈
    queue<TreeNode*> treeQueue;
    treeQueue.push(root);
    
    while (!treeQueue.empty()) {
        TreeNode* front = treeQueue.front();
        treeQueue.pop();
        
        swap(front->left, front->right);
        
        if (front->left != nullptr) treeQueue.push(front->left);
        if (front->right != nullptr) treeQueue.push(front->right);
    }
    
    return root;
}

class ListNode {
public:
    int val;
    ListNode *next;
    
    ListNode(int x) : val(x), next(nullptr) {}
};

// 找到链表中倒数第 k 个节点
ListNode* findKthFromTail(ListNode* pListHead, int k) {
    if (pListHead == nullptr || k == 0) return pListHead;
    
    // 从头节点开始前进 k - 1 步
    ListNode* pBListHead = pListHead;
    for (int i = 0; i < k - 1; i++) {
        // 这里要判断 k 是否大于链表总长度
        if (pBListHead == nullptr) { return nullptr; }
        
        pBListHead = pBListHead->next;
    }
    
    ListNode* pAListNode = pListHead;
    // 这里 while 的条件是 pBListHead->next 不是空。保证第一个指针是指到最后一个节点，而不是指到最后一个节点之外
    while (pBListHead->next != nullptr) {
        pBListHead = pBListHead->next;
        pAListNode = pAListNode->next;
    }
    
    return pAListNode;
}

// 青蛙跳台阶
unsigned long frog(int n) {
    // 递归：太慢，下面是用迭代优化
    // int results[3] = {0, 1, 2};
    // if (n < 3) {
    //    return results[n];
    // }
    //
    // return frog(n - 1) + frog(n - 2);
    
    if (n < 2) return 1;
    
    // 这里要注意一下，和斐波那契数列的微小区别
    // 斐波那契数列如下:
    // n: 0 1 2 3 4 5 6 7  8  9
    //    0 1 1 2 3 5 8 13 21 34...
    // 青蛙跳台阶如下:
    // n: 0 1 2 3 4 5 6 7 8 9 10
    //    0 1 2 3 5 8 13
    // 它们之间刚好错开了一位
    int a = 1, b = 1;
    for (int i = 2; i <= n; i++) { // i 从 2 开始的
        int temp = (a + b) % 1000000007;
        a = b;
        b = temp;
    }
    
    return b;
}

// 重写赋值运算符
class CMyString {
public:
    CMyString(char* pData = nullptr);
    CMyString(const CMyString& str);
    ~CMyString(void);
    
    CMyString& operator=(const CMyString& str) {
        // 初级实现：
        // 注意这里的是: this == &str this 前面没有 *, str 前面加了 &
//        if (this == &str) return *this;
//
//        delete [] m_pData; // 释放原始字符内存空间，注意这里使用的是 delete [], 添加了中括号
//        m_pData = nullptr;
//
//        m_pData = new char[strlen(str.m_pData) + 1]; // 开辟空间，复制 m_pData
//        strcpy(m_pData, str.m_pData);
//
//        return *this;
        
        // 考虑安全异常的进阶实现:
        if (this != &str) {
            CMyString strTemp(str); // 先使用复制构造函数创建 strTemp
            
            char* pTemp = strTemp.m_pData; // 取出 m_pData
            strTemp.m_pData = m_pData; // 用这个临时局部变量释放原实例内存
            m_pData = pTemp; // 赋值
        }
        
        return *this;
    }
    
private:
    char* m_pData;
};

// 找到数组中重复的数字 O(1) O(n)
bool duplicate(int numbers[], int length, int* duplication) {
    if (numbers == nullptr || length <= 0) {
        return false;
    }
    
    // 判断数组中是否有数字不符合数值范围
    for (int i = 0; i < length; i++) {
        if (numbers[i] < 0 || numbers[i] > length - 1) {
            return false;
        }
    }
    
    // 开始循环
    for (int i = 0; i < length; i++) {
        while (numbers[i] != i) {
            // 如果有重复
            if (numbers[i] == numbers[numbers[i]]) {
                *duplication = numbers[i];
                return true;
            }
            
            // 交换位置
            int temp = numbers[i];
            numbers[i] = numbers[temp];
            numbers[temp] = temp;
        }
    }
    
    return false;
}

// 找到数组中重复的数字 O(n) O(n)
int findRepeatNumber(int* nums, int numsSize) {
    if (numsSize <= 0 || nums == nullptr) {
        return NULL;
    }
    
    // 判断数组中是否有数字不符合数值范围
    for (int i = 0; i < numsSize; i++) {
        if (nums[i] < 0 || nums[i] > numsSize - 1) {
            return NULL;
        }
    }
    
    // 准备一个辅助数组
    int a[100000];
    // 辅助数组元素全部置为 0
    memset(a, 0, sizeof(a));
    
    for (int i = 0; i < numsSize; i++) {
        // 根据当前值直接找到对应下标的值，看是否是 0 或者 1，如果是 0 则修改为 1，如果是 1 则表示前面这个数字已经出现过了，即发现了重复数字
        if (!a[nums[i]]) {
            a[nums[i]] = 1;
        } else {
            return nums[i];
        }
    }
    
    return NULL;
}

int countRange(const int* numbers, int length, int start, int end) {
    if (numbers == nullptr)
        return 0;
    
    int count = 0;
    for (int i = 0; i < length; i++) {
        if (numbers[i] >= start && numbers[i] <= end)
            count++;
    }
    
    return count;
}

// 不修改数组找出重复的数字 O(n) O(n*logn)
int getDuplication(const int* numbers, int length) {
    // 判断参数是否符合规则
    if (numbers == nullptr || length <= 0) {
        return -1;
    }
    
    // 数组中的数值从 1 开始到 length - 1
    int start = 1;
    int end = length - 1;
    
    while (start <= end) {
        // 先找到中间值
        int middle = ((end - start) >> 1) + start;
        
        // 统计数组中的元素在给定的数字范围中出现的次数
        int count = countRange(numbers, length, start, middle);
        
        // 如是数字范围开始和结束是同一个数字，并且它出现的次数大于 1 则表示用重复，
        if (start == end) {
            if (count > 1) {
                return start;
            } else {
                break;
            }
        }
        
        // 更新 start 或者 end
        if (count > (middle - start + 1)) { // 说明重复在前半部分
            end = middle;
        } else {
            start = middle + 1;
        }
    }
    
    return -1;
}

// 在二维数组中查找数字
bool Fine(int* matrix, int rows, int colums, int number) {
    // 记录标志是否找到
    bool found = false;
    // 从右上角开始
    int row = 0;
    int colum = colums - 1;
    
    // 行不能超过总行数，列数大于等于 0
    while (matrix != nullptr && row < rows && colum >= 0) {
        // 如果相等则表示找到了
        if (matrix[row * colums + colum] == number) {
            found = true;
            break;
        } else if (matrix[row * colums + colum] > number) { // 如果是大于表示要减小列数
            --colum;
        } else { // 如果是小于则表示要增加行数
            ++row;
        }
    }
    
    return found;
}

// 替换空格
// length 为字符数组 string 的总容量
void ReplaceBlank(char string[], int length) {
    // 首先判断参数是否符合规则
    if (string == nullptr || length <= 0)
        return;
    
    // originalLength 为字符串 string 的实际长度
    int originalLength = 0; // 记录原始字符串的实际字符数
    int numberOfBlank = 0; // 记录空格数
    // 这里用了一个 i 从字符串中根据 i 下标来取出对应的字符
    int i = 0;
    
    // 分别统计它们的值
    while (string[i] != '\0') {
        // 统计实际字符个数
        ++originalLength;
        // 统计空格个数
        if (string[i] == ' ') {
            ++numberOfBlank;
        }
        // 递进 i 的值
        ++i;
    }
    
    // newLength 为把空格替换成 '%20' 之后的长度
    int newLength = originalLength + numberOfBlank * 2; // 原始长度加上空格数乘以 2
    // 判断增长以后是否超出了字符串的空间长度，如果是超出了则防止其他内存被改写，直接返回，结束函数的执行
    if (newLength > length) {
        return;
    }
    
    // 记录 p1 指针位置
    int indexOfOriginal = originalLength;
    // 记录 p2 指针位置
    // 注意这里的 p1 和 p2 指针都是倒序向前的
    int indexOfNew = newLength;
    
    // p1 大于等于 0，p2 指针小于 p1
    while (indexOfOriginal >= 0 && indexOfNew > indexOfOriginal) {
        // p2 指针就不同了，当遇到空格时，要连续移动三步
        if (string[indexOfOriginal] == ' ') {
            string[indexOfNew--] = '0';
            string[indexOfNew--] = '2';
            string[indexOfNew--] = '%';
        } else {
            // 如果不是空格就正常一步一步往前走
            string[indexOfNew--] = string[indexOfOriginal];
        }
        
        // p1 指针就正常什么都不做就一步一步往前走
        --indexOfOriginal;
    }
}

// 往链表的末尾添加一个结点
struct ListNode_struct {
    int m_nValue;
    ListNode_struct* m_pNext;
};

// 参数 pHead 是一个指向指针的指针。当我们往一个空链表中插入一个节点时，
// 新插入的节点就是链表的头指针，由于此时会改动头指针，因此必须把 pHead 参数设为指向
// 指针的指针，否则出了这个函数 pHead 仍然是一个空指针
void AddToTail(ListNode_struct** pHead, int value) {
    // 创建新节点
    ListNode_struct* pNew = new ListNode_struct();
    pNew->m_nValue = value;
    pNew->m_pNext = nullptr;
    
    if (*pHead == nullptr) {
        // 如果是空链表，则新节点为新的头节点
        *pHead = pNew;
    } else {
        // 如果不是空链表，则首先移动到链表尾部
        ListNode_struct* pNode = *pHead;
        // 这里 while 里的判断条件是 pNode->m_pNext 不是 nullptr
        while (pNode->m_pNext != nullptr) {
            pNode = pNode->m_pNext;
        }
        
        // 把新节点放在链表的尾部
        pNode->m_pNext = pNew;
    }
}

// 从链表中找到第一个含有某值的节点并删除该节点
void RemoveNode(ListNode_struct** pHead, int value) {
    // 如果传入了一个空指针或者传入的指针指向空，则可以直接 return 了
    if (pHead == nullptr || *pHead == nullptr) {
        return;
    }
    
    // 这个节点是为了删除要删除的节点
    ListNode_struct* pToBeDeleted = nullptr;
    
    if ((*pHead)->m_nValue == value) {
        // 如果头节点就是要删除的节点
        pToBeDeleted = *pHead; // 记录要删除的节点
        *pHead = (*pHead)->m_pNext; // 删除
    } else {
        // 遍历寻找第一个与 value 值相等的节点
        ListNode_struct* pNode = *pHead;
        while (pNode->m_pNext != nullptr && pNode->m_pNext->m_nValue != value) {
            pNode = pNode->m_pNext;
        }
        
        // 如果找到相等则删除该节点
        if (pNode->m_pNext != nullptr && pNode->m_pNext->m_nValue == value) {
            pToBeDeleted = pNode->m_pNext; // 记录要删除的节点
            
            pNode->m_pNext = pNode->m_pNext->m_pNext; // 删除
        }
    }
    
    if (pToBeDeleted != nullptr) { // 释放内存
        delete pToBeDeleted;
        pToBeDeleted = nullptr;
    }
}

// 从尾到头打印链表（非递归）
void PrintListReversingly_Iteratively(ListNode_struct* pHead) {
    if (pHead == nullptr)
        return;
    
    // 放在栈里面
    std::stack<ListNode_struct*> nodes;
    
    ListNode_struct* pNode = pHead;
    while (pNode != nullptr) {
        nodes.push(pNode);
        pNode = pNode->m_pNext;
    }
    
    // 循环出栈打印
    while (!nodes.empty()) {
        pNode = nodes.top();
        printf("%d\t", pNode->m_nValue);
        nodes.pop();
    }
}

// 从尾到头打印链表（递归）（递归在本质上就是一个栈结构）
void PrintListReversingly_Recursively(ListNode_struct* pHead) {
    if (pHead != nullptr) {
        
        if (pHead->m_pNext != nullptr) {
            // 递归
            PrintListReversingly_Recursively(pHead->m_pNext);
        }
        
        // 打印
        printf("%d\t", pHead->m_nValue);
    }
}

//             10
//          /      \
//         6        14
//       /   \     /   \
//      4     8   12   16
//
// 前序(根左右): 10, 6, 4, 8, 14, 12, 16
// 中序(左根右): 4, 6, 8, 10, 12, 14, 16
// 后序(左右根): 4, 8, 6, 12, 16, 14, 10
// 
// 根据二叉树前序遍历和中序遍历的结果，重建该二叉树
struct BinaryTreeNode {
    int m_nValue;
    BinaryTreeNode* m_pLeft;
    BinaryTreeNode* m_pRight;
};

BinaryTreeNode* ConstructCore(int* startPreorder, int* endPreorder, int* startInorder, int* endInorder);

BinaryTreeNode* Construct(int* preorder, int* inorder, int length) {
    if (preorder == nullptr || inorder == nullptr || length <= 0) {
        return nullptr;
    }
    
    return ConstructCore(preorder, preorder + length - 1, inorder, inorder + length - 1);
}

BinaryTreeNode* ConstructCore(int* startPreorder, int* endPreorder, int* startInorder, int* endInorder) {
    // 前序遍历序列的第一个数字是根节点的值
    int rootValue = startPreorder[0];
    // 创建根节点
    BinaryTreeNode* root = new BinaryTreeNode();
    root->m_nValue = rootValue;
    root->m_pLeft = root->m_pRight = nullptr;
    
    // 如果前序和中序都是一个数字并且都是相同的，则表示这棵树只有一个根节点，可以直接返回了。
    if (startPreorder == endPreorder) {
        if (startInorder == endInorder && *startPreorder == *startInorder) {
            return root;
        } else {
            throw std::exception(); // Invalid input.
        }
    }
    
    // 在中序遍历序列中找到根节点的值
    int* rootInorder = startInorder;
    while (rootInorder <= endInorder && *rootInorder != rootValue) {
        ++rootInorder;
    }
    
    // 判断如果找根节点直接找到了中序的最后一个值，是否是和 rootValue 相等的，如果不等，则表示原始入参有问题，直接返回
    if (rootInorder == endInorder && *rootInorder != rootValue) {
        throw std::exception(); // Invalid input.
    }
    
    // 左子树长度
    long leftLength = rootInorder - startInorder;
    // 左子树前序终点
    int* leftPreorderEnd = startPreorder + leftLength;
    // 递归构建左子树
    if (leftLength > 0) {
        // 构建左子树
        root->m_pLeft = ConstructCore(startPreorder + 1, leftPreorderEnd, startInorder, rootInorder - 1);
    }
    
    // 如果前序起点和终点的距离大于左子树，则表明还有右子树，则递归构建右子树
    if (leftLength < endPreorder - startPreorder) {
        // 构建右子树
        root->m_pRight = ConstructCore(leftPreorderEnd + 1, endPreorder, rootInorder + 1, endInorder);
    }
    
    return root;
}

// 二叉树的下一个节点
struct BinaryTreeNodeWithParent {
    int m_nValue;
    BinaryTreeNodeWithParent* m_pLeft;
    BinaryTreeNodeWithParent* m_pRight;
    BinaryTreeNodeWithParent* m_pParent;
};

BinaryTreeNodeWithParent* getNext(BinaryTreeNodeWithParent* pNode) {
    if (pNode == nullptr) {
        return nullptr;
    }
    
    BinaryTreeNodeWithParent* pNext = nullptr;
    if (pNode->m_pRight != nullptr) {
        // 如果该节点的右子节点不为空，那么它的下一个节点就是它的右子节点的最左子节点，
        // 如果这个右子节点不存在左子节点，则 pNode 的下一个节点就是该右节点了
        BinaryTreeNodeWithParent* pRight = pNode->m_pRight;
        while (pRight->m_pLeft != nullptr) {
            pRight = pRight->m_pLeft;
        }
        
        pNext = pRight;
    } else if (pNode->m_pParent != nullptr) {
        BinaryTreeNodeWithParent* pCurrent = pNode;
        // 如果一个节点的右子节点为空，并且它是自己父节点的左子节点，那么它的下一个节点就是它的父节点
        BinaryTreeNodeWithParent* pParent = pNode->m_pParent;
        while (pParent != nullptr && pCurrent == pParent->m_pRight) { // 如果它是自己父节点的右子节点
            pCurrent = pParent;
            pParent = pParent->m_pParent; // 从父节点一直倒序上去
        }
        
        pNext = pParent;
    }
    
    return pNext;
}

// 用两个栈实现队列
template <typename T>
class CQueue {
public:
    CQueue(void);
    ~CQueue(void);
    
    void appendTail(const T& node);
    T deleteHead();
private:
    stack<T> stack1;
    stack<T> stack2;
};

template <typename T> CQueue<T>::CQueue(void) {
    
}

template <typename T> CQueue<T>::~CQueue(void) {
    
}

template <typename T> void CQueue<T>::appendTail(const T& element) {
    stack1.push(element); // 直接入栈 stack1
}

template <typename T> T CQueue<T>::deleteHead() {
    if (stack2.size() <= 0) {
        while (stack1.size() > 0) {
            // 入栈，这里用的是 T&
            T& data = stack1.top();
            stack1.pop();
            stack2.push(data);
        }
    }

    if (stack2.size() == 0) {
        throw new std::exception();
    }

    T head = stack2.top(); // 这里用的是 T
    stack2.pop();

    return head;
}

// 递归和非递归的方式求 1 + 2 + 3 + ... + n 的值。
int addFrom1ToN_Recursive(int n) {
    return n == 0 ? 0 : n + addFrom1ToN_Recursive(n - 1);
}

int addFrom1ToN_Iterative(int n) {
    int result = 0;
    for(int i = 1; i <= n; ++i) { // 循环加 i
        result += i;
    }
    
    return result;
}

// 斐波那契数列
// 递归
long long fibonacci_Recursive(unsigned int n) {
    if (n <= 0) {
        return 0;
    }
    
    if (n == 1) {
        return 1;
    }
    
    return fibonacci_Recursive(n - 1) + fibonacci_Recursive(n - 2);
}

// 非递归
long long fibonacci_Iterative(unsigned int n) {
    int result[2] = {0, 1};
    if (n < 2) {
        return result[n];
    }
    
    long long fibNMinusOne = 1;
    long long fibNMinusTwo = 0;
    long long fibN = 0;
    
    for (unsigned int i = 2; i <= n; ++i) { // i 从 2 开始的
        fibN = fibNMinusOne + fibNMinusTwo;
        
        fibNMinusTwo = fibNMinusOne;
        fibNMinusOne = fibN;
    }
    
    return fibN;
}

// 对员工年龄排序（计数排序）
void sortAges(int ages[], int length) {
    // 1.参数判断
    if (ages == nullptr || length <= 0) {
        return;
    }
    
    // 2.辅助数组，数组下标对应年龄，下标对应的值表示该年龄的员工数量
    const int oldestAge = 99;
    int timesOfAge[oldestAge + 1];
    for (int i = 0; i <= oldestAge; ++i) {
        timesOfAge[i] = 0;
    }
    
    // 3. 遍历数组，统计不同的年龄
    for (int i = 0; i < length; ++i) {
        int age = ages[i];
        
        // 这里加一个年龄越界的判断
        if (age < 0 || age > oldestAge) {
            throw std::exception(); // age out of range
        }
        
        ++timesOfAge[age];
    }
    
    // 4. 取出统计数组里面的值
    int index = 0;
    for (int i = 0; i <= oldestAge; ++i) {
        while (timesOfAge[i] > 0) {
            ages[index] = i;
            ++index;
            --timesOfAge[i];
        }
    }
}

// 旋转数组的最小数字
int minInOrder(int* numbers, int index1, int index2);
int min(int* numbers, int length) {
    // 判断参数是否有效
    if (numbers == nullptr || length <= 0) {
        throw new std::exception(); // Invalid parameters
    }
    
    // 左侧
    int index1 = 0;
    // 右侧
    int index2 = length - 1;
    // 中间值
    // 默认从 index1 开始，没有初始设置为: indexMid = (index1 + index2) / 2
    // 放在了下面的循环里面的更新 indexMid 的值
    
    // 如果把排序数组的前面的 0 个元素搬到最后面，即排序数组本身，这仍然是数组的一个旋转
    // 我们的代码需要支持这种情况，此时数组第一个数字就是最小数字，可以直接返回，而这就是把
    // indexMid 初始化设置为 index1 的原因
    int indexMid = index1;
    
    while (numbers[index1] >= numbers[index2]) {
        // 如果 index1 和 index2 指向相邻的两个数，
        // 则 index1 指向第一个递增子数组的最后一个数字，
        // index2 指向第二个子数组的第一个数字，也就是数组中的最小数字
        if (index2 - index1 == 1) {
            indexMid = index2;
            break;
        }
        
        // 如果下标为 index1、index2 和 indexMid 指向的三个数字相等，
        // 则只能顺序查找
        indexMid = (index1 + index2) / 2;
        if (numbers[index1] == numbers[index2] && numbers[indexMid] == numbers[index1]) {
            return minInOrder(numbers, index1, index2);
        }
        
        // 缩小查找范围
        // 比较更新: index1 左移 或者 index2 右移
        if (numbers[indexMid] >= numbers[index1]) {
            index1 = indexMid;
        } else if (numbers[indexMid] <= numbers[index2]) {
            index2 = indexMid;
        }
    }
    
    return numbers[indexMid];
}

int minInOrder(int* numbers, int index1, int index2) {
    int result = numbers[index1];
    for (int i = index1 + 1; i <= index2; ++i) {
        if (result > numbers[i]) {
            result = numbers[i];
        }
    }
    return result;
}

// 矩阵中的路径
bool hasPathCore(const char* matrix, int rows, int cols, int row, int col, const char* str, int& pathLength, bool* visited);
bool hasPath(const char* matrix, int rows, int cols, const char* str) {
    // 第一步判断参数是否正确
    // 1. 参数判断
    if (matrix == nullptr || rows < 1 || cols < 1 || str == nullptr) {
        return false;
    }
    // 第二步准备下个函数的入参
    // 这里 bool 矩阵创建重点记一下
    // 2. 创建一个 bool 数组来记录矩阵中对应位置的字母是否已经使用过了
    bool* visited = new bool[rows * cols];
    memset(visited, 0, rows * cols);
    
    // 3. 嵌套循环
    int pathLength = 0; // 记录当前位置
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            
            if (hasPathCore(matrix, rows, cols, row, col, str, pathLength, visited)) {
                return true;
            }
            
        }
    }
    
    delete [] visited;
    return false;
}

bool hasPathCore(const char* matrix, int rows, int cols, int row, int col, const char* str, int& pathLength, bool* visited) {
    // 如果 pathLength 到了字符串末尾，说明已经找到对应路径，可以结束函数执行了！
    if (str[pathLength] == '\0') {
        return true;
    }
    
    bool hasPath = false;
    
    // 判断当前这个指定路径是否能进行判断
    // 这里要判断 row 和 col 大于 0 并且小于最大值，因为下面有个 -1 操作，它们的值可能小于 0
    // 这里要判断 matrix[row * cols + col] == str[pathLength] 确定前一个路径节点包含字符
    // 三是判断 !visited[row + cols + col] 路径节点是第一次经过
    if (row >= 0 && row < rows && col >= 0 && col < cols && matrix[row * cols + col] == str[pathLength] && !visited[row + cols + col]) {
        // 先往前走一步
        ++pathLength;
        // 标记该路径已经被走过
        visited[row * cols + col] = true;
        
        hasPath = hasPathCore(matrix, rows, cols, row, col - 1, str, pathLength, visited) || hasPathCore(matrix, rows, cols, row - 1, col, str, pathLength, visited) || hasPathCore(matrix, rows, cols, row, col + 1, str, pathLength, visited) || hasPathCore(matrix, rows, cols, row + 1, col, str, pathLength, visited);
        
        // 如果经过上面 4 个或，没有正确结果，则倒上去
        if (!hasPath) {
            // 倒退一步
            // 这个 --pathLength 正是回溯法的精髓
            // 倒退一步
            --pathLength;
            // 该路径点置为 false
            visited[row * cols + col] = false;
        }
    }

    return hasPath;
}

// 机器人的运动范围
int movingCountCore(int threshold, int rows, int cols, int row, int col, bool* visited);
bool check(int threshold, int rows, int cols, int row, int col, bool* visited);
int getDigitSum(int number);

int movingCount(int threshold, int rows, int cols) {
    // 参数判断
    if (threshold < 0 || rows <= 0 || cols <= 0) {
        return 0;
    }

    // 创建标记数组，并初始全部设置为 false
    bool* visited = new bool[rows * cols];
    for (int i = 0; i < rows * cols; ++i) {
        visited[i] = false;
    }

    // 计算 count，从 0 0 开始
    int count = movingCountCore(threshold, rows, cols, 0, 0, visited);

    // 释放数组内存
    delete [] visited;

    return count;
}

int movingCountCore(int threshold, int rows, int cols, int row, int col, bool* visited) {
    // 记录数值
    int count = 0;

    if (check(threshold, rows, cols, row, col, visited)) {
        // 标记置为 true，表示该点已经经过了
        visited[row * cols + col] = true;

        count = 1 + movingCountCore(threshold, rows, cols, row - 1, col, visited) + movingCountCore(threshold, rows, cols, row, col - 1, visited) + movingCountCore(threshold, rows, cols, row + 1, col, visited) +
        movingCountCore(threshold, rows, cols, row, col + 1, visited);
    }

    return count;
}

// 判断参数是否合规
bool check(int threshold, int rows, int cols, int row, int col, bool* visited) {
    // 行和列的范围，大于等于 0, 内部有一个 -1 操作，计算可能小于 0
    // 判断 row 和 col 数字之和是否小于等于 threshold
    // 判断数组标记是否为 false
    if (row >= 0 && row < rows && col >= 0 && col < cols && getDigitSum(row) + getDigitSum(col) <= threshold && !visited[row * cols + col]) {
        return true;
    }

    return false;
}

int getDigitSum(int number) {
    int sum = 0;
    while (number > 0) {
        sum += number % 10;
        number /= 10;
    }

    return sum;
}

// 剪绳子
int maxProductAfterCutting_solution1(int length) {
    if (length < 2)
        return 0;
    
    if (length == 2)
        return 1;
    
    if (length == 3)
        return 2;
    
    int* products = new int[length + 1];
    products[0] = 0;
    products[1] = 1;
    products[2] = 2;
    products[3] = 3;
    
    int max = 0;
    for(int i = 4; i <= length; ++i) {
        max = 0;
        for (int j = 1; j <= i / 2; ++j) {
            int product = products[j] * products[i - j];
            if (max < product) {
                max = product;
            }
            
            products[i] = max;
        }
    }
    
    max = products[length];
    delete [] products;
    
    return max;
}

int maxProductAfterCutting_solution2(int length) {
    if (length < 2) {
        return 0;
    }
    if (length == 2) {
        return 1;
    }
    if (length == 3) {
        return 2;
    }
    
    int timesOf3 = length / 3;
    
    if(length - timesOf3 * 3 == 1) {
        timesOf3 -= 1;
    }
    
    int timesOf2 = (length - timesOf3 * 3) / 2;
    
    return (int) (pow(3, timesOf3)) * (int) (pow(2, timesOf2));
}

// 二进制中 1 的个数
int numberOf1(int n) {
    int count = 0;
    while (n > 0) {
        if (n & 1) {
            ++count;
        }
        
        n = n >> 1;
    }
    
    return count;
}

int numberOf1_Two(int n) {
    int count = 0;
    unsigned int flag = 1;
    while (flag) {
        if (n & flag) {
            ++count;
        }
        flag = flag << 1;
    }
    
    return count;
}

int numberOf1_Three(int n) {
    int count = 0;
    while (n) {
        ++count;
        n = (n - 1) & n;
    }
    
    return count;
}

void rec(int N) {
    if (N > 0) {
        rec(N - 10);
        rec(N - 1);
    }
    
    cout << "N=" << N << endl;
    cout << "最后一句了" << endl;
}

void prologue(const BinaryTreeNode* pRoot) {
    if (pRoot == nullptr) {
        return;
    }
    
    // 前序
//    printf("%d\t", pRoot->m_nValue);
    
    if (pRoot->m_pLeft != nullptr) {
        prologue(pRoot->m_pLeft);
    }
    
    // 中序
//    printf("%d\t", pRoot->m_nValue);
    
    if (pRoot->m_pRight != nullptr) {
        prologue(pRoot->m_pRight);
    }
    
    // 后序
//    printf("%d\t", pRoot->m_nValue);
}

// 前序: 10 6 4 8 14 12 16
// 中序: 4 6 8 10 12 14 16
// 后序: 4 8 6 12 16 14 10

void fun(int n) {
    n--;
    
    if ( 0 == n ) { //跳出递归的条件
        printf("end\n");
        return ;//出栈
    }
 
    printf("fun --> %d\n" ,n);//fun函数的值
    fun(n);//fun1
    fun(n);//fun2
    //printf("fun --> %d\n" ,n);
}

void changeP(int* param) {
//    int b = 10;
//    param = &b;
//    (*param)++;
    *param = 10;
}

void swapint(int* a, int* b) {
    int temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

// 值传递
void change1(int n) {
    std::cout << "值传递--函数操作地址 " << &n << std::endl;
    n++;
}

// 引用传递
void change2(int& n) {
    std::cout << "引用传递--函数操作地址 " << &n << std::endl;
    n++;
}

// 指针传递
void change3(int* n) {
    std::cout << "指针传递--函数操作地址 " << n << std::endl;
    *n = *n + 1;
}

//    int a = 4;
//    std::cout << "a = " << a << std::endl;
//    changeP(&a);
//    std::cout << "a = " << a << std::endl;
    
//    int a = 1;
//    int b = 2;
//    std::cout << "a = " << a << std::endl;
//    std::cout << "b = " << b << std::endl;
//
//    swapint(&a, &b);
//
//    std::cout << "a = " << a << std::endl;
//    std::cout << "b = " << b << std::endl;
    
//    int n = 10;
//    std::cout << "实参的地址 " << &n << std::endl;
    
//    change1(n);
//    std::cout << "after change1() n = " << n << std::endl;
    
//    change2(n);
//    std::cout << "after change2() n = " << n << std::endl;
    
//    change3(&n);
//    std::cout << "after change3() n = " << n << std::endl;

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "🎉🎉🎉 Hello, World!\n";
    
    
    return 0;
}
