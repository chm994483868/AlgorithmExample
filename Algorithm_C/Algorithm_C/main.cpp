//
//  main.cpp
//  Algorithm_C
//
//  Created by CHM on 2020/7/3.
//  Copyright © 2020 CHM. All rights reserved.
//

#include <iostream>

#include <stdio.h>
#include <string.h>
#include <vector>
#include <stack>
#include <queue>
#include <list>
#include <algorithm>

using namespace std;

void printArray(string desc, int nums[], int count) {
    std::cout << desc;
    for (int i = 0; i < count; i++) {
        std::cout << " " << nums[i] << " ";
    }
    
    std::cout << "\n";
}

// 冒泡排序
void bubbleSort(int nums[], int count) {
    int k = count - 1;
    for (int i = 0; i < count - 1; i++) {
        bool noExchange = true; // 用于记录本次循环是否发生交换
        int n = 0; // 用于记录本次循环最后一次发生交换时下标
        for (int j = 0; j < k; j++) {
            if (nums[j] > nums[j + 1]) {
                swap(nums[j], nums[j + 1]);
                noExchange = false;
                n = j;
            }
        }
        if (noExchange) { break; }
        k = n;
    }
}

// 插入排序
void insertSort(int nums[], int count) {
    for (int i = 1; i < count; i++) { // i 从 1 开始，0 定为左侧已排序数组第一个元素
        for (int j = i; j > 0 && nums[j - 1] > nums[j]; j--) {
            swap(nums[j - 1], nums[j]);
        }
    }
}

// 选择排序
void selectSort(int nums[], int count) {
    for (int i = 0; i < count; i++) {
        int minIndex = i; // 找最小元素
        for (int j = i + 1; j < count; j++) {
            if (nums[j] < nums[minIndex]) {
                minIndex = j;
            }
        }
        swap(nums[i], nums[minIndex]);
    }
}

// 希尔排序
void shellSort(int nums[], int count) {
    for (int gap = count / 2; gap > 0; gap /= 2) { // 分组
        for (int i = 0; i < gap; i++) { // 对本次分组里的各组进行排序
            for (int j = i + gap; j < count; j += gap) {
                for (int k = j; k > 0 && nums[k - gap] > nums[k]; k -= gap) {
                    swap(nums[k - gap], nums[k]);
                }
            }
        }
    }
}

// 希尔排序 优化
void shellSortOptimize(int nums[], int count) {
    for (int gap = count / 2; gap > 0; gap /= 2) {
        for (int i = gap; i < count; i++) { // 优化
            for (int j = i; j > 0 && nums[j - gap] > nums[j]; j -= gap) {
                swap(nums[j - gap], nums[j]);
            }
        }
    }
}

// 快速排序
// O(N*logN)
void quickSort(int nums[], int l, int r) {
    if (l >= r) {
        return;
    }
    
    int i = l, j = r, x = nums[l];
    
    while (i < j) {
        while (i < j && nums[j] >= x) j--; // 右侧挖坑
        if (i < j) nums[i++] = nums[j];
        
        while (i < j && nums[i] < x) i++; // 左侧挖坑
        if (i < j) nums[j--] = nums[i];
    }
    
    nums[i] = x;
    
    quickSort(nums, l, i - 1); // 分治
    quickSort(nums, i + 1, r);
}

// 将有两个有序数列 a[first...mid] 和 a[mid...last] 合并
void mergeArray(int a[], int first, int mid, int last, int temp[]) {
    int i = first, j = mid + 1;
    int m = mid, n = last;
    int k = 0;
    
    while (i <= m && j <= n) {
        if (a[i] <= a[j]) {
            temp[k++] = a[i++];
        } else {
            temp[k++] = a[j++];
        }
    }
    
    while (i <= m) {
        temp[k++] = a[i++];
    }
    
    while (j <= n) {
        temp[k++] = a[j++];
    }
    
    for (i = 0; i < k; i++) {
        a[first + i] = temp[i];
    }
}

// 归并排序
void mergeSort(int a[], int first, int last, int temp[]) {
    if (first >= last) {
        return;
    }
    
    int mid = (first + last) / 2;
    
    mergeSort(a, first, mid, temp);
    mergeSort(a, mid + 1, last, temp); // 递归拆
    
    mergeArray(a, first, mid, last, temp); // 递归结束开始合并
}

