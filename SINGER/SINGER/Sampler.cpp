//
//  Sampler.cpp
//  SINGER
//
//  Created by Yun Deng on 3/31/23.
//

#include "Sampler.hpp"

Sampler::Sampler(float pop_size, float r, float m) {
    Ne = pop_size;
    mut_rate = m*pop_size;
    recomb_rate = r*pop_size;
}

void Sampler::set_precision(float c, float q) {
    bsp_c = c;
    tsp_q = q;
}

void Sampler::set_pop_size(float n) {
    Ne = n;
}

void Sampler::set_input_file_prefix(string f) {
    input_prefix = f;
}

void Sampler::set_output_file_prefix(string f) {
    output_prefix = f;
}

void Sampler::set_log_file_prefix(string f) {
    log_prefix = f;
}

void Sampler::set_sequence_length(float x) {
    sequence_length = x;
}

void Sampler::set_num_samples(int n) {
    num_samples = n;
}

void Sampler::optimal_ordering() {
    build_all_nodes();
    unordered_map<float, set<Node_ptr>> carriers = {};
    for (Node_ptr n : sample_nodes) {
        for (float m : n->mutation_sites) {
            carriers[m].insert(n);
        }
    }
    while (carriers.size() > 0) {
        cout << "Curr number of nodes: " << ordered_sample_nodes.size() << endl;
        cout << "Curr number of mutations: " << carriers.size() << endl;
        auto it = max_element(carriers.begin(), carriers.end(), [](const auto& l, const auto& r) {return l.second.size() < r.second.size();});
        Node_ptr n = *(it->second.begin());
        for (float m : n->mutation_sites) {
            carriers.erase(m);
        }
        ordered_sample_nodes.push_back(n);
    }
    cout << "Finished ordering" << endl;
}

Node_ptr Sampler::build_node(int index, float time) {
    Node_ptr n = new_node(time);
    n->index = index;
    string mutation_file = input_prefix + "_" + to_string(index) + ".txt";
    n->read_mutation(mutation_file);
    return n;
}

void Sampler::build_all_nodes() {
    for (int i = 0; i < num_samples; i++) {
        Node_ptr n = build_node(i, 0.0);
        sample_nodes.insert(n);
    }
}

void Sampler::build_singleton_arg() {
    float bin_size = rho_unit/recomb_rate;
    Node_ptr n = build_node(0, 0.0);
    arg = ARG(Ne, sequence_length);
    arg.discretize(bin_size);
    arg.build_singleton_arg(n);
    arg.compute_rhos_thetas(recomb_rate, mut_rate);
}

void Sampler::build_void_arg() {
    float bin_size = rho_unit/recomb_rate;
    arg = ARG(Ne, sequence_length);
    arg.discretize(bin_size);
    arg.compute_rhos_thetas(recomb_rate, mut_rate);
}

void Sampler::iterative_start() {
    build_singleton_arg();
    for (int i = 1; i < num_samples; i++) {
        random_seed = rand();
        srand(random_seed);
        Threader_smc threader = Threader_smc(bsp_c, tsp_q, eh);
        Node_ptr n = build_node(i, 0.0);
        threader.thread(arg, n);
        // arg.check_incompatibility();
        // arg.write("/Users/yun_deng/Desktop/SINGER/arg_files/debug_ts_nodes.txt", "/Users/yun_deng/Desktop/SINGER/arg_files/debug_ts_branches.txt", "/Users/yun_deng/Desktop/SINGER/arg_files/arg_files/debug_ts_recombs.txt");
        arg.check_incompatibility();
    }
    arg.write("/Users/yun_deng/Desktop/SINGER/arg_files/debug_ts_nodes.txt", "/Users/yun_deng/Desktop/SINGER/arg_files/debug_ts_branches.txt", "/Users/yun_deng/Desktop/SINGER/arg_files/debug_ts_recombs.txt");
}

void Sampler::fast_iterative_start() {
    build_singleton_arg();
    for (int i = 1; i < num_samples; i++) {
        random_seed = rand();
        srand(random_seed);
        Threader_smc threader = Threader_smc(bsp_c, tsp_q, eh);
        Node_ptr n = build_node(i, 0.0);
        if (arg.sample_nodes.size() > 1) {
            threader.fast_thread(arg, n);
        } else {
            threader.thread(arg, n);
        }
        arg.check_incompatibility();
        // arg.write("/Users/yun_deng/Desktop/SINGER/arg_files/debug_fast_ts_nodes.txt", "/Users/yun_deng/Desktop/SINGER/arg_files/debug_fast_ts_branches.txt", "/Users/yun_deng/Desktop/SINGER/arg_files/arg_files/debug_fast_ts_recombs.txt");
    }
    arg.write("/Users/yun_deng/Desktop/SINGER/arg_files/debug_fast_ts_nodes.txt", "/Users/yun_deng/Desktop/SINGER/arg_files/debug_fast_ts_branches.txt", "/Users/yun_deng/Desktop/SINGER/arg_files/arg_files/debug_fast_ts_recombs.txt");
}

