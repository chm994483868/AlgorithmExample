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
void bubbleSort(int nums[], int count) {
    int k = count - 1;
    for (int i = 0; i < count - 1; ++i) {
        bool noExchange = true;
        int n = 0;
        for (int j = 0; j < k; ++j) {
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
    for (int i = 1; i < count; ++i) {
        for (int j = i; j > 0 && nums[j - 1] > nums[j]; --j) {
            swap(nums[j - 1], nums[j]);
        }
    }
}

// 03. 选择排序 O(n*n)
void selectSort(int nums[], int count) {
    for (int i = 0; i < count; ++i) {
        int minIndex = i;
        for (int j = i + 1; j < count; ++j) {
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
        for (int i = 0; i < gap; ++i) {
            for (int j = i + gap; j < count; j += gap) {
                for (int k = j; k > 0 && nums[k - gap] > nums[k]; k -= gap) {
                    swap(nums[k - gap], nums[k]);
                }
            }
        }
    }
}

// 05. 希尔排序优化 O(n*n)
void shellSortOptimize(int nums[], int count) {
    for (int gap = count / 2; gap > 0; gap /= 2) {
        for (int i = gap; i < count; ++i) {
            for (int j = i; j > 0 && nums[j - gap] > nums[j]; j -= gap) {
                swap(nums[j - gap], nums[j]);
            }
        }
    }
}

// 06. 快速排序 O(n*logn)
void quickSort(int nums[], int l, int r) {
    if (l >= r)
        return;
    
    int i = l, j = r, x = nums[l];
    
    while (i < j) {
        while (i < j && nums[j] >= x) --j;
        if (i < j) nums[i++] = nums[j];
        
        while (i < j && nums[i] < x) ++i;
        if (i < j) nums[j--] = nums[i];
    }
    
    nums[i] = x;
    quickSort(nums, l, i - 1);
    quickSort(nums, i + 1, r);
}

// 07. 归并排序 O(n*logn)
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
    
    for (int i = 0; i < k; ++i) {
        a[first + i] = temp[i];
    }
}

void mergeSort(int a[], int first, int last, int temp[]) {
    if (first >= last)
        return;
    
    int mid = (first + last) / 2;
    
    mergeSort(a, first, mid, temp);
    mergeSort(a, mid + 1, last, temp);
    
    mergeArray(a, first, mid, last, temp);
}

// 08. 堆排序 O(n*logn)
void MaxHeapFixdown(int nums[], int i, int n) {
    int j = i * 2 + 1;
    int temp = nums[i];
    
    while (j < n) {
        if (j + 1 < n && nums[j + 1] > nums[j])
            ++j;
        
        if (nums[j] <= temp)
            break;
        
        nums[i] = nums[j];
        i = j;
        j = i * 2 + 1;
    }
    
    nums[i] = temp;
}

void heapSort(int nums[], int n) {
    for (int i = (n - 1 - 1) / 2; i >= 0; --i) {
        MaxHeapFixdown(nums, i, n);
    }
    
    for (int i = n - 1; i >= 1; --i) {
        swap(nums[i], nums[0]);
        MaxHeapFixdown(nums, 0, i);
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
        swap(nums[i], nums[max]);
        heapify(nums, max, n);
    }
}

// 10. 反转二叉树（递归）
// 11. 反转二叉树（栈或者队列）
// 12. 找到链表中倒数第 k 个节点 O(n)
// 13. 重写赋值运算符
// 14. 找到数组中重复的数字 O(1) O(n)
// 15. 找到数组中重复的数字 O(n) O(n)
// 16. 不修改数组找出重复的数字 O(1) O(n*logn)
// 17. 二维数组中查找数字
// 18. 替换字符串中的空格 O(n)
// 19. 往链表的末尾添加一个结点
// 20. 从链表中找到第一个含有某值的节点并删除该节点
// 21. 从尾到头打印链表（递归）（递归在本质上就是一个栈结构）
// 22. 从尾到头打印链表（非递归）
// 23. 根据二叉树前序遍历和中序遍历的结果，重建该二叉树
// 24. 二叉树的下一个节点
// 25. 用两个栈实现队列
// 26. 用两个队列实现栈
// 27. 求 1 + 2 + ... + n 的值（递归和非递归）
// 28. 斐波那契数列（递归和非递归）
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

}
