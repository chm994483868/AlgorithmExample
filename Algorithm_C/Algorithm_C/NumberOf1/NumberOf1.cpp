//
//  NumberOf1.cpp
//  Algorithm_C
//
//  Created by CHM on 2020/7/22.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "NumberOf1.hpp"

int numberOf1(unsigned int n);
int numberOf1Between1AndN_Solution1(unsigned int n) {
    int number = 0;
    for (int i = 1; i <= n; ++i) {
        number = numberOf1(i);
    }
    
    return number;
}

int numberOf1(unsigned int n) {
    int number = 0;
    
    while (n) {
        if (n % 10 == 1) {
            ++number;
        }
        
        n /= 10;
    }
    
    return number;
}

namespace TEST {

int randInRange(int min, int max) {
    int random = rand() % (max - min + 1) + min;
    return random;
}

// 从 start 和 end 中取一个随机值
// 从数组中取一个随机值 data[index];
// 把这个随机值和数组最后一个元素交换位置
// small 从 start - 1 开始
// 遍历 data
// 遇到小于 data[end] 的值 自增 small 如果 small 和 index 不同
// 交换
// 最后 small 自增，如果 end 不等，交换 data[small] 和 data[end]
// 

}
