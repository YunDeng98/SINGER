//
//  ARG.cpp
//  SINGER
//
//  Created by Yun Deng on 4/14/22.
//

#include "ARG.hpp"

ARG::ARG() {}

ARG::ARG(float N, float l) {
    root->set_index(-1);
    Ne = N;
    sequence_length = l;
    Recombination r = Recombination({}, {});
    r.set_pos(0.0);
    recombinations[0] = r;
    r = Recombination({}, {});
    r.set_pos(INT_MAX);
    recombinations[INT_MAX] = r;
    mutation_sites.insert(INT_MAX);
    mutation_branches[INT_MAX] = {};
}

ARG::~ARG() {
}

void ARG::discretize(float s) {
    map<float, Recombination>::iterator recomb_it = recombinations.upper_bound(0);
    float curr_pos = 0;
    while (curr_pos < sequence_length) {
        coordinates.push_back(max(curr_pos - 0.1f, 0.0f));
        if (recomb_it->first < curr_pos + s) {
            curr_pos = recomb_it->first;
            recomb_it++;
        } else {
            curr_pos = min(curr_pos + s, sequence_length);
        }
    }
    coordinates.push_back(sequence_length);
    bin_num = (int) coordinates.size() - 1;
}

int ARG::get_index(float x) {
    int left = 0;
    int right = (int) coordinates.size() - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (coordinates[mid] == x) {
            return mid;
        }
        if (coordinates[mid] < x) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return -1;
}

void ARG::compute_rhos_thetas(float r, float m) {
    int n = (int) coordinates.size() - 1;
    for (int i = 0; i < n; i++) {
        rhos.push_back(r*(coordinates[i+1] - coordinates[i]));
        thetas.push_back(m*(coordinates[i+1] - coordinates[i]));
    }
}

void ARG::build_singleton_arg(Node *n) {
    Branch branch = Branch(n, root);
    Recombination r = Recombination({}, {branch});
    r.set_pos(0.0);
    recombinations.insert({0, r});
}

void ARG::add_sample(Node *n) {
    sample_nodes.insert(n);
    add_node(n);
    set<float> mutations = n->mutation_sites;
    for (float x : mutations) {
        mutation_sites.insert(x);
    }
    removed_branches.clear();
    removed_branches[0] = Branch(n, root);
    removed_branches[sequence_length] = Branch(n, root);
}

void ARG::add_node(Node *n) {
    if (n != root) {
        node_set.insert(n);
    }
}

Node *ARG::add_new_node(float t) {
    if (t == numeric_limits<float>::infinity()) {
        return root;
    }
    Node *new_node = new Node(t);
    new_node->set_index((int) node_set.size());
    add_node(new_node);
    if (t == 0) {
        sample_nodes.insert(new_node);
    }
    return new_node;
}

Tree ARG::get_tree_at(float x) {
    Tree tree = Tree();
    map<float, Recombination>::iterator recomb_it = recombinations.begin();
    Recombination r;
    while (recomb_it->first <= x) {
        r = recomb_it->second;
        tree.forward_update(r);
        recomb_it++;
    }
    return tree;
}

void ARG::remove(tuple<float, Branch, float> cut_point) {
    float pos;
    Branch center_branch;
    float t;
    tie(pos, center_branch, t) = cut_point;
    cut_time = t;
    cut_node = new Node(cut_time);
    cut_node->set_index(-2);
    add_node(cut_node);
    Tree forward_tree = get_tree_at(pos);
    Tree backward_tree = forward_tree;
    map<float, Recombination>::iterator f_it = recombinations.upper_bound(pos);
    map<float, Recombination>::iterator b_it = recombinations.upper_bound(pos);
    Branch prev_joining_branch;
    Branch next_joining_branch;
    Branch prev_removed_branch = center_branch;
    Branch next_removed_branch = center_branch;
    pair<Branch, Node *> joining_point = {};
    while (next_removed_branch != Branch()) {
        Recombination &r = f_it->second;
        prev_joining_branch = forward_tree.find_joining_branch(prev_removed_branch);
        forward_tree.forward_update(r);
        next_removed_branch = r.trace_forward(t, prev_removed_branch);
        next_joining_branch = forward_tree.find_joining_branch(next_removed_branch);
        joining_point = {next_joining_branch, next_removed_branch.upper_node};
        r.remove(prev_removed_branch, next_removed_branch, prev_joining_branch, next_joining_branch, cut_node);
        removed_branches[min(r.pos, sequence_length)] = next_removed_branch;
        joining_branches[min(r.pos, sequence_length)] = next_joining_branch;
        f_it++;
        prev_removed_branch = next_removed_branch;
    }
    next_removed_branch = center_branch;
    prev_removed_branch = center_branch;
    while (prev_removed_branch != Branch()) {
        b_it--;
        Recombination &r = b_it->second;
        removed_branches[r.pos] = prev_removed_branch;
        next_joining_branch = backward_tree.find_joining_branch(next_removed_branch);
        joining_branches[r.pos] = next_joining_branch;
        backward_tree.backward_update(r);
        prev_removed_branch = r.trace_backward(t, next_removed_branch);
        prev_joining_branch = backward_tree.find_joining_branch(prev_removed_branch);
        r.remove(prev_removed_branch, next_removed_branch, prev_joining_branch, next_joining_branch, cut_node);
        next_removed_branch = prev_removed_branch;
    }
    remove_empty_recombinations();
    remap_mutations();
}

