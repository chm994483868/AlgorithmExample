//
//  Review_1.cpp
//  Algorithm_C
//
//  Created by HM C on 2020/7/11.
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
#include <cstdio>
#include <memory>

using namespace std;

namespace Review_1 {

// 01. 冒泡排序 O(n*n)
void bubbleSort(int nums[], int count) {
    int k = count - 1;
    for (int i = 0; i < count - 1; i++) {
        bool noExchange = true;
        int n = 0;
        for (int j = 0; j < k; j++) {
            if (nums[j] > nums[j + 1]) {
                swap(nums[j], nums[j + 1]);
                noExchange = false;
                n = j;
            }
        }
        
        if (noExchange) break;
        k = n;
    }
}

// 02. 插入排序 O(n*n)
void insertSort(int nums[], int count) {
    for (int i = 1; i < count; i++) {
        for (int j = i; j > 0 && nums[j - 1] > nums[j]; j--) {
            swap(nums[j - 1], nums[j]);
        }
    }
}

// 03. 选择排序 O(n*n)
void selectSort(int nums[], int count) {
    for (int i = 0; i < count; i++) {
        int minIndex = i;
        for (int j = i + 1; j < count; j++) {
            if (nums[j] < nums[minIndex]) {
                minIndex = j;
            }
        }
        swap(nums[i], nums[minIndex]);
    }
}

// 04. 希尔排序 O(n*n)
void shellSort(int nums[], int count) {
    for (int gap = count / 2; gap > 0; gap /= 2) {
        for (int i = 0; i < gap; i++) {
            for (int j = i + gap; j < count; j += gap) {
                for (int k = j; k > 0 && nums[k - gap] > nums[k]; k -= gap) {
                    swap(nums[k - gap], nums[k]);
                }
            }
        }
    }
}

// 05. 希尔排序优化 O(n*n)
void shellSortOptimization(int nums[], int count) {
    for (int gap = count / 2; gap > 0; gap /= 2) {
        for (int i = gap; i < count; i++) {
            for (int j = i; j > 0 && nums[j - gap] > nums[j]; j -= gap) {
                swap(nums[j - gap], nums[j]);
            }
        }
    }
}

// 06. 快速排序 O(n*logn)
void quickSort(int nums[], int l, int r) {
    if (l <= r) {
        return;
    }
    
    int i = l, j = r, x = nums[l];
    while (i < j) {
        while (i < j && nums[j] >= x) j--;
        if (i < j) nums[i++] = nums[j];
        
        while (i < j && nums[i] < x) i++;
        if (i < j) nums[j--] = nums[i];
    }
    
    nums[i] = x;
    quickSort(nums, l, i - 1);
    quickSort(nums, i + 1, r);
}

// 07. 归并排序 O(n*logn)
void mergeArray(int nums[], int first, int mid, int last, int temp[]);

void mergeSort(int nums[], int first, int last, int temp[]) {
    if (first <= last) {
        return;
    }
    
    int mid = (first + last) / 2;
    
    mergeSort(nums, first, mid, temp);
    mergeSort(nums, mid + 1, last, temp);
    
    mergeArray(nums, first, mid, last, temp);
}

void mergeArray(int nums[], int first, int mid, int last, int temp[]) {
    int i = first, j = mid + 1;
    int m = mid, n = last;
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
    
    for (i = 0; i < k; i++) {
        nums[first + i] = temp[i];
    }
}

// 08. 堆排序 O(n*logn)
void MaxHeapFixdown(int nums[], int i, int n);

void heapSort(int nums[], int count) {
    for (int i = ((count - 1) - 1) / 2; i >= 0; i--) {
        MaxHeapFixdown(nums, i, count);
    }
    
    for (int i = count - 1; i >= 1; i--) {
        swap(nums[i], nums[0]);
        MaxHeapFixdown(nums, 0, i);
    }
}

void MaxHeapFixdown(int nums[], int i, int n) {
    int j = i * 2 + 1;
    int temp = nums[i];
    
    while (j < n) {
        if (j + 1 < n && nums[j + 1] > nums[j]) {
            j++;
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
        swap(nums[i], nums[max]);
        heapify(nums, max, n);
    }
}

// 10. 反转二叉树（递归）
struct TreeNode {
    int m_nValue;
    TreeNode* m_pLeft;
    TreeNode* m_pRight;
    
    TreeNode(int x) : m_nValue(x), m_pLeft(nullptr), m_pRight(nullptr) {}
};

TreeNode* invertTree(TreeNode* root) {
    if (root == nullptr)
        return root;
    
    swap(root->m_pLeft, root->m_pRight);
    
    if (root->m_pLeft != nullptr) invertTree(root->m_pLeft);
    if (root->m_pRight != nullptr) invertTree(root->m_pRight);
    
    return root;
}

// 11. 反转二叉树（栈或者队列）
TreeNode* invertTreeWithStack(TreeNode* root) {
    if (root == nullptr)
        return root;
    
    stack<TreeNode*> treeStack;
    treeStack.push(root);
    
    while (!treeStack.empty()) {
        TreeNode* top = treeStack.top();
        treeStack.pop();
        
        swap(top->m_pLeft, top->m_pRight);
        
        if (top->m_pLeft != nullptr) treeStack.push(top->m_pLeft);
        if (top->m_pRight != nullptr) treeStack.push(top->m_pRight);
    }
    
    return root;
}

TreeNode* invertTreeWithQueue(TreeNode* root) {
    if (root == nullptr)
        return root;
    
    queue<TreeNode*> treeQueue;
    treeQueue.push(root);
    
    while (!treeQueue.empty()) {
        TreeNode* front = treeQueue.front();
        treeQueue.pop();
        
        swap(front->m_pLeft, front->m_pRight);
        
        if (front->m_pLeft != nullptr) treeQueue.push(front->m_pLeft);
        if (front->m_pRight != nullptr) treeQueue.push(front->m_pRight);
    }
    
    return root;
}

// 12. 找到链表中倒数第 k 个节点 O(n)
struct ListNode {
    int m_nValue;
    ListNode* m_pNext;
    
    ListNode(int x) : m_nValue(x), m_pNext(nullptr) {}
};

ListNode* findKthFromTail(ListNode* pListHead, int k) {
    if (pListHead == nullptr || k <= 0)
        return nullptr;
    
    ListNode* pBListNode = pListHead;
    for (int i = 0; i < k - 1; i++) {
        if (pBListNode == nullptr)
            return nullptr;
        
        pBListNode = pBListNode->m_pNext;
    }
    
    ListNode* pAListNode = pListHead;
    while (pBListNode->m_pNext != nullptr) {
        pBListNode = pBListNode->m_pNext;
        pAListNode = pAListNode->m_pNext;
    }
    
    return pAListNode;
}

// 13. 重写赋值运算符
class CMyString {
public:
    CMyString(char* m_pData = nullptr);
    CMyString(const CMyString& str);
    ~CMyString();
    
    CMyString& operator=(const CMyString& str) {
//        if (this == &str) {
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
        
        if (this != &str) {
            CMyString strTemp(str);
            
            char* pTemp = strTemp.m_pData;
            strTemp.m_pData = m_pData;
            m_pData = pTemp;
        }
        
        return *this;
    }
    
private:
    char* m_pData;
};

// 14. 找到数组中重复的数字 O(1) O(n)
bool duplicate(int numbers[], int length, int* duplication) {
    if (numbers == nullptr || length <= 0)
        return false;
    
    for (int i = 0; i < length; i++) {
        if (numbers[i] < 0 || numbers[i] > length - 1) {
            return false;
        }
    }
    
    for (int i = 0; i < length; i++) {
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
    
    for (int i = 0; i < numsSize; i++) {
        if (nums[i] < 0 || nums[i] > numsSize - 1) {
            return NULL;
        }
    }
    
    int a[100000];
    memset(a, 0, sizeof(a));
    
    for (int i = 0; i < numsSize; i++) {
        if (!a[nums[i]]) {
            a[nums[i]] = 1;
        } else {
            return nums[i];
        }
    }
    
    return NULL;
}

// 16. 不修改数组找出重复的数字 O(1) O(n*logn)
int countRange(const int* numbers, int length, int start, int end);
int getDuplication(const int* numbers, int length) {
    if (numbers == nullptr || length <= 0) {
        return -1;
    }
    
    int start = 1;
    int end = length - 1;
    
    while (start <= end) {
        int middle = ((end - start) >> 1) + start;
        
        int count = countRange(numbers, length, start, middle);
        
        if (start == end) {
            if (count == 1) {
                return start;
            } else {
                break;
            }
        }
        
        if (count > (middle - start + 1)) {
            end = middle;
        } else {
            start = middle + 1;
        }
    }
    
    return -1;
}

int countRange(const int* numbers, int length, int start, int end) {
    if (numbers == nullptr || length <= 0) {
        return 0;
    }
    
    int count = 0;
    for (int i = 0; i < length; i++) {
        if (numbers[i] >= start && numbers[i] <= end) {
            ++count;
        }
    }
    
    return count;
}

// 17. 二维数组中查找数字
bool Fine(int* matrix, int rows, int colums, int number) {
    bool found = false;
    // 从右上角开始
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
// length 为字符数组 string 的总容量
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
void AddToTail(ListNode** pHead, int value) {
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
void RemoveNode(ListNode** pHead, int value) {
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
void PrintListReversingly_Recursively(ListNode* pHead) {
    ListNode* pNode = pHead;
    if (pNode == nullptr) {
        
        if (pNode->m_pNext != nullptr) {
            PrintListReversingly_Recursively(pNode->m_pNext);
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
TreeNode* constructCore(int* startPreorder, int* endPreorder, int* startInorder, int* endInorder);
TreeNode* construct(int* preorder, int* inorder, int length) {
    if (preorder == nullptr || inorder == nullptr || length <= 0) {
        return nullptr;
    }
    
    return constructCore(preorder, preorder + length - 1, inorder, inorder + length - 1);
}

TreeNode* constructCore(int* startPreorder, int* endPreorder, int* startInorder, int* endInorder) {
    int rootValue = startPreorder[0];
    TreeNode* root = new TreeNode(rootValue);
    
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
        root->m_pLeft = constructCore(startPreorder + 1, leftPreorderEnd, startPreorder, rootInorder - 1);
    }
    
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

BinaryTreeNodeWithParent* getNext(BinaryTreeNodeWithParent* pNode) {
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
template <typename T>
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

template <typename T> void CQueue<T>::appendNodeToTail(const T& node) {
    stack1.push(node);
}

template <typename T> T CQueue<T>::deleteHead() {
    if (stack2.empty()) {
        while (!stack1.empty()) {
            T& data = stack1.top();
            stack1.pop();
            
            stack2.push(data);
        }
    }
    
    if (stack2.empty()) {
        throw std::exception(); // CQuque is empty
    }
    
    T head = stack2.top();
    stack2.pop();
    return head;
}

// 26. 用两个队列实现栈
// 27. 求 1 + 2 + ... + n 的值（递归和非递归）
int addFrom1ToN_Recursive(int n) {
    return n == 0 ? 0 : n + addFrom1ToN_Recursive(n - 1);
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
    int results[2] = {0, 1};
    if (n < 2) {
        return results[n];
    }
    
    long long fibNMinusOne = 1;
    long long fibNMinusTwo = 0;
    long long fibN = 0;
    
    for (unsigned int i = 2; i <= n; i++) {
        fibN = fibNMinusOne + fibNMinusTwo;
        
        fibNMinusTwo = fibNMinusOne;
        fibNMinusOne = fibN;
    }
    
    return fibN;
}

// 29. 对公司员工年龄排序（计数排序）。
void sortAges(int ages[], int length) {
    if (ages == nullptr || length <= 0) {
        return;
    }
    
    const int oldestAge = 99;
    int timesOfAge[oldestAge + 1];
    for (int i = 0; i <= oldestAge; ++i) {
        timesOfAge[i] = 0;
    }
    
    for (int i = 0 ; i < length; ++i) {
        int age = ages[i];
        
        if (age < 0 || age > oldestAge) {
            throw std::exception(); // 年龄超过限制，参数错误
        }
        
        ++timesOfAge[age];
    }
    
    int index = 0;
    for (int i = 0; i <= oldestAge; ++i) {
        while (timesOfAge[i] > 0) {
            ages[index] = i;
            
            --timesOfAge[i];
            ++index;
        }
    }
}

// 30. 旋转数组的最小数字。
int minInOrder(int* numbers, int index1, int index2);
int min(int* numbers, int length) {
    if (numbers == nullptr || length <= 0) {
        throw std::exception(); // 参数无效
    }
    
    int index1 = 0;
    int index2 = length - 1;
    int indexMid = index1;
    
    while (numbers[index1] >= numbers[index2]) {
        if (index2 - index1 == 1) {
            indexMid = index2;
            break;
        }
        
        indexMid = (index2 - index1) / 2 + index1;
        
        if (numbers[index1] == numbers[index2] && numbers[indexMid] == numbers[index1]) {
            return minInOrder(numbers, index1, index2);
        }
        
        if (numbers[index1] <= numbers[indexMid]) {
            index1 = indexMid;
        } else if (numbers[index2] >= numbers[indexMid]) {
            index2 = indexMid;
        }
    }
    
    return numbers[indexMid];
}

int minInOrder(int* numbers, int index1, int index2) {
    int result = numbers[index1];
    for (int i = index1 + 1; i <= index2; ++i) {
        if (numbers[i] < result) {
            result = numbers[i];
        }
    }
    return result;
}

// 31. 矩阵中的路径。
bool hasPathCore(const char* matrix, int rows, int cols, int row, int col, const char* str, int& pathLength, bool* visited);
bool hasPath(const char* matrix, int rows, int cols, const char* str) {
    if (matrix == nullptr || rows < 1 || cols < 1 || str == nullptr) {
        return false;
    }
    
    bool* visited = new bool[rows * cols];
    memset(visited, 0, rows * cols); // 置为 0
    
    int pathLength = 0;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            
            if(hasPathCore(matrix, rows, cols, row, col, str, pathLength, visited)) {
                return true;
            }
            
        }
    }
    
    delete [] visited;
    return false;
}

bool hasPathCore(const char* matrix, int rows, int cols, int row, int col, const char* str, int& pathLength, bool* visited) {
    if (str[pathLength] == '\0') {
        return true;
    }
    
    bool hasPath = false;
    if (row >= 0 && row < rows && col >= 0 && col < cols && matrix[row * cols + col] == str[pathLength] && !visited[row * cols + col]) {
        
        ++pathLength;
        visited[row * cols + col] = true;
        
        hasPath = hasPathCore(matrix, rows, cols, row - 1, col, str, pathLength, visited) || hasPathCore(matrix, rows, cols, row, col - 1, str, pathLength, visited) || hasPathCore(matrix, rows, cols, row + 1, col, str, pathLength, visited) || hasPathCore(matrix, rows, cols, row, col + 1, str, pathLength, visited);
        
        if (!hasPath) {
            --pathLength;
            visited[row * cols + col] = false;
        }
    }
    
    return hasPath;
}

// 32. 机器人的运动范围。
int movingCountCore(int threshold, int rows, int cols, int row, int col, bool* visited);
bool check(int threshold, int rows, int cols, int row, int col, bool* visited);
int getDigitSum(int number);

int movingCount(int threshold, int rows, int cols) {
    if (threshold < 0 || rows <= 0 || cols <= 0)
        return 0;
    
    bool* visited = new bool[rows * cols];
    for (int i = 0; i < rows * cols; ++i) {
        visited[i] = false;
    }
    
    int count = movingCountCore(threshold, rows, cols, 0, 0, visited);
    
    delete [] visited;
    return count;
}

int movingCountCore(int threshold, int rows, int cols, int row, int col, bool* visited) {
    int count = 0;
    if (check(threshold, rows, cols, row, col, visited)) {
        visited[row * cols + col] = true;
        
        count = 1 + movingCountCore(threshold, rows, cols, row - 1, col, visited) + movingCountCore(threshold, rows, cols, row, col - 1, visited) + movingCountCore(threshold, rows, cols, row + 1, col, visited) + movingCountCore(threshold, rows, cols, row, col + 1, visited);
    }
    
    return count;
}

bool check(int threshold, int rows, int cols, int row, int col, bool* visited) {
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

// 33. 剪绳子。
//int maxProductAfterCutting_solution1(int length) {
//    if (length < 2)
//        return 0;
//    if (length == 2)
//        return 1;
//    if (length == 3)
//        return 2;
//
//    int* products = new int[length + 1];
//    products[0] = 0;
//    products[1] = 1;
//    products[2] = 2;
//    products[3] = 3;
//
//    int max = 0;
//    for (int i = 4; i <= length; ++i) {
//
//        max = 0;
//        for (int j = 1; j <= i / 2; ++j) {
//            int product = products[j] * products[i - j];
//            if (max < product)
//                max = product;
//
//            products[i] = max;
//        }
//
//    }
//
//    max = products[length];
//    delete [] products;
//
//    return max;
//}

//int maxProductAfterCutting_solution2(int length) {
//    if (length < 2)
//        return 0;
//    if (length == 2)
//        return 1;
//    if (length == 3)
//        return 2;
//
//    int timesOf3 = length / 3;
//
//    if (length - timesOf3 * 3 == 1)
//        timesOf3 -= 1;
//
//    int timesOf2 = (length - timesOf3 * 3) / 2;
//
//    return (int) (pow(3, timesOf3)) * (int) (pow(2, timesOf2));
//}

// 34. 二进制中 1 的个数。
int ttt(int n) {
    int count = 0;
    while (n) {
        ++count;
        n = (n - 1) & n;
    }
    return n;
}

// 35. 求数值的整数次方，不得使用库函数，不需要考虑大数问题。
bool g_InvalidInput = false;
bool equal(double num1, double num2);
double powerWithUnsignedExponent(double base, unsigned int exponent);

double power(double base, int exponent) {
    g_InvalidInput = false;
    
    if (equal(base, 0.0) && exponent < 0) {
        g_InvalidInput = true;
        return 0.0;
    }
    
    unsigned int absExponent = (unsigned int)exponent;
    if (exponent < 0) {
        absExponent = (unsigned int)(-exponent);
    }
    
    double result = powerWithUnsignedExponent(base, absExponent);
    if (exponent < 0) {
        result = 1 / result;
    }
    
    return result;
}

double powerWithUnsignedExponent(double base, unsigned int exponent) {
    if (exponent == 0)
        return 1;
    if (exponent == 1)
        return base;
    
    int result = powerWithUnsignedExponent(base, exponent >> 1);
    
    result *= result;
    if ((exponent & 0x1) == 1) {
        result *= base;
    }
    
    return result;
}

bool equal(double num1, double num2) {
    if (num1 - num2 > -0.0000001 && num1 - num2 < 0.0000001) {
        return true;
    } else {
        return false;
    }
}

// 36. 打印从 1 到最大的 n 位数。
void PrintNumber(char* number);
bool Increment(char* number);
void Print1ToMaxOfNDigitsRecursively(char* number, int length, int index);

void Print1ToMaxOfNDigits_1(int n) {
    if (n <= 0)
        return;
    
    char* number = new char[n + 1];
#warning 小括号里面是 n，写成了 n - 1
    memset(number, '0', n);
    number[n] = '\0';
    
    while (!Increment(number)) {
        PrintNumber(number);
    }
    
#warning 这里 number 忘记了释放
    delete [] number;
}

bool Increment(char* number) {
    bool isOverflow = false;
    int nTakeOver = 0;
    unsigned long nLength = strlen(number);
    
    for (unsigned long i = nLength - 1; i >= 0; --i) {
        int nSum = number[i] - '0' + nTakeOver;
        
        if (i == nLength - 1) {
            ++nSum;
        }
        
        if (nSum >= 10) {
            if (i == 0) {
                isOverflow = true;
            } else {
                nSum -= 10;
                nTakeOver = 1;
                number[i] = '0' + nSum;
            }
        } else {
            number[i] = '0' + nSum;
            break;
        }
    }
    
    return isOverflow;
}

void PrintNumber(char* number) {
    bool isBeginning0 = true;
    unsigned long nLength = strlen(number);
    
    for (unsigned long i = 0; i < nLength; ++i) {
        
#warning if 里面的 != '0' 的判断写成了 == '0'
        if (isBeginning0 && number[i] != '0')
            isBeginning0 = false;
        
        if (!isBeginning0) {
            printf("%c", number[i]);
        }
    }
    
    printf("\t");
}

// 37. 删除链表的节点。
// 分三种情况：
// 1): 链表长度大于 1，且待删除节点位于中间，就是待删除节点的 n_pNext != nullptr
// 2): 链表长度为 1，且待删除节点就是头节点
// 3): 链表长度大于 1，且待删除节点是链表的尾节点，删除之前我们要遍历链表，找到待删除节点的前一个节点
void DeleteNode(ListNode** pListHead, ListNode* pToBeDeleted) {
    if (pListHead == nullptr || *pListHead == nullptr || pToBeDeleted == nullptr)
        return;
    
    if (pToBeDeleted->m_pNext != nullptr) {
        ListNode* pNext = pToBeDeleted->m_pNext;
        pToBeDeleted->m_nValue = pNext->m_nValue;
        pToBeDeleted->m_pNext = pNext->m_pNext;
        
        delete pNext;
        pNext = nullptr;
    } else if (*pListHead == pToBeDeleted) {
        delete pToBeDeleted;
        pToBeDeleted = nullptr;
        *pListHead = nullptr;
    } else {
        ListNode* pNode = *pListHead;
        while (pNode->m_pNext != pToBeDeleted) {
            pNode = pNode->m_pNext;
        }
        
        pNode->m_pNext = nullptr;
        delete pToBeDeleted;
        pToBeDeleted = nullptr;
    }
}

// 38. 删除链表中重复的节点。
// 分五种情况:
// 1): 重复的节点在链表头
// 2): 重复的节点在链表中间
// 3): 重复的节点在链表尾
// 4): 没有重复的节点
// 5): 整个链表都是重复的节点
void DeleteDuplication(ListNode** pHead) {
    if (pHead == nullptr || *pHead == nullptr)
        return;
    
    ListNode* pPreNode = nullptr;
    ListNode* pNode = *pHead;
    
    while (pNode != nullptr) {
        ListNode* pNext = pNode->m_pNext;
        bool needDelete = false;
        
        if (pNext != nullptr && pNode->m_nValue == pNext->m_nValue)
            needDelete = true;
        
        if (!needDelete) {
            pPreNode = pNode;
            pNode = pNode->m_pNext;
        } else {
            ListNode* pToBeDel = pNode;
            int value = pNode->m_nValue;
            
            while (pToBeDel != nullptr && pToBeDel->m_nValue == value) {
                pNext = pToBeDel->m_pNext;
                
                delete pToBeDel;
                pToBeDel = nullptr;
                
                pToBeDel = pNext;
            }
            
            if (pPreNode == nullptr)
                *pHead = pNext;
            else
                pPreNode->m_pNext = pNext;
            
            pNode = pNext;
        }
    }
}

// 39. 正则表达式匹配。
bool matchCore(const char* str, const char* pattern);
bool match(const char* str, const char* pattern) {
    if (str == nullptr || pattern == nullptr)
        return false;
    
    return matchCore(str, pattern);
}

bool matchCore(const char* str, const char* pattern) {
    if (*str == '\0' && *pattern == '\0')
        return false;
    
    if (*str != '\0' && *pattern == '\0')
        return false;
    
    if (*(pattern + 1) == '*') {
        if (*str == *pattern || (*pattern == '.' && *str != '\0')) {
            return matchCore(str + 1, pattern + 2) ||
            matchCore(str + 1, pattern) ||
            matchCore(str, pattern + 2);
        } else {
            return matchCore(str, pattern + 2);
        }
    }
    
    if (*str == *pattern || (*pattern == '.' && *str != '\0')) {
        return matchCore(str + 1, pattern + 1);
    }
    
    return false;
}

}
