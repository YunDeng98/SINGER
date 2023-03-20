//
//  Parsimony_pruner.cpp
//  SINGER
//
//  Created by Yun Deng on 3/14/23.
//

#include "Parsimony_pruner.hpp"

Parsimony_pruner::Parsimony_pruner() {}

void Parsimony_pruner::set_match_map(map<float, float> mm) {
    match_map = mm;
}

void Parsimony_pruner::start_search(ARG &a, Node *n, float m) {
    int mismatch = 0;
    Tree tree = a.get_tree_at(m);
    for (Branch branch : tree.branches) {
        mismatch = count_mismatch(branch, n, m);
        if (mismatch == 0) {
            curr_mismatch[branch] = mismatch;
        }
    }
}

void Parsimony_pruner::extend(ARG &a, Node *n) {
    float x = find_minimum_match();
    start_search(a, n, x);
    extend_forward(a, n, x);
}

void Parsimony_pruner::mutation_forward(Node *n, float m) {
    int mismatch = 0;
    Branch branch = Branch();
    for (auto x : curr_mismatch) {
        branch = x.first;
        mismatch = x.second;
        if (mismatch > max_mismatch) {
            curr_mismatch.erase(branch);
        } else {
            mismatch += count_mismatch(branch, n, m);
            curr_mismatch[branch] = mismatch;
        }
    }
}

void Parsimony_pruner::recombination_forward(Recombination &r) {
    Branch branch = Branch();
    for (auto x : curr_mismatch) {
        branch = x.first;
        if (not r.affect(branch)) {
            transition_helper(branch, branch);
        } else if (branch == r.source_branch) {
            transition_helper(branch, r.recombined_branch);
            transition_helper(branch, r.merging_branch);
        } else if (branch == r.target_branch) {
            transition_helper(branch, r.lower_transfer_branch);
            transition_helper(branch, r.upper_transfer_branch);
            transition_helper(branch, r.recombined_branch);
        } else {
            transition_helper(branch, r.merging_branch);
        }
    }
    update_mismatch();
}

float Parsimony_pruner::find_minimum_match() {
    auto it = min_element(match_map.begin(), match_map.end(),
                          [](const auto& l, const auto& r) { return l.second < r.second;});
    return it->first;
}

float Parsimony_pruner::count_mismatch(Branch branch, Node *n, float m) {
    float s0 = n->get_state(m);
    float sl = branch.lower_node->get_state(m);
    float su = branch.upper_node->get_state(m);
    if (abs(sl - s0) > 0.5 and abs(su - s0) > 0.5) {
        return 1;
    } else {
        return 0;
    }
}

void Parsimony_pruner::transition_helper(Branch sb, Branch tb) {
    if (transitions.count(tb) > 0) {
        transitions[tb].insert(sb);
    } else {
        transitions[tb] = {sb};
    }
}

void Parsimony_pruner::update_mismatch() {
    map<Branch, float> prev_mismatch = curr_mismatch;
    map<Branch, float> next_mismatch = {};
    Branch branch = Branch();
    float mismatch_sum = 0;
    for (auto x : transitions) {
        branch = x.first;
        mismatch_sum = 0;
        for (Branch b : x.second) {
            mismatch_sum += prev_mismatch[b];
        }
        next_mismatch[branch] = mismatch_sum/x.second.size();
    }
    curr_mismatch = next_mismatch;
}

void Parsimony_pruner::extend_forward(ARG &a, Node *n, float x) {
    map<float, Recombination>::iterator recomb_it = a.recombinations.upper_bound(x);
    set<float>::iterator mut_it = a.mutation_sites.upper_bound(x);
    float curr_pos = x;
    float m = x;
    while (curr_mismatch.size() > 0) {
        curr_pos = recomb_it->first;
        Recombination r = recomb_it->second;
        recomb_it++;
        while (*mut_it < curr_pos) {
            m = *mut_it;
            mutation_forward(n, m);
            mut_it++;
        }
        recombination_forward(r);
    }
}