void ARG::remove(map<float, Branch> seed_branches) {
    // insights here: the coordinates of removed branches is the same as recombinations
    float start = removed_branches.begin()->first;
    float end = removed_branches.rbegin()->first;
    Tree tree = Tree();
    map<float, Recombination>::iterator recomb_it = recombinations.lower_bound(start);
    map<float, Branch>::iterator seed_it = seed_branches.begin();
    Branch prev_removed_branch = Branch();
    Branch next_removed_branch = Branch();
    Branch prev_joining_branch = Branch();
    Branch next_joining_branch = Branch();
    while (recomb_it->first < end) {
        next_removed_branch = seed_it->second;
        seed_it++;
        Recombination &r = recomb_it->second;
        tree.forward_update(r);
        next_joining_branch = tree.find_joining_branch(next_removed_branch);
        recomb_it++;
        r.remove(prev_removed_branch, next_removed_branch, prev_joining_branch, next_joining_branch);
        removed_branches[r.pos] = next_removed_branch;
        joining_branches[r.pos] = next_joining_branch;
        prev_removed_branch = next_removed_branch;
        prev_joining_branch = next_joining_branch;
    }
    removed_branches[end] = next_removed_branch;
    joining_branches[end] = next_joining_branch;
    remove_empty_recombinations();
    remap_mutations();
}

void ARG::remove_leaf(int index) {
    Node *s = nullptr;
    for (Node *n : sample_nodes) {
        if (n->index == index) {
            s = n;
        }
    }
    Tree tree = Tree();
    map<float, Recombination>::iterator recomb_it = recombinations.begin();
    Branch removed_branch = Branch();
    Node *joining_node = nullptr;
    map<float, Branch> removed_branches = {};
    while (recomb_it->first < sequence_length) {
        Recombination r = recomb_it->second;
        tree.forward_update(r);
        recomb_it++;
        joining_node = tree.parents[s];
        removed_branch = Branch(s, joining_node);
        removed_branches[r.pos] = removed_branch;
    }
    remove(removed_branches);
}

void ARG::add(map<float, Branch> new_joining_branches, map<float, Branch> added_branches) {
    for (auto x : added_branches) {
        add_node(x.second.upper_node);
    }
    float start = removed_branches.begin()->first;
    float end = removed_branches.rbegin()->first;
    map<float, Branch>::iterator join_it = new_joining_branches.begin();
    map<float, Branch>::iterator add_it = added_branches.begin();
    map<float, Recombination>::iterator recomb_it = recombinations.lower_bound(start);
    Branch prev_joining_branch = Branch();
    Branch next_joining_branch = Branch();
    Branch prev_added_branch = Branch();
    Branch next_added_branch = Branch();
    while (join_it->first < end) {
        if (recomb_it->first == join_it->first) {
            Recombination r = recomb_it->second;
            recomb_it++;
            next_joining_branch = join_it->second;
            join_it++;
            next_added_branch = add_it->second;
            add_it++;
            r.add(prev_added_branch, next_added_branch, prev_joining_branch, next_joining_branch, cut_node);
            prev_joining_branch = next_joining_branch;
            prev_added_branch = next_added_branch;
        } else {
            next_joining_branch = join_it->second;
            join_it++;
            next_added_branch = add_it->second;
            add_it++;
            new_recombination(add_it->first, prev_added_branch, prev_joining_branch, next_added_branch, next_joining_branch);
        }
    }
    remove_empty_recombinations();
    impute_nodes(start, end);
}

