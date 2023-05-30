//
//  Coalescent_calculator.cpp
//  SINGER
//
//  Created by Yun Deng on 4/17/23.
//

#include "Coalescent_calculator.hpp"

Coalescent_calculator::Coalescent_calculator(float t) {
    cut_time = t;
}

Coalescent_calculator::~Coalescent_calculator() {}

void Coalescent_calculator::compute(set<Branch> &branches) {
    compute_rate_changes(branches);
    compute_rates();
    assert(rates.size() == rate_changes.size());
    compute_probs_quantiles();
    assert(rates.size() == rate_changes.size());
}

float Coalescent_calculator::weight(float lb, float ub) {
    float p = prob(ub) - prob(lb);
    assert(!isnan(p));
    return p;
}

float Coalescent_calculator::time(float lb, float ub) {
    float lq = prob(lb);
    float uq = prob(ub);
    float mid = 0;
    float t;
    if (isinf(ub)) {
        return lb + log(2);
    }
    if (ub - lb < 1e-3 or uq - lq < 1e-3) {
        t = 0.5*(lb + ub);
    } else {
        mid = 0.5*(lq + uq);
        t = quantile(mid);
    }
    assert(t >= lb and t <= ub);
    return t;
}

void Coalescent_calculator::compute_rate_changes(set<Branch> &branches) {
    rate_changes.clear();
    float lb = 0;
    float ub = 0;
    for (Branch b : branches) {
        lb = max(cut_time, b.lower_node->time);
        ub = b.upper_node->time;
        rate_changes[lb] += 1;
        rate_changes[ub] -= 1;
    }
    min_time = rate_changes.begin()->first;
    max_time = rate_changes.rbegin()->first;
}

void Coalescent_calculator::compute_rates() {
    rates.clear();
    int curr_rate = 0;
    for (auto &x : rate_changes) {
        curr_rate += x.second;
        rates[x.first] = curr_rate;
    }
    assert(rates.size() == rate_changes.size());
}

void Coalescent_calculator::compute_probs_quantiles() {
    probs.clear();
    quantiles.clear();
    int curr_rate = 0;
    float prev_time = 0.0;
    float next_time = 0.0;
    float prev_prob = 1.0;
    float next_prob = 1.0;
    float cum_prob = 0.0;
    for (auto it = rates.begin(); it != prev(rates.end()); it++) {
        curr_rate = it->second;
        prev_time = it->first;
        next_time = next(it)->first;
        if (curr_rate > 0) {
            next_prob = prev_prob*exp(-curr_rate*(next_time - prev_time));
            cum_prob += (prev_prob - next_prob)/curr_rate;
        } else {
            next_prob = prev_prob;
        }
        assert(cum_prob <= 1.0001);
        probs.insert({next_time, cum_prob});
        quantiles.insert({next_time, cum_prob});
        prev_prob = next_prob;
    }
    probs.insert({min_time, 0});
    quantiles.insert({min_time, 0});
    assert(probs.size() == rates.size());
    assert(quantiles.size() == rates.size());
}

float Coalescent_calculator::prob(float x) {
    if (x > max_time) {
        x = max_time;
    } else if (x < min_time) {
        x = min_time;
    }
    pair<float, float> query = {x, -1.0f};
    auto it = probs.find(query);
    if (it != probs.end()) {
        return it->second;
    }
    auto u_it = probs.upper_bound(query);
    auto l_it = probs.upper_bound(query);
    l_it--;
    float base_prob = l_it->second;
    int rate = rates[l_it->first];
    if (rate == 0) {
        return base_prob;
    }
    float delta_t = u_it->first - l_it->first;
    float delta_p = u_it->second - l_it->second;
    float new_delta_t = x - l_it->first;
    // float new_delta_p = delta_p*(1 - exp(-rate*new_delta_t))/(1 - exp(-rate*delta_t));
    float new_delta_p = delta_p*expm1(-rate*new_delta_t)/expm1(-rate*delta_t);
    assert(!isnan(new_delta_p));
    return base_prob + new_delta_p;
}

float Coalescent_calculator::quantile(float p) {
    pair<float, float> query = {-1.0f, p};
    auto u_it = quantiles.upper_bound(query);
    auto l_it = quantiles.upper_bound(query);
    l_it--;
    float base_time = l_it->first;
    int rate = rates[l_it->first];
    float delta_t = u_it->first - l_it->first;
    float delta_p = u_it->second - l_it->second;
    float new_delta_p = p - l_it->second;
    float new_delta_t = 1 - new_delta_p/delta_p*(1 - exp(-rate*delta_t));
    new_delta_t = -log(new_delta_t)/rate;
    assert(!isnan(new_delta_t));
    return base_time + new_delta_t;
}
