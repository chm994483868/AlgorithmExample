//
//  SquenceOfBST.cpp
//  OfferReview
//
//  Created by CHM on 2020/7/30.
//  Copyright © 2020 CHM. All rights reserved.
//

#include "SquenceOfBST.hpp"

bool SquenceOfBST::verifySquenceOfBST(int sequence[], int length) {
    if (sequence == nullptr || length <= 0) {
        return false;
    }
    
    int nRootValue = sequence[length - 1];
    unsigned int nLeftIndexEnd = 0;
    for (; nLeftIndexEnd < length - 1; ++nLeftIndexEnd) {
        if (sequence[nLeftIndexEnd] > nRootValue) {
            break;
        }
    }

    unsigned int nRightStart = nLeftIndexEnd;
    for (; nRightStart < length - 1; ++nRightStart) {
        if (sequence[nRightStart] < nRootValue) {
            return false;
        }
    }

    bool bLeft = true;
    if (nLeftIndexEnd > 0) {
        bLeft = verifySquenceOfBST(sequence, nLeftIndexEnd);
    }

    bool bRight = true;
    if (nLeftIndexEnd < length - 1) {
        bRight = verifySquenceOfBST(sequence + nLeftIndexEnd, length - nLeftIndexEnd - 1);
    }

    return bLeft && bRight;
}

// 测试代码
void SquenceOfBST::Test(const char* testName, int sequence[], int length, bool expected) {
    if(testName != nullptr)
        printf("%s begins: ", testName);
    
    if(verifySquenceOfBST(sequence, length) == expected)
        printf("passed.\n");
    else
        printf("failed.\n");
}

//            10
//         /      \
//        6        14
//       /\        /\
//      4  8     12  16
void SquenceOfBST::Test1() {
    int data[] = {4, 8, 6, 12, 16, 14, 10};
    Test("Test1", data, sizeof(data)/sizeof(int), true);
}

//           5
//          / \
//         4   7
//            /
//           6
void SquenceOfBST::Test2() {
    int data[] = {4, 6, 7, 5};
    Test("Test2", data, sizeof(data)/sizeof(int), true);
}

//               5
//              /
//             4
//            /
//           3
//          /
//         2
//        /
//       1
void SquenceOfBST::Test3() {
    int data[] = {1, 2, 3, 4, 5};
    Test("Test3", data, sizeof(data)/sizeof(int), true);
}

// 1
//  \
//   2
//    \
//     3
//      \
//       4
//        \
//         5
void SquenceOfBST::Test4() {
    int data[] = {5, 4, 3, 2, 1};
    Test("Test4", data, sizeof(data)/sizeof(int), true);
}

// 树中只有1个结点
void SquenceOfBST::Test5() {
    int data[] = {5};
    Test("Test5", data, sizeof(data)/sizeof(int), true);
}

void SquenceOfBST::Test6() {
    int data[] = {7, 4, 6, 5};
    Test("Test6", data, sizeof(data)/sizeof(int), false);
}

void SquenceOfBST::Test7() {
    int data[] = {4, 6, 12, 8, 16, 14, 10};
    Test("Test7", data, sizeof(data)/sizeof(int), false);
}

void SquenceOfBST::Test8() {
    Test("Test8", nullptr, 0, false);
}

void SquenceOfBST::Test() {
    Test1();
    Test2();
    Test3();
    Test4();
    Test5();
    Test6();
    Test7();
    Test8();
}