void ARG::smc_sample_recombinations() {
    RSP_smc rsp = RSP_smc();
    Tree tree = Tree();
    for (auto &x : recombinations) {
        if (x.first != 0 and x.first < sequence_length) {
            rsp.sample_recombination(x.second, cut_time, tree);
        }
        tree.forward_update(x.second);
    }
}

int ARG::count_incompatibility() {
    Tree tree = Tree();
    map<float, Recombination>::iterator recomb_it = recombinations.begin();
    set<float>::iterator mut_it = mutation_sites.begin();
    float start = 0;
    float end = 0;
    float x = 0;
    int count = 0;
    for (int i = 0; i < bin_num; i++) {
        start = coordinates[i];
        end = coordinates[i+1];
        if (start  == recomb_it->first) {
            Recombination r = recomb_it->second;
            tree.forward_update(r);
            recomb_it++;
        }
        while (*mut_it < end) {
            x = *mut_it;
            count += count_incompatibility(tree, x);
            mut_it++;
        }
    }
    return count;
}

void ARG::write(string node_file, string branch_file, string recomb_file) {
    sort_nodes();
    write_nodes(node_file);
    write_branches(branch_file);
    write_recombs(recomb_file);
}

void ARG::read(string node_file, string branch_file) {
    read_nodes(node_file);
    read_branches(branch_file);
}

void ARG::read(string node_file, string branch_file, string recomb_file) {
    read_nodes(node_file);
    read_branches(branch_file);
    read_recombs(recomb_file);
}

// private methods:

void ARG::impute_nodes(float x, float y) {
    Tree start_tree = get_tree_at(x);
    Branch null_branch = Branch();
    Fitch_reconstruction rc = Fitch_reconstruction(start_tree);
    map<float, Recombination>::iterator recomb_it = recombinations.upper_bound(x);
    set<float>::iterator mut_it = mutation_sites.lower_bound(x);
    float curr_pos = x;
    float m = 0;
    while (curr_pos < y) {
        curr_pos = recomb_it->first;
        while (*mut_it < curr_pos) {
            m = *mut_it;
            rc.reconstruct(m);
            mut_it++;
        }
        rc.update(recomb_it->second);
        recomb_it++;
    }
    return;
}

void ARG::impute(map<float, Branch> new_joining_branches, map<float, Branch> added_branches) {
    float start = added_branches.begin()->first;
    float end = added_branches.rbegin()->first;
    set<float>::iterator mut_it = mutation_sites.lower_bound(start);
    map<float, Branch>::iterator join_it = new_joining_branches.begin();
    map<float, Branch>::iterator add_it = added_branches.begin();
    float m = 0, sl = 0, su = 0, s0 = 0, sm = 0;
    Branch joining_branch = Branch();
    Branch added_branch = Branch();
    while (*mut_it < end) {
        m = *mut_it;
        while (join_it->first < m) {
            join_it++;
            add_it++;
        }
        joining_branch = join_it->second;
        added_branch = add_it->second;
        sl = joining_branch.lower_node->get_state(m);
        su = joining_branch.upper_node->get_state(m);
        s0 = added_branch.lower_node->get_state(m);
        if (sl + su + s0 > 1) {
            sm = 1;
        } else {
            sm = 0;
        }
        added_branch.upper_node->write_state(m, sm);
    }
}

void ARG::map_mutations(float x, float y) {
    Tree tree = get_tree_at(x);
    map<float, Recombination>::iterator recomb_it = recombinations.upper_bound(x);
    set<float>::iterator mut_it = mutation_sites.lower_bound(x);
    float m = *mut_it;
    while (*mut_it < y) {
        m = *mut_it;
        while (recomb_it->first < m) {
            Recombination r = recomb_it->second;
            tree.forward_update(r);
            recomb_it++;
        }
        map_mutation(tree, m);
        mut_it++;
    }
}

