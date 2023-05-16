//
//  random_utils.hpp
//  SINGER
//
//  Created by Yun Deng on 5/10/23.
//

#ifndef random_utils_hpp
#define random_utils_hpp

#pragma once
#include <stdio.h>
#include <random>

extern std::mt19937 random_engine;
extern std::uniform_real_distribution<> uniform_distribution;

float uniform_random();

void set_seed(unsigned seed);

#endif /* random_utils_hpp */
