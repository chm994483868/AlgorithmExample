//
//  Review_2.cpp
//  Algorithm_C
//
//  Created by HM C on 2020/7/12.
//  Copyright © 2020 CHM. All rights reserved.
//

#include <stdio.h>
#include <iostream>

#include <string.h>
#include <vector>
#include <stack>
#include <queue>
#include <list>
#include <algorithm>

using namespace std;

namespace Review_2 {

// 01. 冒泡排序 O(n*n)
// 每一趟循环比较相邻元素大小交换位置，每次把一个最大值或最小值放在数组一端
void bubbleSort(int nums[], int length) {
    int k = length - 1;
    for (int i = 0; i < length - 1; ++i) {
        bool noExchange = true;
        int n = 0;
        for (int j = 0; j < k; ++j) {
            if (nums[j] > nums[j + 1]) {
                swap(nums[j], nums[j + 1]);
                noExchange = false;
                n = j;
            }
        }
        noExchange = true;
        k = n;
    }
}

// 02. 插入排序 O(n*n)
// 开始时假设左边第一元素为已排序数组，然后每次从右边取一个元素插入左边的已排序数组
void insertSort(int nums[], int length) {
    for (int i = 1; i < length; ++i) {
        for (int j = i; j > 0 && nums[j - 1] > nums[j]; ++j) {
            swap(nums[j - 1], nums[j]);
        }
    }
}

// 03. 选择排序 O(n*n)
// 假设左边序列已排序，每次从数组中找到一个最小值放在左边已排序数组的最后
void selectSort(int nums[], int length) {
    for (int i = 0; i < length; ++i) {
        int minIndex = i;
        for (int j = i + 1; j < length; ++j) {
            if (nums[j] < nums[minIndex]) {
                minIndex = j;
            }
        }
        swap(nums[minIndex], nums[i]);
    }
}

// 04. 希尔排序 O(n*n)
// 优化插入排序
void shellSort(int nums[], int length) {
    for (int gap = length / 2; gap > 0; gap /= 2) {
        for (int i = 0; i < gap; ++i) {
            for (int j = i + gap; j < length; j += gap) {
                for (int k = j; k > 0 && nums[k - gap] > nums[k]; k -= gap) {
                    swap(nums[k - gap], nums[k]);
                }
            }
        }
    }
}

// 05. 希尔排序优化 O(n*n)
void shellSortOptimize(int nums[], int length) {
    for (int gap = length / 2; gap > 0; gap /= 2) {
        for (int i = gap; i < length; ++i) {
            for (int j = i; j > 0 && nums[j - gap] > nums[j]; j += gap) {
                swap(nums[j - gap], nums[j]);
            }
        }
    }
}

// 06. 快速排序 O(n*logn)
// 挖坑 + 分治法
void quickSort(int nums[], int left, int right) {
    if (left >= right) {
        return;
    }
    
    int i = left, j = right, x = nums[left];
    while (i < j) {
        while (i < j && nums[j] >= x) {
            --j;
        }
        if (i < j) {
            nums[i++] = nums[j];
        }
        while (i < j && nums[i] < x) {
            ++i;
        }
        if (i < j) {
            nums[j--] = nums[i];
        }
    }
    
    nums[i] = x;
    
    quickSort(nums, left, i - 1);
    quickSort(nums, i + 1, right);
}

// 07. 归并排序 O(n*logn)
// 先把无序数组拆开成一个个有序数组，然后再把它们按大小顺序合并起来
void mergeArray(int nums[], int left, int mid, int right, int temp[]) {
    int i = left, j = mid + 1;
    int m = mid, n = right;
    int k = 0;
    
    while (i <= m && j <= n) {
        if (nums[i] <= nums[j]) {
            temp[k++] = nums[i++];
        } else {
            temp[k++] = nums[j++];
        }
    }
    
    while (i <= m) {
        temp[k++] = nums[i++];
    }
    
    while (j <= n) {
        temp[k++] = nums[j++];
    }
    
    for (int i = 0; i < k; ++i) {
        nums[left + i] = temp[i];
    }
}

void mergeSort(int nums[], int left, int right, int temp[]) {
    if (left >= right) {
        return;
    }
    
    int mid = (left + right) / 2;
    
    mergeSort(nums, left, mid, temp);
    mergeSort(nums, mid + 1, right, temp);
    
    mergeArray(nums, left, mid, right, temp);
}

// 08. 堆排序 O(n*logn)
// 首先把数据堆化，（堆是一种特殊的二叉树，根节点总是大于或者小于左右子节点，而且如果只有一个节点时，总是左子节点不为空）
// 然后把根元素和数组最后一个元素交换位置，即把最大或者最小的元素置于数组末尾，然后把数组交换 0 位剩下的数据继续堆化
// 然后循环重复上面的步骤，知道 0 1 交换
void maxheapFixdown(int nums[], int i, int n) {
    int j = i * 2 + 1;
    int temp = nums[i];
    
    while (j < n) {
        if (j + 1 < n && nums[j + 1] > nums[j]) {
            ++j;
        }
        
        if (nums[j] <= temp) {
            break;
        }
        
        nums[i] = nums[j];
        i = j;
        j = i * 2 + 1;
    }
    
    nums[i] = temp;
}

void heapSort(int nums[], int length) {
    for (int i = ((length - 1) - 1) / 2; i >= 0; --i) {
        maxheapFixdown(nums, i, length);
    }
    
    for (int j = length - 1; j >= 1; --j) {
        swap(nums[0], nums[j]);
        maxheapFixdown(nums, 0, j);
    }
}

// 09. 堆排序（素燕）O(n*logn)
void heapify(int nums[], int i, int n) {
    if (i >= n) {
        return;
    }
    
    int c1 = i * 2 + 1;
    int c2 = i * 2 + 2;
    int max = i;
    
    if (c1 < n && nums[c1] > nums[max]) {
        max = c1;
    }
    
    if (c2 < n && nums[c2] > nums[max]) {
        max = c2;
    }
    
    if (max != i) {
        swap(nums[max], nums[i]);
        heapify(nums, max, n);
    }
}

// 10. 反转二叉树（递归）
struct BinaryTreeNode {
    int m_nValue;
    BinaryTreeNode* m_pLeft;
    BinaryTreeNode* m_pRight;
    
