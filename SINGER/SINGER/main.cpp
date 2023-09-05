//
//  main.cpp
//  SINGER
//
//  Created by Yun Deng on 3/13/23.
//

#include <iostream>
#include "Test.hpp"

struct Config {
    bool resume = false;
    bool fast = false;
    int num_iters = 0;
    int spacing = 1;
    float start_pos = -1, end_pos = -1;
    float Ne = -1, r = -1, m = -1;
    string input_filename = "", output_prefix = "";
    float penalty = 1;
    float epsilon_hmm = 0.01;
    float epsilon_psmc = 0.05;
};

Config parse_argument(int argc, const char* argv[]) {
    Config config;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-fast") {
            if (i + 1 < argc && argv[i+1][0] != '-') {
                cerr << "Error: -fast flag doesn't take any value. " << endl;
                exit(1);
            }
            config.fast = true;
        }
        else if (arg == "-Ne") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -Ne flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                config.Ne = stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -Ne flag expects a number. " << endl;
                exit(1);
            }
            config.Ne = 2*config.Ne;
        }
        else if (arg == "-r") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -r flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                config.r = stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -r flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-m") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -m flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                config.m = stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -r flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-penalty") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -penalty flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                config.penalty = stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -penalty flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-hmm_epsilon") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -hmm_epsilon flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                config.epsilon_hmm = stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -hmm_epsilon flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-psmc_bins") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -psmc_epsilon flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                config.epsilon_psmc = 1.0/stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -p flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-start") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -start flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                config.start_pos = stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -start flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-end") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -end flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                config.end_pos = stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -end flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-input") {
            if (i + 1 > argc || argv[i+1][0] == '-') {
                cerr << "Error: -input flag cannot be empty. " << endl;
            }
            config.input_filename = argv[++i];
        }
        else if (arg == "-output") {
            if (i + 1 > argc || argv[i+1][0] == '-') {
                cerr << "Error: -output flag cannot be empty. " << endl;
            }
            config.output_prefix = argv[++i];
        }
        else if (arg == "-n") {
            if (i + 1 > argc || argv[i+1][0] == '-') {
                cerr << "Error: -n flag cannot be empty. " << endl;
            }
            try {
                config.num_iters = stoi(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -n flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-thinning") {
            if (i + 1 > argc || argv[i+1][0] == '-') {
                cerr << "Error: -thinning flag cannot be empty. " << endl;
            }
            try {
                config.spacing = stoi(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -thinning flag expects a number. " << endl;
                exit(1);
            }
        }
        else {
            cerr << "Error: Unknown flag. " << arg << endl;
            exit(1);
        }
    }
    if (config.r < 0) {
        cerr << "-r flag missing or invalid value. " << endl;
    }
    if (config.m < 0) {
        cerr << "-m flag missing or invalid value. " << endl;
    }
    if (config.Ne < 0) {
        cerr << "-Ne flag missing or invalid value. " << endl;
    }
    if (config.input_filename.size() == 0) {
        cerr << "-input flag missing or invalid value. " << endl;
    }
    if (config.output_prefix.size() == 0) {
        cerr << "-output flag missing or invalid value. " << endl;
    }
    if (config.num_iters < 0) {
        cerr << "-num_iters flag is invalid. " << endl;
    }
    if (config.spacing < 1) {
        cerr << "-thinning flag is invalid. " << endl;
    }
    return config;
}

/*
int main(int argc, const char * argv[]) {
    bool fast = false;
    int resume_point = -1;
    float r = -1, m = -1, Ne = -1;
    int num_iters = 0;
    int spacing = 1;
    float start_pos = -1, end_pos = -1;
    string input_filename = "", output_prefix = "";
    float penalty = 1;
    float epsilon_hmm = 0.01;
    float epsilon_psmc = 0.05;
    unsigned seed = -1;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-fast") {
            if (i + 1 < argc && argv[i+1][0] != '-') {
                cerr << "Error: -fast flag doesn't take any value. " << endl;
                exit(1);
            }
            fast = true;
        }
        else if (arg == "-resume") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -resume flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                resume_point = stoi(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -resume flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-Ne") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -Ne flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                Ne = stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -Ne flag expects a number. " << endl;
                exit(1);
            }
            Ne = 2*Ne;
        }
        else if (arg == "-r") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -r flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                r = stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -r flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-m") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -m flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                m = stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -r flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-penalty") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -penalty flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                penalty = stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -penalty flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-hmm_epsilon") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -hmm_epsilon flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                epsilon_hmm = stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -hmm_epsilon flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-psmc_bins") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -psmc_epsilon flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                epsilon_psmc = 1.0/stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -p flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-start") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -start flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                start_pos = stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -start flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-end") {
            if (i + 1 >= argc || argv[i+1][0] == '-') {
                cerr << "Error: -end flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                end_pos = stod(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -end flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-input") {
            if (i + 1 > argc || argv[i+1][0] == '-') {
                cerr << "Error: -input flag cannot be empty. " << endl;
                exit(1);
            }
            input_filename = argv[++i];
        }
        else if (arg == "-output") {
            if (i + 1 > argc || argv[i+1][0] == '-') {
                cerr << "Error: -output flag cannot be empty. " << endl;
                exit(1);
            }
            output_prefix = argv[++i];
        }
        else if (arg == "-n") {
            if (i + 1 > argc || argv[i+1][0] == '-') {
                cerr << "Error: -n flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                num_iters = stoi(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -n flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-thinning") {
            if (i + 1 > argc || argv[i+1][0] == '-') {
                cerr << "Error: -thinning flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                spacing = stoi(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -thinning flag expects a number. " << endl;
                exit(1);
            }
        }
        else if (arg == "-seed") {
            if (i + 1 > argc || argv[i+1][0] == '-') {
                cerr << "Error: -seed flag cannot be empty. " << endl;
                exit(1);
            }
            try {
                seed  = stoi(argv[++i]);
            } catch (const invalid_argument&) {
                cerr << "Error: -seed flag expects a number. " << endl;
                exit(1);
            }
        }
        else {
            cerr << "Error: Unknown flag. " << arg << endl;
            exit(1);
        }
    }
    if (r < 0) {
        cerr << "-r flag missing or invalid value. " << endl;
        exit(1);
    }
    if (m < 0) {
        cerr << "-m flag missing or invalid value. " << endl;
        exit(1);
    }
    if (Ne < 0) {
        cerr << "-Ne flag missing or invalid value. " << endl;
        exit(1);
    }
    if (input_filename.size() == 0) {
        cerr << "-input flag missing or invalid value. " << endl;
        exit(1);
    }
    if (output_prefix.size() == 0) {
        cerr << "-output flag missing or invalid value. " << endl;
        exit(1);
    }
    if (num_iters < 0) {
        cerr << "-num_iters flag is invalid. " << endl;
        exit(1);
    }
    if (spacing < 1) {
        cerr << "-thinning flag is invalid. " << endl;
        exit(1);
    }
    Sampler sampler = Sampler(Ne, r, m);
    sampler.penalty = penalty;
    sampler.set_precision(0.01, 0.05);
    sampler.set_output_file_prefix(output_prefix);
    sampler.load_vcf(input_filename, start_pos, end_pos);
    if (resume_point > 0) {
        if (fast) {
            sampler.resume_fast_internal_sample(num_iters, spacing, resume_point, seed, 561618);
        } else {
            sampler.random_seed = seed;
            sampler.resume_internal_sample(num_iters, spacing, resume_point, seed, 561618);
        }
    }
    if (fast) {
        sampler.fast_iterative_start();
    } else {
        sampler.iterative_start();
    }
    if (fast) {
        sampler.internal_sample(num_iters, spacing);
    } else {
        sampler.internal_sample(num_iters, spacing);
    }
    return 0;
}
*/


int main(int argc, const char * argv[]) {
    test_internal_sampling();
    // test_african_dataset();
}