void ARG::remap_mutations() {
    float x = joining_branches.begin()->first;
    float y = joining_branches.rbegin()->first;
    map<float, set<Branch>>::iterator mut_it = mutation_branches.lower_bound(x);
    map<float, Branch>::iterator join_it = joining_branches.begin();
    map<float, Branch>::iterator remove_it = removed_branches.begin();
    Node *joining_node = nullptr;
    Branch joining_branch = Branch();
    Branch removed_branch = Branch();
    Branch lower_branch = Branch();
    Branch upper_branch = Branch();
    while (mut_it->first < y) {
        while (join_it->first < mut_it->first) {
            joining_branch = join_it->second;
            join_it++;
        }
        while (remove_it->first < mut_it->first) {
            removed_branch = remove_it->second;
            joining_node = removed_branch.upper_node;
            remove_it++;
        }
        lower_branch = Branch(joining_branch.lower_node, joining_node);
        upper_branch = Branch(joining_node, joining_branch.upper_node);
        if (mut_it->second.count(removed_branch) > 0) {
            mut_it->second.erase(removed_branch);
        }
        if (mut_it->second.count(lower_branch) > 0 or mut_it->second.count(upper_branch) > 0) {
            mut_it->second.erase(lower_branch);
            mut_it->second.erase(upper_branch);
            mut_it->second.insert(joining_branch);
        }
        mut_it++;
    }
}

void ARG::map_mutation(Tree tree, float x) {
    set<Branch> branches = {};
    float sl = 0;
    float su = 0;
    for (Branch b : tree.branches) {
        sl = b.lower_node->get_state(x);
        su = b.upper_node->get_state(x);
        if (sl != su) {
            branches.insert({b});
        }
    }
    mutation_branches[x] = branches;
}

void ARG::clear_memory() {
    set<Node*, compare_node> reduced_node_set = {};
    set<Node *> deleted_nodes = {};
    Recombination r = recombinations.begin()->second;
    for (Branch b : r.inserted_branches) {
        reduced_node_set.insert(b.lower_node);
    }
    for (auto x : recombinations) {
        if (x.first > 0 and x.first < sequence_length) {
            reduced_node_set.insert(x.second.inserted_node);
        }
    }
    for (Node *n : node_set) {
        if (reduced_node_set.count(n) == 0 and n->index != -1) {
            deleted_nodes.insert(n);
        }
    }
    for (Node *n : deleted_nodes) {
        delete n;
    }
    node_set.clear();
    node_set = reduced_node_set;
}

void ARG::check_mapping() {
    for (Node *n : node_set) {
        assert(n->ambiguous_sites.size() == 0);
    }
}

void ARG::clear_memory(map<float, Branch> added_branches) {
    Node *node = nullptr;
    for (auto x : added_branches) {
        if (x.second.upper_node != node) {
            node = x.second.upper_node;
            if (node_set.count(node) == 0) {
                delete node;
            }
        }
    }
}

void ARG::clear_remove_info() {
    removed_branches.clear();
    joining_branches.clear();
}
 
float ARG::smc_prior_likelihood(float r) {
    Tree tree = get_tree_at(0);
    float rho = 0;
    float log_likelihood = 0;
    log_likelihood += tree.prior_likelihood();
    float tree_length = tree.length();
    map<float, Recombination>::iterator recomb_it = recombinations.upper_bound(0);
    float bin_start = 0;
    float bin_end = 0;
    for (int i = 0; i < bin_num; i++) {
        bin_start = coordinates[i];
        bin_end = coordinates[i+1];
        rho = (bin_end - bin_start)*r*Ne;
        if (bin_start == recomb_it->first) {
            Recombination r = recomb_it->second;
            recomb_it++;
            log_likelihood -= rho*tree_length;
            log_likelihood += log(rho*tree_length);
            log_likelihood += tree.transition_likelihood(r);
            tree.forward_update(r);
            tree_length = tree.length();
            assert(tree_length > 0);
        } else {
            log_likelihood -= rho*tree_length;
        }
        assert(!isnan(log_likelihood));
    }
    return log_likelihood;
}

