# 《剑指 Offer》面试题四十一～面试题五十的总结

> &emsp;上一篇是 31～40 题，本篇是 41～50 题。⛽️⛽️

## 面试题 41:数据流中的中位数
&emsp;题目：如何得到一个数据流中的中位数？如果从数据流中读出奇数个数值，那么中位数就是所有数值排序之后位于中间的数值。如果从数据流中读出偶数个数值，那么中位数就是所有数值排序之后中间两个数的平均值。
```c++
namespace StreamMedian {

template <typename T>
class DynamicArray {
public:
    void insert(T num) {
        if (((min.size() + max.size()) & 1) == 0) {
            if (max.size() > 0 && num < max[0]) {
                max.push_back(num);
                push_heap(max.begin(), max.end(), less<T>());
                
                num = max[0];
                
                pop_heap(max.begin(), max.end(), less<T>());
                max.pop_back();
            }
            
            min.push_back(num);
            push_heap(min.begin(), min.end(), greater<T>());
        } else {
            if (min.size() > 0 && min[0] < num) {
                min.push_back(num);
                push_heap(min.begin(), min.end(), greater<T>());
                
                num = min[0];
                
                pop_heap(min.begin(), min.end(), greater<T>());
                min.pop_back();
            }
            
            max.push_back(num);
            push_heap(max.begin(), max.end(), less<T>());
        }
    }
    
    T getMedian() {
        int size = min.size() + max.size();
        if (size == 0) {
            throw exception(); // no numbers are available
        }
        
        T median = 0;
        if ((size & 1) == 1) {
            median = min[0];
        } else {
            median = (min[0] + max[0]) / 2;
        }
        
        return median;
    }
    
private:
    vector<T> min;
    vector<T> max;
};
}
```
## 面试题 42:连续子数组的最大和
&emsp;题目：输入一个整型数组，数组里有正数也有负数。数组中一个或连续的多个整数组成一个子数组。求所有子数组的和的最大值。要求时间复杂度为 O(n)。
```c++
namespace GreatestSumOfSubarrays {
static bool g_InvalidInput = false;
int findGreatestSumOfSubArray(int* pData, int nLength);
}

int GreatestSumOfSubarrays::findGreatestSumOfSubArray(int* pData, int nLength) {
    if (pData == nullptr || nLength <= 0) {
        g_InvalidInput = true;
        return 0;
    }
    
    g_InvalidInput = false;
    int nCurSum = 0;
    int nGreatestSum = 0x80000000;
    for (unsigned int i = 0; i < nLength; ++i) {
        if (nCurSum <= 0) {
            nCurSum = pData[i];
        } else {
            nCurSum += pData[i];
        }
        
        if (nCurSum > nGreatestSum) {
            nGreatestSum = nCurSum;
        }
    }
    
    return nGreatestSum;
}
```
## 43:从1到n整数中1出现的次数
&emsp;题目：输入一个整数 n，求从 1 到 n 这 n 个整数的十进制表示中1出现的次数。例如输入 12，从 1 到 12 这些整数中包含 1 的数字有 1，10，11 和 12，1 一共出现了 5 次。
```c++
namespace NumberOf1 {
int numberOf1(unsigned int n);
int numberOf1Between1AndN_Solution1(unsigned int n);

int numberOf1(const char* strN);
int powerBase10(unsigned int n);
int numberOf1Between1AndN_Solution2(int n);
}

int NumberOf1::numberOf1(unsigned int n) {
    int number = 0;
    while (n) {
        if (n % 10 == 1) {
            ++number;
        }
        
        n /= 10;
    }
    
    return number;
}

int NumberOf1::numberOf1Between1AndN_Solution1(unsigned int n) {
    int number = 0;
    for (unsigned int i = 1; i <= n; ++i) {
        number += numberOf1(i);
    }
    
    return number;
}

int NumberOf1::numberOf1(const char* strN) {
    if (strN == nullptr || *strN < '0' || *strN > '9' || *strN == '\0') {
        return 0;
    }
    
    int first = *strN - '0';
    unsigned int length = static_cast<unsigned int>(strlen(strN));
    
    if (length == 1 && first == 0) {
        return 0;
    }
    
    if (length == 1 && first > 0) {
        return 1;
    }
    
    int numFirstDigit = 0;
    if (first > 1) {
        numFirstDigit = powerBase10(length - 1);
    } else if (first == 1) {
        numFirstDigit = atoi(strN + 1) + 1;
    }
    
    int numOtherDigits = first * (length - 1) * powerBase10(length - 2);
    
    // 递归
    int numRecursive = numberOf1(strN + 1);
    
    return numFirstDigit + numOtherDigits + numRecursive;
}

// 10 的次方
int NumberOf1::powerBase10(unsigned int n) {
    int result = 1;
    for (unsigned int i = 0; i < n; ++i) {
        result *= 10;
    }
    return result;
}

int NumberOf1::numberOf1Between1AndN_Solution2(int n) {
    if (n <= 0) {
        return 0;
    }
    
    char strN[50];
    sprintf(strN, "%d", n);
    
    return numberOf1(strN);
}
```
## 44:数字序列中某一位的数字
&emsp;题目：数字以 0123456789101112131415⋯ 的格式序列化到一个字符序列中。在这个序列中，第 5 位（从 0 开始计数）是 5，第 13 位是 1，第 19 位是 4，等等。请写一个函数求任意位对应的数字。
```c++
namespace DigitsInSequence {
int countOfIntegers(int digits);
int digitAtIndex(int index, int digits);
int beginNumber(int digits);

int digitAtIndex(int index);
}

int DigitsInSequence::countOfIntegers(int digits) {
    if (digits == 1) {
        return 10;
    }
    
    int count = (int)pow(10, digits - 1);
    return 9 * count;
}

int DigitsInSequence::digitAtIndex(int index, int digits) {
    int number = beginNumber(digits) + index / digits;
    int indexFromRight = digits - index % digits;
    for (int i = 1; i < indexFromRight; ++i) {
        number /= 10;
    }
    
    return number % 10;
}

int DigitsInSequence::beginNumber(int digits) {
    if (digits == 1) {
        return 0;
    }
    
    return (int)pow(10, digits - 1);
}

int DigitsInSequence::digitAtIndex(int index) {
    if (index < 0) {
        return -1;
    }
    
    int digits = 1;
    while (true) {
        int number = countOfIntegers(digits);
        if (index < number * digits) {
            return digitAtIndex(index, digits);
        }
        
        index -= number * digits;
        ++digits;
    }
    
    return -1;
}
```
## 完结撒花🎉🎉，感谢陪伴！
