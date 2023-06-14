//
//  fast_BSP.hpp
//  SINGER
//
//  Created by Yun Deng on 5/25/23.
//

#ifndef fast_BSP_hpp
#define fast_BSP_hpp

#include <stdio.h>
#include "Tree.hpp"
#include "Emission.hpp"
#include "Interval.hpp"
#include "Coalescent_calculator.hpp"

using Interval_ptr = shared_ptr<Interval>;

class fast_BSP {
    
public:
    
    // basic setup
    float cut_time = 0.0;
    float cutoff = 0.0;
    float epsilon = 1e-6;
    shared_ptr<Emission> eh;
    set<float> check_points = {};
    equal_interval ei = equal_interval();
    
    // hmm running results
    vector<float> rhos = {};
    vector<float> recomb_sums = {}; // length: number of blocks - 1
    vector<float> weight_sums = {}; // length: number of blocks
    
    // hmm states
    int curr_index = 0;
    map<int, vector<Interval_ptr>>  state_spaces = {{INT_MAX, {}}};
    map<int, vector<float>> times_map = {{INT_MAX, {}}};
    map<int, vector<float>> weights_map = {{INT_MAX, {}}};
    vector<Interval_ptr> curr_intervals = {};
    vector<float> temp_probs = {};
    vector<Interval_ptr> temp_intervals = {};
    
    // coalescent computation
    shared_ptr<Coalescent_calculator> cc;
    
    // transfer at recombinations
    map<Interval_info, vector<Interval_ptr>> transfer_intervals = {};
    map<Interval_info, vector<float>> transfer_weights = {};
    
    // cache:
    float prev_rho = -1;
    float prev_theta = -1;
    Node_ptr prev_node = nullptr;
    
    // vector computation:
    int dim = 0;
    float recomb_sum = 0;
    float weight_sum = 0;
    vector<float> raw_times = {};
    vector<float> raw_weights = {};
    vector<float> recomb_probs = {};
    vector<float> recomb_weights = {};
    vector<float> null_emit_probs = {};
    vector<float> mut_emit_probs = {};
    int sample_index = -1;
    vector<float> trace_back_probs = {};
    vector<vector<float>> forward_probs = {};
    
    // states after pruning:
    float valid_prob = 0.0;
    bool branch_change = false;
    bool interval_change = false;
    set<Branch> full_branches = {};
    set<Branch> covered_branches = {};
    set<Branch> valid_branches = {};
    map<Branch, set<Interval_info>> valid_intervals = {};
    
    fast_BSP();
    
    ~fast_BSP();
    
    void reserve_memory(int length);
    
    void start(set<Interval_info> &intervals, float t);
    
    void set_cutoff(float x);
    
    void set_emission(shared_ptr<Emission> e);
    
    void set_check_points(set<float> &p);
    
    void forward(float rho); // forward pass when there is no recombination (without emission). Don't forget to update recomb_sums and weight_sums.
    
    void transfer(Recombination &r); // forward pass when there is a recombination (without emission), and add a transition object. Don't forget to update active intervals, recomb_sums and weight_sums.
    
    void regular_forward(float rho);
    
    void update(float rho);
    
    float get_recomb_prob(float rho, float t);
    
    void null_emit(float theta, Node_ptr query_node);
    
    void mut_emit(float theta, float bin_size, set<float> &mut_set, Node_ptr query_node);
    
    map<float, Branch> sample_joining_branches(int start_index, vector<float> &coordinates);
    
    float avg_num_states();
    
    // private methods:
    
    // bool intercept(Interval_ptr interval);
    
    // bool intercept(Interval_info &ii);
    
    void update_states(set<Interval_info> &deletions, set<Interval_info> &insertions);
    
    void set_dimensions();
    
    void compute_recomb_probs(float rho);
    
    void compute_recomb_weights(float rho);
    
    void compute_null_emit_prob(float theta, Node_ptr query_node);
    
    void compute_mut_emit_probs(float theta, float bin_size, set<float> &mut_set, Node_ptr query_node);
    
    void transfer_helper(Interval_info &next_interval, Interval_ptr prev_interval, float w);
    
    float compute_transfer_prob();
    
    void compute_interval_info();
    
    void sanity_check(Recombination &r);
    
    void get_full_branches(Recombination &r);
    
    void generate_intervals(Recombination &r);
    
    float get_overwrite_prob(Recombination &r, float lb, float ub);
    
    void process_interval(Recombination &r, int i);
    
    void process_source_interval(Recombination &r, int i);
    
    void process_target_interval(Recombination &r, int i);
    
    void process_other_interval(Recombination &r, int i);
    
    float random();
    
    int get_prev_breakpoint(int x);
    
    vector<Interval_ptr > &get_state_space(int x);
    
    vector<float> &get_raw_times(int x);
    
    vector<float> &get_raw_weights(int x);
    
    int get_interval_index(Interval_ptr interval, vector<Interval_ptr> &intervals);
    
    void simplify(map<float, Branch> &joining_branches);
    
    Interval_ptr sample_curr_interval(int x);
    
    Interval_ptr sample_prev_interval(int x);
    
    Interval_ptr sample_source_interval(Interval_ptr interval, int x);
    
    Interval_ptr sample_connection_interval(Interval_ptr interval, int x);
    
    int trace_back_helper(Interval_ptr interval, int x);
    
    // void check_intervals();
    
    // void check_recomb_sums();
    
};

#endif /* fast_BSP_hpp */
