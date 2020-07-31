//
//  StreamMedian.hpp
//  OfferReview
//
//  Created by CHM on 2020/7/31.
//  Copyright © 2020 CHM. All rights reserved.
//

#ifndef StreamMedian_hpp
#define StreamMedian_hpp

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <functional>

using namespace std;

namespace StreamMedian {

// 41：数据流中的中位数
// 题目：如何得到一个数据流中的中位数？如果从数据流中读出奇数个数值，那么
// 中位数就是所有数值排序之后位于中间的数值。如果从数据流中读出偶数个数值，
// 那么中位数就是所有数值排序之后中间两个数的平均值。
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

// 测试代码
void Test(char* testName, DynamicArray<double>& numbers, double expected);

void Test();

}

#endif /* StreamMedian_hpp */