void Sampler::recombination_climb(int num_iters, int spacing) {
    for (int i = 0; i < num_iters; i++) {
        cout << get_time() << " Iteration: " << to_string(i) << endl;
        float updated_length = 0;
        random_seed = rand();
        srand(random_seed);
        while (updated_length < spacing*arg.sequence_length) {
            Threader_smc threader = Threader_smc(bsp_c, tsp_q, eh);
            tuple<float, Branch, float> cut_point = arg.sample_recombination_cut();
            threader.internal_rethread(arg, cut_point);
            updated_length += arg.coordinates[threader.end_index] - arg.coordinates[threader.start_index];
            arg.clear_remove_info();
        }
        arg.check_incompatibility();
        string node_file = output_prefix + "_nodes_" + to_string(sample_index) + ".txt";
        string branch_file= output_prefix + "_branches_" + to_string(sample_index) + ".txt";
        string recomb_file = output_prefix + "_recombs_" + to_string(sample_index) + ".txt";
        arg.write(node_file, branch_file, recomb_file);
        sample_index += 1;
        cout << "Number of trees: " << arg.recombinations.size() << endl;
    }
}


void Sampler::terminal_sample(int num_iters) {
    for (int i = 0; i < num_iters; i++) {
        cout << get_time() << " Iteration: " << to_string(i) << endl;
        random_seed = rand();
        srand(random_seed);
        Threader_smc threader = Threader_smc(bsp_c, tsp_q, eh);
        tuple<int, Branch, float> cut_point = arg.sample_terminal_cut();
        threader.terminal_rethread(arg, cut_point);
        arg.clear_remove_info();
        cout << "Number of trees: " << arg.recombinations.size() << endl;
    }
}

void Sampler::internal_sample(int num_iters, int spacing) {
    for (int i = 0; i < num_iters; i++) {
        cout << get_time() << " Iteration: " << to_string(i) << endl;
        float updated_length = 0;
        random_seed = rand();
        srand(random_seed);
        while (updated_length < spacing*arg.sequence_length) {
            Threader_smc threader = Threader_smc(bsp_c, tsp_q, eh);
            tuple<float, Branch, float> cut_point = arg.sample_internal_cut();
            threader.internal_rethread(arg, cut_point);
            updated_length += arg.coordinates[threader.end_index] - arg.coordinates[threader.start_index];
            arg.clear_remove_info();
        }
        arg.check_incompatibility();
        string node_file = output_prefix + "_nodes_" + to_string(sample_index) + ".txt";
        string branch_file= output_prefix + "_branches_" + to_string(sample_index) + ".txt";
        string recomb_file = output_prefix + "_recombs_" + to_string(sample_index) + ".txt";
        arg.write(node_file, branch_file, recomb_file);
        sample_index += 1;
        cout << "Number of trees: " << arg.recombinations.size() << endl;
    }
}

void Sampler::fast_internal_sample(int num_iters, int spacing) {
    for (int i = 0; i < num_iters; i++) {
        cout << get_time() << " Iteration: " << to_string(i) << endl;
        float updated_length = 0;
        random_seed = rand();
        srand(random_seed);
        while (updated_length < spacing*arg.sequence_length) {
            Threader_smc threader = Threader_smc(bsp_c, tsp_q, eh);
            tuple<float, Branch, float> cut_point = arg.sample_internal_cut();
            threader.fast_internal_rethread(arg, cut_point);
            updated_length += arg.coordinates[threader.end_index] - arg.coordinates[threader.start_index];
            arg.clear_remove_info();
        }
        arg.check_incompatibility();
        // arg.write("/Users/yun_deng/Desktop/SINGER/arg_files/sample_ts_nodes.txt", "/Users/yun_deng/Desktop/SINGER/arg_files/sample_ts_branches.txt", "/Users/yun_deng/Desktop/SINGER/arg_files/sample_ts_recombs.txt");
        string node_file = output_prefix + "fast_nodes_" + to_string(i) + ".txt";
        string branch_file= output_prefix + "fast_branches_" + to_string(i) + ".txt";
        string recomb_file = output_prefix + "fast_recombs_" + to_string(i) + ".txt";
        arg.write(node_file, branch_file, recomb_file);
        cout << "Number of trees: " << arg.recombinations.size() << endl;
    }
}