    BinaryTreeNode(int x) : m_nValue(x), m_pLeft(nullptr), m_pRight(nullptr) {}
};

BinaryTreeNode* invertTree(BinaryTreeNode* pRoot) {
    if (pRoot == nullptr) {
        return pRoot;
    }
    
    BinaryTreeNode* pTemp = pRoot->m_pLeft;
    pRoot->m_pLeft = pRoot->m_pRight;
    pRoot->m_pRight = pTemp;
    
    if (pRoot->m_pLeft != nullptr) {
        invertTree(pRoot->m_pLeft);
    }
    
    if (pRoot->m_pRight != nullptr) {
        invertTree(pRoot->m_pRight);
    }
    
    return pRoot;
}

// 11. 反转二叉树（栈或者队列）
BinaryTreeNode* invertTreeWithStack(BinaryTreeNode* pRoot) {
    if (pRoot == nullptr) {
        return pRoot;
    }
    
    std::stack<BinaryTreeNode*> nodes;
    nodes.push(pRoot);
    
    while (!nodes.empty()) {
        BinaryTreeNode* node = nodes.top();
        nodes.pop();
        
        BinaryTreeNode* temp = node->m_pLeft;
        node->m_pLeft = node->m_pRight;
        node->m_pRight = temp;
        
        if (node->m_pLeft != nullptr) {
            nodes.push(node->m_pLeft);
        }
        
        if (node->m_pRight != nullptr) {
            nodes.push(node->m_pRight);
        }
    }
    
    return pRoot;
}

// 12. 找到链表中倒数第 k 个节点 O(n)
struct ListNode {
    int m_nValue;
    ListNode* m_pNext;
    
    ListNode(int x) : m_nValue(x), m_pNext(nullptr) {}
};

ListNode* findKthFromTail(ListNode* pHead, int k) {
    if (pHead == nullptr || k <= 0) {
        return nullptr;
    }
    
    // 1. 第一个节点指针前进 k - 1 步
    ListNode* pAHead = pHead;
    for (int i = 0; i < k - 1; ++i) {
        if (pAHead->m_pNext == nullptr) {
            return nullptr;
        } else {
            pAHead = pAHead->m_pNext;
        }
    }
    
    // 2. 第二个节点指针从头节点开始，跟着第一个节点指针一起移动，当第一个节点到达尾节点时，第二个节点指针刚好到达倒数第 K 个节点
    ListNode* pBheind = pHead;
    while (pAHead->m_pNext != nullptr) {
        pAHead = pAHead->m_pNext;
        pBheind = pBheind->m_pNext;
    }
    
    return pBheind;
}

// 13. 重写赋值运算符
class CMyString {
public:
    CMyString(char* m_pData = nullptr);
    CMyString(const CMyString& str);
    ~CMyString();
    
