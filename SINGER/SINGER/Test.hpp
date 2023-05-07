//
//  Test.hpp
//  SINGER
//
//  Created by Yun Deng on 3/17/23.
//

#ifndef Test_hpp
#define Test_hpp

#include <stdio.h>
#include "Sampler.hpp"
#include "Trace_pruner.hpp"

void test_read_arg();

void test_parsimony_pruner();

void test_pruner_efficiency();

void test_trace_pruner();

void test_iterative_start();

void test_fast_iterative_start();

void test_terminal_sampling();

void test_internal_sampling();

void test_fast_internal_sampling();

#endif /* Test_hpp */
