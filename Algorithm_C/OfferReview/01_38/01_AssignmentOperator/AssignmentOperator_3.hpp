//
//  AssignmentOperator_3.hpp
//  OfferReview
//
//  Created by CHM on 2021/1/22.
//  Copyright © 2021 CHM. All rights reserved.
//

#ifndef AssignmentOperator_3_hpp
#define AssignmentOperator_3_hpp

#include <stdio.h>
#include <cstring>
#include <cstdio>

using namespace std;

namespace AssignmentOperator_3 {

class CMyString {
public:
    CMyString(const char* pData = nullptr);
    CMyString(const CMyString& str);
    ~CMyString(void);
    
    CMyString& operator = (const CMyString& str);
    
    void Print();
private:
    char* m_pData;
};

}

#endif /* AssignmentOperator_3_hpp */
