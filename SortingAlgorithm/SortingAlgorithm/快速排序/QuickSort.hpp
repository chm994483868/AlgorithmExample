//
//  QuickSort.hpp
//  SortingAlgorithm
//
//  Created by CHM on 2019/12/30.
//  Copyright © 2019 HM C. All rights reserved.
//

#ifndef QuickSort_hpp
#define QuickSort_hpp

#include <stdio.h>

void quickSort(int s[], int l, int r) {
    if (l < r) {
        int i = l, j = r, x = s[l];
        
        while (i < j) {
            // 从右向左找小于 x 的数来填 s[i]
            while (i < j && s[j] >= x) j--;
            // 将 s[j] 填到 s[i] 中，s[j] 就形成了一个新的坑
            if (i < j) s[i++] = s[j];
            
            // 从左往右找大于活等于 x 的数来填 s[j]
            while (i < j && s[i] < x) i++;
            // 将 s[i] 填到 s[j] 中，s[i] 就形成了一个新坑
            if (i < j) s[j--] = s[i];
        }
        
        // 退出时，i 等于 j。将 x 填到这个坑中。
        s[i] = x;
        
        quickSort(s, l, i - 1);
        quickSort(s, i + 1, r);
    }
}

#endif /* QuickSort_hpp */
