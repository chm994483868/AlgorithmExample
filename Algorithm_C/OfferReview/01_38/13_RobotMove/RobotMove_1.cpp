//
//  RobotMove_1.cpp
//  OfferReview
//
//  Created by CHM on 2020/11/2.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "RobotMove_1.hpp"

int RobotMove_1::movingCoungCore(int threshold, int rows, int cols, int row, int col, bool* visited) {
    int count = 0;
    if (check(threshold, rows, cols, row, col, visited)) {
        visited[row * cols + col] = true;
        count = 1 + movingCoungCore(threshold, rows, cols, row - 1, col, visited) + movingCoungCore(threshold, rows, cols, row, col - 1, visited) + movingCoungCore(threshold, rows, cols, row + 1, col, visited) + movingCoungCore(threshold, rows, cols, row, col + 1, visited);
    }
    
    return count;
}

bool RobotMove_1::check(int threshold, int rows, int cols, int row, int col, bool* visited) {
    if (row >= 0 && row < rows && col >= 0 ) {
        <#statements#>
    }
}