float ARG::data_likelihood(float m) {
    impute_nodes(0, bin_num);
    float theta = 0;
    float log_likelihood = 0;
    Tree tree = get_tree_at(0);
    map<float, Recombination>::iterator recomb_it = recombinations.upper_bound(0);
    set<float>::iterator mut_it = mutation_sites.begin();
    float bin_start = 0;
    float bin_end = 0;
    float prev_mut_pos = 0;
    float next_mut_pos = 0;
    for (int i = 0; i < bin_num; i++) {
        bin_start = coordinates[i];
        bin_end = coordinates[i+1];
        if (bin_start == recomb_it->first) {
            Recombination r = recomb_it->second;
            recomb_it++;
            tree.forward_update(r);
        }
        while (*mut_it < bin_end) {
            next_mut_pos = *mut_it;
            theta = m*Ne;
            log_likelihood += tree.data_likelihood(theta, next_mut_pos);
            theta = (next_mut_pos - prev_mut_pos - 1)*m*Ne;
            log_likelihood += tree.null_likelihood(theta);
            prev_mut_pos = next_mut_pos;
        }
    }
    return log_likelihood;
}

float ARG::smc_likelihood(float r, float m) {
    return smc_prior_likelihood(r) + data_likelihood(m);
}

set<float> ARG::get_check_points() {
    float start_pos = removed_branches.begin()->first;
    float end_pos = removed_branches.rbegin()->first;
    map<float, Recombination>::iterator recomb_it = recombinations.lower_bound(start_pos);
    map<Node *, float> deleted_nodes = {};
    vector<tuple<Node *, float, float>> node_span = {};
    while (recomb_it->first <= end_pos) {
        Recombination r = recomb_it->second;
        deleted_nodes[r.deleted_node] = r.pos;
        Node *inserted_node = r.inserted_node;
        if (deleted_nodes.count(inserted_node) > 0 and inserted_node != root) {
            node_span.push_back({inserted_node, deleted_nodes.at(inserted_node), r.pos});
            deleted_nodes.erase(inserted_node);
        }
        recomb_it++;
    }
    Node *n;
    float x;
    float y;
    set<float> check_points = {};
    for (auto ns : node_span) {
        tie(n, x, y) = ns;
        if (!check_disjoint_nodes(x, y)) {
            check_points.insert(y);
        }
    }
    return check_points;
}

bool ARG::check_disjoint_nodes(float x, float y) {
    map<float, Recombination>::iterator recomb_it = recombinations.lower_bound(x);
    float t = recomb_it->second.deleted_node->time;
    Branch b = recomb_it->second.merging_branch;
    while (recomb_it->first < y) {
        b = recomb_it->second.trace_forward(t, b);
        if (b == Branch()) {
            return false;
        }
        recomb_it++;
    }
    if (b != recomb_it->second.target_branch) {
        return false;
    }
    return true;
}

// private methods:

void ARG::new_recombination(float pos, Branch prev_added_branch, Branch prev_joining_branch, Branch next_added_branch, Branch next_joining_branch) {
    set<Branch> deleted_branches;
    set<Branch> inserted_branches;
    deleted_branches.insert(prev_added_branch);
    deleted_branches.insert(Branch(prev_joining_branch.lower_node, prev_added_branch.upper_node));
    deleted_branches.insert(Branch(prev_added_branch.upper_node, prev_joining_branch.upper_node));
    deleted_branches.insert(next_joining_branch);
    inserted_branches.insert(next_added_branch);
    inserted_branches.insert(Branch(next_joining_branch.lower_node, next_added_branch.upper_node));
    inserted_branches.insert(Branch(next_added_branch.upper_node, next_joining_branch.upper_node));
    inserted_branches.insert(prev_joining_branch);
    Recombination r = Recombination(deleted_branches, inserted_branches);
    r.set_pos(pos);
    recombinations.insert({pos, r});
    return;
}

float ARG::random() {
    return (float) rand()/RAND_MAX;
}

void ARG::remove_empty_recombinations() {
    map<float, Recombination>::iterator recomb_it = recombinations.begin();
    while (recomb_it != recombinations.end()) {
        Recombination r = recomb_it->second;
        if (r.deleted_branches.size() == 0 and r.inserted_branches.size() == 0 and r.pos < sequence_length) {
            recomb_it = recombinations.erase(recomb_it);
        } else {
            ++recomb_it;
        }
    }
}

int ARG::count_incompatibility(Tree tree, float x) {
    int count = -1;
    for (Branch b : tree.branches) {
        if (b.upper_node->index > 0) {
            int i1 = b.upper_node->get_state(x);
            int i2 = b.lower_node->get_state(x);
            if (i1 != i2) {
                count += 1;
            }
        }
    }
    return max(0, count);
}

