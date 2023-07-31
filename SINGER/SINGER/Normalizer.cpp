//
//  Normalizer.cpp
//  SINGER
//
//  Created by Yun Deng on 7/13/23.
//

#include "Normalizer.hpp"

Normalizer::Normalizer() {}

Normalizer::~Normalizer() {}

void Normalizer::get_root_span(ARG &a) {
    map<Node_ptr, float, compare_node> root_start = {};
    map<Node_ptr, float, compare_node> root_span = {};
    Branch prev_branch;
    Branch next_branch;
    Recombination &r = a.recombinations.begin()->second;
    prev_branch = *(r.inserted_branches.rbegin());
    root_start[prev_branch.lower_node] = 0;
    auto r_it = a.recombinations.upper_bound(0);
    while (next(r_it)->first < a.sequence_length) {
        Recombination &r = r_it->second;
        prev_branch = *r.deleted_branches.rbegin();
        next_branch = *r.inserted_branches.rbegin();
        if (prev_branch.upper_node == a.root) {
            Node_ptr dn = prev_branch.lower_node;
            Node_ptr in = next_branch.lower_node;
            assert(dn != in);
            assert(next_branch.upper_node == a.root);
            assert(root_start.count(dn) > 0);
            root_span[dn] += r.pos - root_start[dn];
            root_start.erase(dn);
            root_start[in] = r.pos;
        } else {
            assert(next_branch.upper_node != a.root);
        }
        r_it++;
    }
    for (auto &x : root_start) {
        Node_ptr n = x.first;
        root_span[n] += a.sequence_length - x.second;
    }
    all_root_nodes.resize(root_span.size());
    all_root_spans.resize(root_span.size());
    int index = 0;
    for (auto &x : root_span) {
        all_root_nodes[index] = x.first;
        all_root_spans[index] = x.second;
        index++;
    }
}

void Normalizer::get_node_span(ARG &a) {
    map<Node_ptr, float, compare_node> node_start = {};
    map<Node_ptr, float, compare_node> node_span = {};
    for (const Branch &b : a.recombinations.begin()->second.inserted_branches) {
        node_start[b.upper_node] = 0;
    }
    node_start.erase(a.root);
    auto r_it = next(a.recombinations.begin());
    while (next(r_it) != a.recombinations.end()) {
        Node_ptr dn = r_it->second.deleted_node;
        Node_ptr in = r_it->second.inserted_node;
        assert(node_start.count(dn) > 0);
        node_span[dn] += r_it->first - node_start[dn];
        node_start.erase(dn);
        node_start[in] = r_it->first;
        r_it++;
    }
    for (auto &x : node_start) {
        Node_ptr n = x.first;
        node_span[n] += a.sequence_length - node_start[n];
    }
    all_nodes.resize(node_span.size());
    all_spans.resize(node_span.size());
    int index = 0;
    for (auto &x : node_span) {
        all_nodes[index] = x.first;
        all_spans[index] = x.second;
        index++;
    }
}

void Normalizer::randomize_mutation_ages(ARG &a) {
    float lb, ub, m;
    for (auto &x : a.mutation_branches) {
        for (auto &y : x.second) {
            if (y.upper_node != a.root) {
                lb = y.lower_node->time;
                ub = y.upper_node->time;
                m = uniform_random()*(ub - lb) + lb;
                mutation_ages.insert(m);
            }
        }
    }
    mutation_ages.insert(INT_MAX);
}

