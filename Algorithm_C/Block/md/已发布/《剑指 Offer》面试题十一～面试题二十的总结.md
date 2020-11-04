# 《剑指 Offer》面试题十一～面试题二十的总结
> &emsp;上一篇是 1~10 题，本篇是 11~20 题。⛽️⛽️

## 面试题 11:旋转数组的最小数字
&emsp;题目：把一个数组最开始的若干个元素搬到数组的末尾，我们称之为数组的旋转。输入一个递增排序的数组的一个旋转，输出旋转数组的最小元素。例如数组 {3, 4, 5, 1, 2} 为 {1, 2, 3, 4, 5} 的一个旋转，该数组的最小值为 1 。
```c++
namespace MinNumberInRotatedArray {

// 开局相关题目：
// 快速排序
unsigned int randomInRange(unsigned int start, unsigned int end);
int partition(int data[], int length, int start, int end);
void swap(int* num1, int* num2);
void quickSort(int data[], int length, int start, int end);
// 员工年龄排序（计数排序）
void sortAges(int ages[], int length);

int minInorder(int* numbers, int index1, int index2);
int min(int* numbers, int length);

}

// 快速排序
unsigned int MinNumberInRotatedArray::randomInRange(unsigned int start, unsigned int end) {
    int rand = (random() % (end - start + 1)) + start;
    return rand;
}

int MinNumberInRotatedArray::partition(int data[], int length, int start, int end) {
    if (data == nullptr || length <= 0 || start < 0 || end >= length) {
        throw std::exception(); // 参数错误
    }
    
    int index = randomInRange(start, end);
    swap(&data[index], &data[end]);
    int small = start - 1;
    for (index = start; index < end; ++index) {
        if (data[index] < data[end]) {
            ++small;
            
            if (small != index) {
                swap(&data[small], &data[index]);
            }
        }
    }
    
    ++small;
    if (small != end) {
        swap(&data[small], &data[end]);
    }
    
    return small;
}

void MinNumberInRotatedArray::swap(int* num1, int* num2) {
    int temp = *num1;
    *num1 = *num2;
    *num2 = temp;
}

void MinNumberInRotatedArray::quickSort(int data[], int length, int start, int end) {
    if (start == end) {
        return;
    }
    
    int index = partition(data, length, start, end);
    if (index > start) {
        quickSort(data, length, start, index - 1);
    }
    
    if (index < end) {
        quickSort(data, length, index + 1, end);
    }
}

// 员工年龄排序（计数排序）
void MinNumberInRotatedArray::sortAges(int ages[], int length) {
    if (ages == nullptr || length <= 0) {
        return;
    }
    
    const int oldestAge = 99;
    int timesOfAge[oldestAge + 1];
    int i = 0;
    for (; i <= oldestAge; ++i) {
        timesOfAge[i] = 0;
    }
    
    for (i = 0; i < length; ++i) {
        int age = ages[i];
        if (age < 0 || age > oldestAge) {
            throw std::exception(); // 年龄超过范围
        }
        
        ++timesOfAge[age];
    }
    
    int index = 0;
    for (i = 0; i <= oldestAge; ++i) {
        int count = timesOfAge[i];
        while (count > 0) {
            ages[index] = i;
            ++index;
            --count;
        }
    }
}

int MinNumberInRotatedArray::minInorder(int* numbers, int index1, int index2) {
    int result = numbers[index1];
    for (int i = index1 + 1; i <= index2; ++i) {
        if (result > numbers[i]) {
            result = numbers[i];
        }
    }
    
    return result;
}

int MinNumberInRotatedArray::min(int* numbers, int length) {
    if (numbers == nullptr || length <= 0) {
        throw std::exception(); // 参数错误
    }
    
    int index1 = 0;
    int index2 = length - 1;
    int indexMid = index1;
    while (numbers[index1] >= numbers[index2]) {
        if (index2 - index1 == 1) {
            indexMid = index2;
            break;
        }
        
        indexMid = ((index2 - index1) >> 1) + index1;
        if (numbers[index1] == numbers[index2] && numbers[indexMid] == numbers[index1]) {
            return minInorder(numbers, index1, index2);
        }
        
        if (numbers[indexMid] >= numbers[index1]) {
            index1 = indexMid;
        } else if (numbers[indexMid] <= numbers[index2]) {
            index2 = indexMid;
        }
    }
    
    return numbers[indexMid];
}
```
## 面试题 12:矩阵中的路径
&emsp;题目：请设计一个函数，用来判断在一个矩阵中是否存在一条包含某字符串所有字符的路径。路径可以从矩阵中任意一格开始，每一步可以在矩阵中向左、右、上、下移动一格。如果一条路径经过了矩阵的某一格，那么该路径不能再次进入该格子。例如在下面的3×4的矩阵中包含一条字符串“bfce”的路径（路径中的字母用下划线标出）。但矩阵中不包含字符串“abfb”的路径，因为字符串的第一个字符b占据了矩阵中的第一行第二个格子之后，路径不能再次进入这个格子。
> A B T G
> C F C S
> J D E H
```c++
namespace StringPathInMatrix {

bool hasPathCore(const char* matrix, int rows, int cols, int row, int col, const char* str, int& pathLength, bool* visited);
bool hasPath(const char* matrix, int rows, int cols, const char* str);

}

bool StringPathInMatrix::hasPathCore(const char* matrix, int rows, int cols, int row, int col, const char* str, int& pathLength, bool* visited) {
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

bool StringPathInMatrix::hasPath(const char* matrix, int rows, int cols, const char* str) {
    if (matrix == nullptr || rows < 1 || cols < 1 || str == nullptr) {
        return false;
    }
    
    bool* visited = new bool[rows * cols];
    memset(visited, 0, rows * cols);
    int pathLength = 0;

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
```
## 面试题 13:机器人的运动范围
&emsp;题目：地上有一个 m 行 n 列的方格。一个机器人从坐标 (0, 0) 的格子开始移动，它每一次可以向左、右、上、下移动一格，但不能进入行坐标和列坐标的数位之和大于 k 的格子。例如，当 k 为 18 时，机器人能够进入方格 (35, 37)，因为 3+5+3+7=18。但它不能进入方格 (35, 38)，因为 3+5+3+8=19。请问该机器人能够到达多少个格子？
```c++
namespace RobotMove {

int movingCoungCore(int threshold, int rows, int cols, int row, int col, bool* visited);
bool check(int threshold, int rows, int cols, int row, int col, bool* visited);
int getDigitSum(int number);
int movingCount(int threshold, int rows, int cols);

}

int RobotMove::movingCoungCore(int threshold, int rows, int cols, int row, int col, bool* visited) {
    int count = 0;
    if (check(threshold, rows, cols, row, col, visited)) {
        visited[row * cols + col] = true;
        count = 1 + movingCoungCore(threshold, rows, cols, row - 1, col, visited) + movingCoungCore(threshold, rows, cols, row, col - 1, visited) + movingCoungCore(threshold, rows, cols, row + 1, col, visited) + movingCoungCore(threshold, rows, cols, row, col + 1, visited);
    }
    
    return count;
}

bool RobotMove::check(int threshold, int rows, int cols, int row, int col, bool* visited) {
    if (row >= 0 && row < rows && col >= 0 && col < cols && getDigitSum(row) + getDigitSum(col) <= threshold && !visited[row * cols + col]) {
        return true;
    } else {
        return false;
    }
}

int RobotMove::getDigitSum(int number) {
    int sum = 0;
    while (number > 0) {
        sum += number % 10;
        number /= 10;
    }
    
    return sum;
}

int RobotMove::movingCount(int threshold, int rows, int cols) {
    if (threshold < 0 || rows <= 0 || cols <= 0) {
        return 0;
    }
    
    bool* visited = new bool[rows * cols];
    for (int i = 0; i < rows * cols; ++i) {
        visited[i] = false;
    }
    
    int count = movingCoungCore(threshold, rows, cols, 0, 0, visited);
    
    delete [] visited;
    
    return count;
}

```
## 面试题 14:剪绳子
&emsp;题目：给你一根长度为 n 绳子，请把绳子剪成 m 段（ m、n 都是整数，n>1 并且 m≥1）。每段的绳子的长度记为 k[0]、k[1]、⋯⋯、k[m]。k[0] * k[1] * ⋯ *k[m] 可能的最大乘积是多少？例如当绳子的长度是 8 时，我们把它剪成长度分别为 2、3、3 的三段，此时得到最大的乘积 18。
```c++
namespace CuttingRope {

// 动态规划
int maxProductAfterCutting_solution1(int length);
// 贪婪算法
int maxProductAfterCutting_solution2(int length);

}

int CuttingRope::maxProductAfterCutting_solution1(int length) {
    if (length < 2) {
        return 0;
    }
    if (length == 2) {
        return 1;
    }
    if (length == 3) {
        return 2;
    }
    
    int* products = new int[length + 1];
    products[0] = 0;
    products[1] = 1;
    products[2] = 2;
    products[3] = 3;
    
    int max = 0;
    for (int i = 4; i <= length; ++i) {
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

int CuttingRope::maxProductAfterCutting_solution2(int length) {
    if (length < 2) {
        return 0;
    }
    if (length == 2) {
        return 1;
    }
    if (length == 3) {
        return 2;
    }
    
    // 尽可能多地减去长度为 3 的绳子段
    int timesOf3 = length / 3;
    
    // 当绳子最后剩下的长度为 4 的时候，不能再剪去长度为 3 的绳子段。
    // 此时更好的办法是把绳子剪成长度为 2 的两段，因为 2 * 2 > 3 * 1.
    if (length - timesOf3 * 3 == 1) {
        timesOf3 -= 1;
    }
    
    int timesOf2 = (length - timesOf3 * 3) / 2;
    
    return (int) (pow(3, timesOf3)) * (int) (pow(2, timesOf2));
}

```
## 面试题 15:二进制中1的个数
&emsp;题目：请实现一个函数，输入一个整数，输出该数二进制表示中 1 的个数。例如把 9 表示成二进制是 1001，有 2 位是 1。因此如果输入 9，该函数输出 2。
```c++
namespace NumberOf1InBinary {

// 相关题目:
// 用一条语句判断一个整数是不是 2 的整数次方。
// 一个整数如果是 2 的整数次方，那么它的二进制表示中有且只有一位是 1，而其它所有位都是 0。
// ((n - 1) & n) == 0? true: false;

// 输入两个整数 m 和 n，计算需要改变 m 的二进制表示中的多少位才能得到 n。
// 分两步，第一步求这两个数的异或；第二步统计异或结果中 1 的位数。

int numberOf1_Solution1(int n);
int numberOf1_Solution2(int n);

}

int NumberOf1InBinary::numberOf1_Solution1(int n) {
    int count = 0;
    unsigned int flag = 1;
    // 这里 flag 要向右移动 32 位才能结束 while 循环
    while (flag) {
        if (n & flag) {
            ++count;
        }
        
        flag = flag << 1;
    }
    
    return count;
}

int NumberOf1InBinary::numberOf1_Solution2(int n) {
    int count = 0;
    // 这里很巧妙，while 循环的次数就是 1 的个数
    while (n) {
        ++count;
        n = (n - 1) & n; // 减 1 后再做 与 操作，后面的 1 全部被去掉了...
    }
    
    return count;
}
```
## 面试题 16:数值的整数次方
&emsp;题目：实现函数 double Power(double base, int exponent)，求 base 的 exponent 次方。不得使用库函数，同时不需要考虑大数问题。
```c++
namespace CPower {

static bool g_InvalidInput = false;
bool equal(double num1, double num2);
double powerWithUnsignedExponent(double base, unsigned int exponent);

double power(double base, int exponent);

}

bool CPower::equal(double num1, double num2) {
    if ((num1 - num2 > -0.0000001) && (num1 - num2 < 0.0000001)) {
        return true;
    } else {
        return false;
    }
}

double CPower::powerWithUnsignedExponent(double base, unsigned int exponent) {
//    if (exponent == 0) {
//        return 1;
//    }
//
//    if (exponent == 1) {
//        return base;
//    }
//
//    double result = 1.0;
//    for (unsigned int i = 1; i <= exponent; ++i) {
//        result *= base;
//    }
//
//    return result;
    
    if (exponent == 0) {
        return 1;
    }

    if (exponent == 1) {
        return base;
    }

    double result = powerWithUnsignedExponent(base, exponent >> 1);
    result *= result;

    if ((exponent & 0x1) == 1) {
        result *= base;
    }

    return result;
}

double CPower::power(double base, int exponent) {
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
        result = 1.0 / result;
    }
    
    return result;
}

```
## 面试题 17:打印 1 到最大的 n 位数
&emsp;题目：输入数字 n，按顺序打印出从 1 最大的 n 位十进制数。比如输入 3，则打印出 1、2、3 一直到最大的 3 位数即 999。
```c++
namespace Print1ToMaxOfNDigits {

void printNumber(char* number);
bool increment(char* number);
void print1ToMaxOfNDigits_1(int n);

void print1ToMaxOfNDigitsRecursively(char* number, int length, int index);
void print1ToMaxOfNDigits_2(int n);

}

void Print1ToMaxOfNDigits::printNumber(char* number) {
    bool isBeginning0 = true;
    unsigned long nLength = strlen(number);
    unsigned long i = 0;
    for (; i < nLength; ++i) {
        if (isBeginning0 && number[i] != '0') {
            isBeginning0 = false;
        }
        
        if (!isBeginning0) {
            printf("%c", number[i]);
        }
    }
    
    printf("\t");
}

bool Print1ToMaxOfNDigits::increment(char* number) {
    bool isOverflow = false;
    int nTakeOver = 0;
    unsigned long nLength = strlen(number);
    unsigned long i = nLength - 1;
    
    for (; i >= 0; --i) {
        int sum = number[i] - '0' + nTakeOver;
        if (i == nLength - 1) {
            ++sum;
        }
        
        if (sum >= 10) {
            if (i == 0) {
                isOverflow = true;
            } else {
                sum -= 10;
                nTakeOver = 1;
                number[i] = '0' + sum;
            }
        } else {
            number[i] = '0' + sum;
            break;
        }
    }
    
    return isOverflow;
}

void Print1ToMaxOfNDigits::print1ToMaxOfNDigits_1(int n) {
    if (n <= 0) {
        return;
    }
    
    char* number = new char[n + 1];
    memset(number, '0', n);
    number[n] = '\0';
    
    while (!increment(number)) {
        printNumber(number);
    }
    
    delete [] number;
}

void Print1ToMaxOfNDigits::print1ToMaxOfNDigitsRecursively(char* number, int length, int index) {
    if (index == length - 1) {
        printNumber(number);
        return;
    }
    
    for (int i = 0; i < 10; ++i) {
        number[index + 1] = i + '0';
        print1ToMaxOfNDigitsRecursively(number, length, index + 1);
    }
}

void Print1ToMaxOfNDigits::print1ToMaxOfNDigits_2(int n) {
    if (n <= 0) {
        return;
    }
    
    char* number = new char[n + 1];
    number[n] = '\0';
    
    for (int i = 0; i < 10; ++i) {
        number[0] = i + '0';
        print1ToMaxOfNDigitsRecursively(number, n, 0);
    }
}

```
## 18:(一)在 O(1) 时间删除链表结点
&emsp;题目：给定单向链表的头指针和一个结点指针，定义一个函数在O(1)时间删除该结点。
```c++
namespace DeleteNodeInList {

void deleteNode(ListNode** pListHead, ListNode* pToBeDeleted);

}

void DeleteNodeInList::deleteNode(ListNode** pListHead, ListNode* pToBeDeleted) {
    if (pListHead == nullptr || *pListHead == nullptr || pToBeDeleted == nullptr) {
        return;
    }
    
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

```
## 18:(二)删除链表中重复的结点
&emsp;题目：在一个排序的链表中，如何删除重复的结点？
```c++
namespace DeleteDuplicatedNode {

void deleteDuplication(ListNode** pHead);

}

void DeleteDuplicatedNode::deleteDuplication(ListNode** pHead) {
    if (pHead == nullptr || *pHead == nullptr) {
        return;
    }
    
    ListNode* pPreNode = nullptr;
    ListNode* pNode = *pHead;
    
    while (pNode != nullptr) {
        ListNode* pNext = pNode->m_pNext;
        bool needDelete = false;
        
        if (pNext != nullptr && pNode->m_nValue == pNext->m_nValue) {
            needDelete = true;
        }
        
        if (!needDelete) {
            pPreNode = pNode;
            pNode = pNode->m_pNext;
        } else {
            int value = pNode->m_nValue;
            ListNode* pToBeDelNode = pNode;
            
            while (pToBeDelNode != nullptr && pToBeDelNode->m_nValue == value) {
                pNext = pToBeDelNode->m_pNext;
                
                delete pToBeDelNode;
                pToBeDelNode = nullptr;
                
                pToBeDelNode = pNext;
            }
            
            if (pPreNode == nullptr) {
                *pHead = pNext;
            } else {
                pPreNode->m_pNext = pNext;
            }
            
            pNode = pNext;
        }
    }
}
```
## 面试题 19:正则表达式匹配
&emsp;题目：请实现一个函数用来匹配包含 '.' 和 '*' 的正则表达式。模式中的字符 '.' 表示任意一个字符，而 '*' 表示它前面的字符可以出现任意次（含 0 次）。在本题中，匹配是指字符串的所有字符匹配整个模式。例如，字符串 "aaa" 与模式 "a.a" 和 "ab*ac*a" 匹配，但与 "aa.a" 及 "ab*a" 均不匹配。
```c++
// 
```
## 面试题 20:表示数值的字符串
&emsp;题目：请实现一个函数用来判断字符串是否表示数值（包括整数和小数）。例如，字符串“+100”、“5e2”、“-123”、“3.1416”及“-1E-16”都表示数值，但“12e”、“1a3.14”、“1.2.3”、“+-5”及“12e+5.4”都不是。
```c++
//
```
