//
//  Node.hpp
//  SINGER
//
//  Created by Yun Deng on 3/31/22.
//

#ifndef Node_hpp
#define Node_hpp

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <unordered_set>
using namespace std;

class Node {
    
public:
    
    unordered_set<float> mutation_sites = {};
    unordered_set<float> ambiguous_sites = {};
    
    int index = 0;
    
    float time = 0;
    
    Node(float t);
    
    void set_index(int index);
    
    void print();
    
    void add_mutation(float pos);
    
    float get_state(float pos);
    
    void write_state(float pos, float s);
    
    void read_mutation(string filename);
};

struct compare_node {
    
    bool operator() (const Node *n1, const Node *n2) const {
        if (n1->time != n2->time) {
            return n1->time < n2->time;
        } else if (n1->index != n2->index) {
            return n1->index < n2->index;
        } else {
            return n1 < n2;
        }
    }
};

#endif /* Node_hpp */