void Normalizer::randomized_normalize(ARG &a) {
    get_root_span(a);
    get_node_span(a);
    randomize_mutation_ages(a);
    float active_lineages = accumulate(all_spans.begin(), all_spans.end(), 0);
    active_lineages += accumulate(all_root_spans.begin(), all_root_spans.end(), 0);
    float active_length = a.sequence_length;
    float mean_num_lineages = 0;
    float delta_k = 0;
    float prior_rate = 0;
    float theta = 0;
    float m = 0;
    float t = 0.0005;
    auto m_it = mutation_ages.begin();
    int j = 0;
    for (int i = 0; i < all_nodes.size(); i++) {
        Node_ptr n = all_nodes[i];
        m = 0;
        while (*m_it < n->time) {
            m += 1;
            m_it++;
        }
        // assert(active_lineages > 0);
        mean_num_lineages = active_lineages/active_length;
        theta = active_lineages*4e-4;
        t += max(m/theta, 1e-6f);
        all_nodes[i]->time = t;
        delta_k = all_spans[i]/active_lineages;
        prior_rate = mean_num_lineages*(mean_num_lineages - delta_k)/2/delta_k;
        active_lineages -= all_spans[i];
        if (n == all_root_nodes[j]) {
            active_lineages -= all_root_spans[j];
            active_length -= all_root_spans[j];
            j++;
        }
    }
    for (auto &x : a.recombinations) {
        x.second.start_time = -1;
    }
    a.heuristic_sample_recombinations();
}

void Normalizer::normalize(ARG &a) {
    get_root_span(a);
    get_node_span(a);
    partition_arg(a);
    count_mutations(a);
    auto c_it = mutation_counts.begin();
    float t = 0;
    int i = 0;
    while (c_it->first < INT_MAX) {
        float e = 4e-4*ls/num_windows;
        float o = c_it->second;
        float scale = o/e;
        while (i < all_nodes.size() and all_nodes[i]->time < next(c_it)->first) {
            all_nodes[i]->time = t + scale*(all_nodes[i]->time - c_it->first);
            i++;
        }
        t += scale*(next(c_it)->first - c_it->first);
        c_it++;
    }
    a.heuristic_sample_recombinations();
}

void Normalizer::partition_arg(ARG &a) {
    float active_lineages = accumulate(all_spans.begin(), all_spans.end(), 0);
    active_lineages += accumulate(all_root_spans.begin(), all_root_spans.end(), 0);
    float t = 0;
    int j = 0;
    for (int i = 0; i < all_spans.size(); i++) {
        Node_ptr n = all_nodes[i];
        ls += (n->time - t)*active_lineages;
        active_lineages -= all_spans[i];
        if (all_root_nodes[j] == n) {
            active_lineages -= all_root_spans[j];
            j++;
        }
        t = n->time;
    }
    float l = 0;
    float x = 0;
    t = 0;
    j = 0;
    active_lineages = accumulate(all_spans.begin(), all_spans.end(), 0);
    active_lineages += accumulate(all_root_spans.begin(), all_root_spans.end(), 0);
    for (int i = 0; i < all_spans.size(); i++) {
        Node_ptr n = all_nodes[i];
        l += (n->time - t)*active_lineages;
        while (l > ls/num_windows) {
            l -= ls/num_windows;
            x = n->time - l/active_lineages;
            mutation_counts[x] = 0;
        }
        active_lineages -= all_spans[i];
        if (all_root_nodes[j] == n) {
            active_lineages -= all_root_spans[j];
            j++;
        }
        t = n->time;
    }
}

void Normalizer::count_mutations(ARG &a) {
    mutation_counts[0] = 0;
    mutation_counts[INT_MAX] = 0;
    float lb, ub;
    for (auto &x : a.mutation_branches) {
        for (auto &y : x.second) {
            if (y.upper_node != a.root) {
                lb = y.lower_node->time;
                ub = y.upper_node->time;
                add_mutation(lb, ub);
            }
        }
    }
}

void Normalizer::add_mutation(float lb, float ub) {
    float x, y, l;
    auto it = mutation_counts.lower_bound(lb);
    while (it->first < ub) {
        x = it->first;
        y = next(it)->first;
        l = min(ub, y) - max(lb, x);
        mutation_counts[x] += l/(ub - lb);
        it++;
    }
}
