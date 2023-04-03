//
//  BSP_smc.hpp
//  SINGER
//
//  Created by Yun Deng on 4/2/23.
//

#ifndef BSP_smc_hpp
#define BSP_smc_hpp

#include <stdio.h>
#include "Tree.hpp"
#include "Interval.hpp"
// #include "Binary_emission.hpp"

class BSP_smc {
    
public:
    
    float cut_time = 0.0;
    float start_pos = 0.0;
    float end_pos = 0.0;
    vector<float> rhos = {};
    set<float> check_points = {};
    
    int curr_pos;
    float cutoff = 0;
    map<int, vector<Interval *>>  state_spaces = {{INT_MAX, {}}};
    vector<float> recomb_sums = {}; // length: number of blocks - 1
    vector<float> weight_sums = {}; // length: number of blocks
    vector<Interval *> curr_intervals = {};
    // Fitch_emission eh;
    // Binary_emission eh;
    
    set<float> coalescence_times = {};
    map<float, int> coalescence_rates = {};
    map<float, float> coalescence_probs = {};
    map<float, float> coalescence_quantiles = {};
    
    map<Interval_info, vector<float>> transfer_weights = {};
    map<Interval_info, vector<Interval *>> transfer_intervals = {};
    
    BSP_smc();
    
    ~BSP_smc();
    
    void start(set<Branch>, float x, float t);
    
    void set_cutoff(float x);
    
    void set_check_points(set<float> p);
    
    void forward(float rho); // forward pass when there is no recombination (without emission). Don't forget to update recomb_sums and weight_sums.
    
    void transfer(Recombination r); // forward pass when there is a recombination (without emission), and add a transition object. Don't forget to update active intervals, recomb_sums and weight_sums.
    
    void null_emit(float theta, Node *base_node);
    
    void mut_emit(float bin_size, float theta, vector<float> mutations, Node *base_node);
    
    map<float, Branch> sample_joining_branches();
    
    // private methods:
    
    void transfer_helper(Interval_info interval_info, Interval *i, float w);
    
    void fill_interval_info();
    
    void sanity_check(Recombination r);
    
    float get_prop(float lb, float ub);
    
    float get_overwrite_prob(Recombination r, float lb, float ub);
    
    void update_coalescence_times(Recombination r);
    
    void calculate_coalescence_stats();
    
    float get_prob(float x);
    
    float get_quantile(float p);
    
    float get_median(float lb, float ub);
    
    void process_interval(Recombination r, Interval *prev_interval);
    
    void process_source_interval(Recombination r, Interval *prev_interval);
    
    void process_target_interval(Recombination r, Interval *prev_interval);
    
    void process_other_interval(Recombination r, Interval *prev_interval);
    
    float random();
    
    int get_prev_breakpoint(int x);
    
    vector<Interval *> get_state_space(int x);
    
    Interval *sample_terminal_interval(int x);
    
    Interval *sample_prev_interval(int x);
    
    int trace_back_helper(Interval *i, int x);
    
};

#endif /* BSP_smc_hpp */