void ARG::sort_nodes() {
    int index = 0;
    for (Node *n : node_set) {
        n->set_index(index);
        index += 1;
    }
}

void ARG::write_nodes(string filename) {
    ofstream file;
    file.open(filename);
    int index = 0;
    for (Node *n : node_set) {
        n->set_index(index);
        file << setprecision(6) << n->time*Ne << "\n";
        index += 1;
    }
    file.close();
}

void ARG::write_branches(string filename) {
    map<Branch, int> branch_map;
    vector<tuple<int, int, float, float>> branch_info;
    int pos;
    for (auto x : recombinations) {
        if (x.first < sequence_length) {
            pos = x.first;
            Recombination r = x.second;
            for (Branch b : r.inserted_branches) {
                branch_map.insert({b, pos});
            }
            for (Branch b : r.deleted_branches) {
                assert((node_set.count(b.lower_node) > 0 and node_set.count(b.upper_node) > 0) or b.upper_node == root);
                int k1 = b.upper_node->index;
                int k2 = b.lower_node->index;
                assert(k1 < 1e4 and k2 < 1e4);
                branch_info.push_back({k1, k2, branch_map.at(b), pos});
                branch_map.erase(b);
            }
        }
    }
    for (auto x : branch_map) {
        Branch b = x.first;
        assert((node_set.count(b.lower_node) > 0 and node_set.count(b.upper_node) > 0) or b.upper_node == root);
        int k1 = b.upper_node->index;
        int k2 = b.lower_node->index;
        assert(k1 < 1e4 and k2 < 1e4);
        branch_info.push_back({k1, k2, x.second, sequence_length});
    }
    sort(branch_info.begin(), branch_info.end(), compare_edge);
    ofstream file;
    file.open(filename);
    for (int i = 0; i < branch_info.size(); i++) {
        auto [k1, k2, x, l] = branch_info[i];
        assert(k1 < 1e4 and k2 < 1e4);
        file << x << " " << l << " " << k1 << " " << k2 << "\n";
    }
    file.close();
}

void ARG::write_recombs(string filename) {
    ofstream file;
    file.open(filename);
    for (auto x : recombinations) {
        Recombination r = x.second;
        if (x.first > 0 and x.first < bin_num) {
            file << r.pos << " " << r.source_branch.lower_node->index << " " << r.source_branch.upper_node->index << " " << Ne*r.start_time << endl;
        }
    }
}

void ARG::read_nodes(string filename) {
    root->set_index(-1);
    node_set.clear();
    ifstream fin(filename);
    if (!fin.good()) {
        cerr << "input file not found" << endl;
        exit(1);
    }
    float x;
    while (fin >> x) {
        add_new_node(x/Ne);
    }
}

void ARG::read_branches(string filename) {
    ifstream fin(filename);
    if (!fin.good()) {
        cerr << "input file not found" << endl;
        exit(1);
    }
    vector<Node*> nodes = vector<Node*>(node_set.begin(), node_set.end());
    float x;
    float y;
    float p;
    float c;
    float left;
    float right;
    Node *parent_node;
    Node *child_node;
    Branch b;
    map<float, set<Branch>> deleted_branches = {{0, {}}};
    map<float, set<Branch>> inserted_branches = {};
    while (fin >> x >> y >> p >> c) {
        left = x;
        right = y;
        if (p < 0) {
            parent_node = root;
        } else {
            parent_node = nodes[int(p)];
        }
        child_node = nodes[int(c)];
        b = Branch(child_node, parent_node);
        if (deleted_branches.count(right) > 0) {
            deleted_branches.at(right).insert(b);
        } else {
            deleted_branches.insert({right, {b}});
        }
        if (inserted_branches.count(left) > 0) {
            inserted_branches.at(left).insert(b);
        } else {
            inserted_branches.insert({left, {b}});
        }
    }
    deleted_branches.erase(sequence_length);
    for (auto x : deleted_branches) {
        float pos = x.first;
        set<Branch> db = deleted_branches.at(pos);
        set<Branch> ib = inserted_branches.at(pos);
        Recombination r = Recombination(db, ib);
        r.set_pos(pos);
        recombinations[pos] = r;
    }
}

