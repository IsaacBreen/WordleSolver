#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <stack>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <variant>
#include <type_traits>
#include <utility>
#include <iomanip>
#include <sstream>
#include <bitset>
#include <numeric>
#include <functional>
#include <iterator>
#include <time.h>
#include <stdexcept>

#include <common.hpp>
#include <data/data.hpp>
#include <hint.hpp>
#include <compatibility.hpp>
#include <entropy.hpp>

// Only include OpenMP if it is available
#ifdef _OPENMP
#include <omp.h>
#endif

#include <iostream>
using namespace std;

#pragma once

#define PRINT_LEVELS 4
#define SPARSITY_THRESHOLD 0.1
#define SMALL_WORDLIST 100
#define MEDIUM_WORDLIST 100
#define LARGE_WORDLIST 1000
#define SMALL_GUESSLIST 100
#define WORDLIST_SIZE_PRINT_THRESHOLD 20
#define MAX_TURNS 6
#define PRECOMPUTE_LEVEL 5
#define USELESS_GUESS_ELIMINATION_LEVEL 3
#define SAMPLE_HEURISTIC false
// #define MIN_DIGIT_PADDING 2

bool is_poisoned(Strategy& strategy) {
    return strategy.is_poisoned();
}

template<typename T>
auto maybe_sparsify(T& wordlist) {
    // If wordlist is DenseWordlist and its density is less than SPARSITY_THRESHOLD, convert it to SparseWordlist
    if constexpr (not is_same<T, SparseWordlist>::value) {
        if (wordlist.density() < SPARSITY_THRESHOLD) {
            return variant<T, SparseWordlist>(SparseWordlist(wordlist));
        } else {
           return variant<T, SparseWordlist>(wordlist);
        }
    } else {
        return variant<T>(wordlist);
    }
}

template<long N>
auto sparsify(DenseWordlist<N>& wordlist) {
    return SparseWordlist(wordlist);
}

auto sparsify(SparseWordlist& wordlist) {
    return wordlist;
}


template<typename T>
string sparsity_string(T& wordlist) {
    if constexpr (is_same<T, SparseWordlist>::value) {
        return "sparse";
    } else {
        return "dense";
    }
}

class OptimizerConfig {
public:
    int max_turns = 6;
    int precompute_level = 5;
    bool sample_heuristic = false;
    bool verbose = false;
    float max_Exp_turns_remaining_stop = BIG_NUMBER;
    int turn = 1;
    int optimisation_level = 10;
};

string to_string(OptimizerConfig& config) {
    stringstream ss;
    ss << "OptimizerConfig(";
    ss << "max_turns=" << config.max_turns << ", ";
    ss << "precompute_level=" << config.precompute_level << ", ";
    ss << "sample_heuristic=" << config.sample_heuristic << ", ";
    ss << "verbose=" << config.verbose << ", ";
    ss << "max_Exp_turns_remaining_stop=" << config.max_Exp_turns_remaining_stop << ", ";
    ss << "turn=" << config.turn << ", ";
    ss << "optimisation_level=" << config.optimisation_level;
    ss << ")";
    return ss.str();
}

template<typename WL>
class Node {
public:
    Word word;
    Guess guess;
    vector<Node> children;
    float score;
};

template<typename WL, typename GL>
auto _find_optimal_strategy(WL& wordlist, GL& guesslist, OptimizerConfig config) {

}

template<typename WL, typename GL>
Strategy find_optimal_strategy(WL& wordlist, GL& guesslist, bool verbose=true) {
    OptimizerConfig config;
    config.verbose = verbose;
    return _find_optimal_strategy(wordlist, guesslist, config);
}

template<typename WL, typename GL>
float estimate_execution_time_find_optimal_strategy(WL wordlist, GL guesslist) {
    // Estimate the execution time of _find_optimal_strategy() by running it on random words/guesses
    const auto num_samples = 2;
    auto t0 = chrono::high_resolution_clock::now();
    for (int i = 0; i < num_samples; i++) {
        auto t00 = chrono::high_resolution_clock::now();
        Word word = rand() % wordlist.size();
        Guess guess = rand() % guesslist.size();
        auto wordlist_remaining = get_compatible_words(guess, get_hint(word, guess), wordlist);
        // auto SparseWordlist(wordlist_remaining);
        cout << "Estimating execution time. Trying " << get_guess(guess) << " on " << get_word(word) << " with " << wordlist_remaining.size() << " words remaining." << endl;
        find_optimal_strategy(wordlist_remaining, guesslist);
        auto t01 = chrono::high_resolution_clock::now();
        cout << "Done. Took " << chrono::duration_cast<chrono::milliseconds>(t01 - t00).count() << "ms." << endl;
    }
    auto t1 = chrono::high_resolution_clock::now();
    auto num_pairs = wordlist.size() * guesslist.size();
    return chrono::duration_cast<chrono::seconds>(t1 - t0).count() / num_samples * num_pairs;
}