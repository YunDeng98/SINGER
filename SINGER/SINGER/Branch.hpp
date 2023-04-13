//
//  Branch.hpp
//  SINGER
//
//  Created by Yun Deng on 3/31/22.
//

#ifndef Branch_hpp
#define Branch_hpp

#include <stdio.h>
#include "Node.hpp"

class Branch {
    
public:
    
    Node *lower_node;
    Node *upper_node;
    
    Branch();
    
    Branch(Node *l, Node *u);
    
    float length();
    
    bool operator<(const Branch &other) const;
    
    bool operator==(const Branch &other) const;
    
    bool operator!=(const Branch &other) const;
};

#endif /* Branch_hpp */