void ARG::read_recombs(string filename) {
    ifstream fin(filename);
    if (!fin.good()) {
        cerr << "input file not found" << endl;
        exit(1);
    }
    vector<Node*> nodes = vector<Node*>(node_set.begin(), node_set.end());
    map<int, Branch> source_branches = {};
    map<int, float> start_times = {};
    float pos;
    int n1;
    int n2;
    float t;
    Node *ln;
    Node *un;
    Branch b;
    while (fin >> pos >> n1 >> n2 >> t) {
        ln = nodes[n1];
        if (n2 == -1) {
            un = root;
        } else {
            un = nodes[n2];
        }
        b = Branch(nodes[n1], nodes[n2]);
        source_branches.insert({pos, b});
        start_times.insert({pos, t});
    }
    for (auto &x : recombinations) {
        pos = x.first;
        if (pos > 0 and pos < sequence_length) {
            t = start_times.at(pos);
            b = source_branches.at(pos);
            if (pos > 0 and pos < sequence_length) {
                x.second.start_time = t;
                x.second.source_branch = b;
                x.second.find_nodes();
                x.second.find_target_branch();
                x.second.find_recomb_info();
            }
        }
    }
}

float ARG::get_arg_length() {
    Tree tree = Tree();
    map<float, Recombination>::iterator recomb_it = recombinations.begin();
    Recombination r;
    float l = 0;
    float tree_length = 0;
    float curr_pos = 0;
    while (curr_pos < sequence_length) {
        
    }
    for (int i = 0; i < bin_num; i++) {
        if (recomb_it->first == i) {
            r = recomb_it->second;
            recomb_it++;
            tree.forward_update(r);
            tree_length = tree.length();
        }
        l += tree_length;
    }
    return l;
}

float ARG::get_arg_length(float x, float y) {
    Tree tree = get_tree_at(x);
    map<float, Recombination>::iterator recomb_it = recombinations.upper_bound(x);
    Recombination r;
    float l = 0;
    float tree_length = tree.length();
    float prev_pos = x;
    float next_pos = x;
    while (next_pos <= y) {
        next_pos = recomb_it->first;
        l += tree_length*(next_pos - prev_pos);
        r = recomb_it->second;
        recomb_it++;
        tree.forward_update(r);
        tree_length = tree.length();
    }
    return l;
}

tuple<float, Branch, float> ARG::sample_internal_cut() {
    float arg_length = get_arg_length();
    float p = random();
    p = 0.01 + 0.98*p; // smooth p away from extremely values
    float l = arg_length*p;
    Tree tree = Tree();
    map<float, Recombination>::iterator recomb_it = recombinations.begin();
    float tree_length = 0;
    Recombination r;
    Branch branch;
    float prev_pos = 0;
    float next_pos = 0;
    float cut_pos = 0;
    float time;
    while (next_pos < sequence_length) {
        r = recomb_it->second;
        recomb_it++;
        next_pos = recomb_it->first;
        tree.forward_update(r);
        tree_length = tree.length();
        l -= tree_length*(next_pos - prev_pos);
        if (l < 0) {
            cut_pos = next_pos + l/tree_length;
            tie(branch, time) = tree.sample_cut_point();
            return {cut_pos, branch, time};
        }
        prev_pos = next_pos;
    }
    cerr << "sample internal cut failed" << endl;
    exit(1);
}

tuple<float, Branch, float> ARG::sample_terminal_cut() {
    Branch branch;
    float time = 1e-10;
    vector<Node *> nodes = vector<Node *>(sample_nodes.begin(), sample_nodes.end());
    int index = rand() % nodes.size();
    Node *terminal_node = nodes[index];
    Tree start_tree = get_tree_at(0);
    for (Branch b : start_tree.branches) {
        if (b.lower_node == terminal_node) {
            branch = b;
            break;
        }
    }
    return {0, branch, time};
}

bool compare_edge(const tuple<int, int, float, float>& edge1, const tuple<int, int, float, float>& edge2) {
    if (get<0>(edge1) < get<0>(edge2)) {
        return true;
    } else if (get<0>(edge1) > get<0>(edge2)) {
        return false;
    }
    if (get<1>(edge1) < get<1>(edge2)) {
        return true;
    } else if (get<1>(edge1) > get<1>(edge2)) {
        return false;
    }
    return get<2>(edge1) < get<2>(edge2);
}
