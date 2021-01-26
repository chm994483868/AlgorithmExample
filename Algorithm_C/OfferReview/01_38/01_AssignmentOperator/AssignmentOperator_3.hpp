//
//  AssignmentOperator_3.hpp
//  OfferReview
//
//  Created by CHM on 2021/1/26.
//  Copyright © 2021 CHM. All rights reserved.
//

#ifndef AssignmentOperator_3_hpp
#define AssignmentOperator_3_hpp

#include <stdio.h>
#include <cstring>
#include <cstdio>

namespace AssignmentOperator_3 {

class CMyString {
public:
    CMyString(const char* pData = nullptr);
    CMyString(const CMyString& str);
    ~CMyString(void);
    
    CMyString& operator = (const CMyString& str);
    
    void Print(void);
    
private:
    char* m_pData;
};

}
#endif /* AssignmentOperator_3_hpp */