    CMyString& operator=(const CMyString& str) {
//        if (&str == this) {
//            return *this;
//        }
//
//        delete [] m_pData;
//        m_pData = nullptr;
//
//        m_pData = new char[strlen(str.m_pData) + 1];
//        strcpy(m_pData, str.m_pData);
//
//        return *this;
        
        if (&str != this) {
            
            CMyString strTemp(str);
            
            char* temp = strTemp.m_pData;
            strTemp.m_pData = m_pData;
            m_pData = temp;
        }
        
        return *this;
    }
    
private:
    char* m_pData;
};

// 14. 找到数组中重复的数字 O(1) O(n)
bool duplicate(int numbers[], int length, int* duplication) {
    if (numbers == nullptr || length <= 0) {
        return false;
    }
    
    for (int i = 0; i < length; ++i) {
        if (numbers[i] < 0 || numbers[i] > length - 1) {
            return false;
        }
    }
    
    for (int i = 0; i < length; ++i) {
        while (numbers[i] != i) {
            if (numbers[i] == numbers[numbers[i]]) {
                *duplication = numbers[i];
                return true;
            } else {
                swap(numbers[i], numbers[numbers[i]]);
            }
        }
    }
    
    return false;
}

// 15. 找到数组中重复的数字 O(n) O(n)
int findRepeatNumber(int* nums, int numsSize) {
    if (nums == nullptr || numsSize <= 0) {
        return NULL;
    }
    
    for (int i = 0; i < numsSize; ++i) {
        if (nums[i] < 0 || nums[i] > numsSize - 1) {
            return false;
        }
    }
    
    int a[100000];
#warning 长度用 sizeof(a) 不是用 100000， 这是如果 int 占 4 个字节的话其实是 400000 长度，需要的的字节个数而不是数组长度
    memset(a, 0, sizeof(a));
    
    for (int i = 0; i < numsSize; ++i) {
        if (a[nums[i]] == 0) {
            a[nums[i]] = 1;
        } else {
            return nums[i];
        }
    }
    
    return NULL;
}

// 16. 不修改数组找出重复的数字 O(1) O(n*logn)
int countRange(const int* numbers, int length, int start, int end) {
    if (numbers == nullptr || length <= 0) {
        return 0;
    }
    
    int count = 0;
    for (int i = 0; i < length; ++i) {
        if (numbers[i] >= start && numbers[i] <= end) {
            ++count;
        }
    }
    
    return count;
}

int getDuplication(const int* numbers, int length) {
    if (numbers == nullptr || length <= 0) {
        return NULL;
    }
    
    int start = 1;
    int end = length - 1;
    
    while (start <= end) {
        // 找到 start 和 end 中间值
        int middle = ((end - start) >> 1) + start;
        
        // 统计指定范围内数字个数
        int count = countRange(numbers, length, start, middle);
        
        // 如果 length 是 2, start 是 1 end 也是 1，则符合下面的情况
        if (start == end) {
            if (count == 1) {
                return start;
            } else {
                break;;
            }
        }
        
        // 调整 end 或者 star 的值
        if (count > (middle - start + 1)) {
            end = middle;
        } else {
            start = middle + 1;
        }
    }
    
    return NULL;
}

// 17. 二维数组中查找数字
bool find(int* matrix, int rows, int colums, int number) {
    bool found = false;
    int row = 0;
    int colum = colums - 1;
    
    while (matrix != nullptr && row < rows && colum >= 0) {
        if (matrix[row * colums + colum] == number) {
            found = true;
            return found;
        } else if (matrix[row * colums + colum] > number) {
            --colum;
        } else {
            ++row;
        }
    }
    
    return found;
}

// 18. 替换字符串中的空格 O(n)
void ReplaceBlank(char string[], int length) {
    if (string == nullptr || length <= 0) {
        return;
    }
    
    int originalLength = 0;
    int numberOfBlank = 0;
    int i = 0;
    while (string[i] != '\0') {
        ++originalLength;
        if (string[i] == ' ') {
            ++numberOfBlank;
        }
        
        ++i;
    }
    
    int newLength = originalLength + numberOfBlank * 2;
    if (newLength > length) {
        return;
    }
    
    int indexOfOriginal = originalLength;
    int indexOfNew = newLength;
    
    while (indexOfOriginal >= 0 && indexOfNew > indexOfOriginal) {
        if (string[indexOfOriginal] == ' ') {
            string[indexOfNew--] = '0';
            string[indexOfNew--] = '2';
            string[indexOfNew--] = '%';
        } else {
            string[indexOfNew--] = string[indexOfOriginal];
        }
        
        --indexOfOriginal;
    }
}

// 19. 往链表的末尾添加一个结点
void addToTail(ListNode** pHead, int value) {
    ListNode* pNew = new ListNode(value);
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

// 20. 从链表中找到第一个含有某值的节点并删除该节点
void removeNode(ListNode** pHead, int value) {
    if (pHead == nullptr || *pHead == nullptr) {
        return;
    }
    
    ListNode* pToBeDeleted = nullptr;
    if ((*pHead)->m_nValue == value) {
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

// 21. 从尾到头打印链表（递归）（递归在本质上就是一个栈结构）
void printListReversingly_Recursively(ListNode* pHead) {
    ListNode* pNode = pHead;
    if (pNode != nullptr) {
        
        if (pNode->m_pNext != nullptr) {
            printListReversingly_Recursively(pNode->m_pNext);
        }
        
        printf("%d\t", pNode->m_nValue);
    }
}

// 22. 从尾到头打印链表（非递归）
void PrintListReversingly_Iteratively(ListNode* pHead) {
    if (pHead == nullptr) {
        return;
    }
    
    stack<ListNode*> nodeStack;
    ListNode* pNode = pHead;
    while (pNode != nullptr) {
        nodeStack.push(pNode);
        pNode = pNode->m_pNext;
    }
    
    while (!nodeStack.empty()) {
        pNode = nodeStack.top();
        printf("%d\t", pNode->m_nValue);
        nodeStack.pop();
    }
}

// 23. 根据二叉树前序遍历和中序遍历的结果，重建该二叉树
BinaryTreeNode* constructCore(int* startPreorder, int* endPreorder, int* startInorder, int* endInorder);
BinaryTreeNode* construct(int* preorder, int* inorder, int length) {
    if (preorder == nullptr || inorder == nullptr || length <= 0) {
        return nullptr;
    }
    
    return constructCore(preorder, preorder + length - 1, inorder, inorder + length - 1);
}

BinaryTreeNode* constructCore(int* startPreorder, int* endPreorder, int* startInorder, int* endInorder) {
    int rootValue = startPreorder[0];
    BinaryTreeNode* root = new BinaryTreeNode(rootValue);
    
    if (startPreorder == endPreorder) {
        if (startInorder == endInorder && (*startPreorder) == (*startInorder)) {
            return root;
        } else {
            throw std::exception(); // Invalid input.
        }
    }
    
    int* rootInorder = startInorder;
    while (rootInorder <= endInorder && (*rootInorder) != rootValue) {
        ++rootInorder;
    }
    
    if (rootInorder == endInorder && (*rootInorder) != rootValue) {
        throw std::exception(); // Invalid input.
    }
    
    long leftLength = rootInorder - startInorder;
    int* leftPreorderEnd = startPreorder + leftLength;
    if (leftLength > 0) {
        root->m_pLeft = constructCore(startPreorder + 1, leftPreorderEnd, startPreorder, <#int *endInorder#>)
    }
    
    // endPreorder - startPreorder 的值是左右子树的节点个数之和，如果大于 leftLength 表示存在右子树
    if (leftLength < endPreorder - startPreorder) {
        root->m_pRight = constructCore(leftPreorderEnd + 1, endPreorder, rootInorder + 1, endInorder);
    }
    
    return root;
}

// 24. 二叉树的下一个节点
struct BinaryTreeNodeWithParent {
    int m_nValue;
    BinaryTreeNodeWithParent* m_pLeft;
    BinaryTreeNodeWithParent* m_pRight;
    BinaryTreeNodeWithParent* m_pParent;
};

BinaryTreeNodeWithParent* getNext(BinaryTreeNodeWithParent *pNode) {
    if (pNode == nullptr) {
        return nullptr;
    }
    
    BinaryTreeNodeWithParent* pNext = nullptr;
    if (pNode->m_pRight != nullptr) {
        BinaryTreeNodeWithParent* pRight = pNode->m_pRight;
        while (pRight->m_pLeft != nullptr) {
            pRight = pRight->m_pLeft;
        }
        
        pNext = pRight;
    } else if (pNode->m_pParent != nullptr) {
        BinaryTreeNodeWithParent* pCurrent = pNode;
        BinaryTreeNodeWithParent* pParent = pNode->m_pParent;
        
        while (pParent != nullptr && pCurrent == pParent->m_pRight) {
            pCurrent = pParent;
            pParent = pParent->m_pParent;
        }
        
        pNext = pParent;
    }
    
    return pNext;
}

// 25. 用两个栈实现队列
template<typename T>
class CQueue {
public:
    CQueue(void);
    ~CQueue(void);
    
    void appendNodeToTail(const T& node);
    T deleteHead();
    
private:
    stack<T> stack1;
    stack<T> stack2;
};

template<typename T> void CQueue<T>::appendNodeToTail(const T& node) {
    stack1.push(node);
}

template<typename T> T CQueue<T>::deleteHead() {
    if (stack2.empty()) {
        while (!stack1.empty()) {
            T& data = stack1.top();
            stack1.pop();
            
            stack2.push(data);
        }
    }
    
    if (stack2.empty()) {
        throw std::exception(); // CQueuq is empty
    }
    
    T head = stack2.top();
    stack2.pop();
    
    return head;
}

// 26. 用两个队列实现栈
// 27. 求 1 + 2 + ... + n 的值（递归和非递归）
int addFrom1ToN_Recursive(int n) {
    return n == 0? 0 : n + addFrom1ToN_Recursive(n - 1);
}

int addFrom1ToN_Iterative(int n) {
    int result = 0;
    for (int i = 1; i <= n; ++i) {
        result += i;
    }
    
    return result;
}

// 28. 斐波那契数列（递归和非递归）
long long fibonacci_Recursive(unsigned int n) {
    if (n <= 0) {
        return 0;
    }
    
    if (n == 1) {
        return 1;
    }
    
    return fibonacci_Recursive(n - 1) + fibonacci_Recursive(n - 2);
}

long long fibonacci_Iterative(unsigned int n) {
    int result[2] = {0, 1};
    if (n < 2) {
        return result[n];
    }
    
    long long fibNMinusOne = 1;
    long long fibNMinusTwo = 0;
    long long fibN = 0;
    
    for (unsigned int i = 2; i <= n; ++i) {
        fibN = fibNMinusOne + fibNMinusTwo;
        
        fibNMinusTwo = fibNMinusOne;
        fibNMinusOne = fibN;
    }
    
    return fibN;
}

// 29. 旋转数组的最小数字。
// 30. 对公司员工年龄排序（计数排序）。
// 31. 矩阵中的路径。
// 32. 机器人的运动范围。
// 33. 剪绳子。
// 34. 二进制中 1 的个数。
// 35. 求数值的整数次方，不得使用库函数，不需要考虑大数问题。
// 36. 打印从 1 到最大的 n 位数。
// 37. 删除链表的节点。
// 38. 删除链表中重复的节点。
// 39. 正则表达式匹配。
// 40. 表示数值的字符串。
// 41. 调整数组顺序使奇数位于偶数前面。
// 42. 链表中倒数第 k 个节点。
// 43. 链表中环的入口节点。
// 44. 反转链表。
// 45. 合并两个排序的链表。
// 46. 树的子结构。
// 47. 二叉树的镜像（反转二叉树）。
// 48. 对称的二叉树。
// 49. 顺时针打印矩阵。
// 50. 包含 min 函数的栈。
// 51. 栈的压入、弹出序列。
// 52. 从上到下打印二叉树。
// 53. 分行从上到下打印二叉树。
// 54. 之字形打印二叉树。
// 55. 二叉搜索树的后序遍历序列。
// 56. 二叉树中和为某一值的路径。

}