void MinHeapFixup(int a[], int i) {
    int j = (i - 1) / 2, temp = a[i];
    
    while (j >= 0 && i != 0) {
        if (a[j] <= temp) break;
        
        a[i] = a[j];
        i = j;
        j = (i - 1) / 2;
    }

    a[i] = temp;
}

void MinHeapAddNumber(int a[], int n, int nNum) {
    a[n] = nNum;
    MinHeapFixup(a, n);
}

void MinHeapFixdown(int a[], int i, int n) {
    int j = 2 * i + 1, temp = a[i]; // 找到左子节点

    while (j < n) {
        if (j + 1 < n && a[j + 1] > a[j]) { // 找到左右子节点中的最大值
            j++;
        }
        
        if (a[j] <= temp) {
            break;
        }
        
        a[i] = a[j];
        i = j;
        j = 2 * i + 1;
    }
    
    a[i] = temp;
}

// 堆排序
void heapSort(int nums[], int n) {
    for (int i = (n - 1 - 1) / 2; i >= 0; i--) {
        MinHeapFixdown(nums, i, n); // 建堆
    }
    
    for (int j = n - 1; j >= 1; j--) { // 排序
        swap(nums[j], nums[0]);
        MinHeapFixdown(nums, 0, j);
    }
}

// 堆化数组
void heapify(int tree[], int n, int i) {
    if (i >= n) {
        return;
    }
    
    int c1 = 2 * i + 1; // 左子节点
    int c2 = 2 * i + 2; // 右子节点
    int max = i;
    
    if (c1 < n && tree[c1] > tree[max]) {
        max = c1;
    }
    
    if (c2 < n && tree[c2] > tree[max]) {
        max = c2;
    }
    
    if (max != i) {
        swap(tree[max], tree[i]);
        heapify(tree, n, max);
    }
}

// 堆排序(素燕)
void heap_sort(int tree[], int n) {
    for (int i = ((n - 1) - 1) / 2; i >= 0; i--) {
        heapify(tree, n, i);
    }
    
    for (int i = n - 1; i >= 1; i--) {
        swap(tree[i], tree[0]);
        heapify(tree, i, 0);
    }
}

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
    
    for (unsigned int i = 2; i <= n; ++i) { // i 从 2 开始的
        fibN = fibNMinusOne + fibNMinusTwo;
        
        fibNMinusTwo = fibNMinusOne;
        fibNMinusOne = fibN;
    }
    
    return fibN;
}

void sortAges(int ages[], int length) {
    if (ages == nullptr || length <= 0) {
        return;
    }
    
    const int oldestAge = 99;
    int timesOfAge[oldestAge + 1];
    
    for (int i = 0; i <= oldestAge; ++i) {
        timesOfAge[i] = 0;
    }
    
    for (int i = 0; i < length; ++i) {
        int age = ages[i];
        if (age < 0 || age > oldestAge) {
            throw std::exception(); // age out of ranges.
        }
        
        ++timesOfAge[age];
    }
    
    int index = 0;
    for (int i = 0; i <= oldestAge; ++i) {
        while (timesOfAge[i] > 0) {
            ages[index] = i;
            ++index;
            
            --timesOfAge[i];
        }
    }
}

int main(int argc, const char * argv[]) {
    // insert code here...
//    std::cout << "Hello, World!\n";
    
    int nums[] = {4, 6, 3, 2, 1, 8, 20, 14};
    printArray("排序前:", nums, 8);
    
    // 冒泡排序
//    bubbleSort(nums, 8);
    // 插入排序
//    insertSort(nums, 8);
    // 选择排序
//    selectSort(nums, 8);
    // 希尔排序
//    shellSort(nums, 8);
    // 希尔排序优化
//    shellSortOptimize(nums, 8);
    // 快速排序
//    quickSort(nums, 0, 7);
    // 归并排序
//    int temp[] = {};
//    mergeSort(nums, 0, 7, temp);
    // 堆排序
//    heapSort(nums, 8);
    heap_sort(nums, 8);
    
    printArray("排序后:", nums, 8);
    
    return 0;
}
