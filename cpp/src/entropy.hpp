#include <iostream>
#include <string>
#include <vector>
#include <array>
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
#include <utils.hpp>

// Only include OpenMP if it is available
#ifdef _OPENMP
#include <omp.h>
#endif

#include <iostream>
using namespace std;

#pragma once

#define PRINT_LEVELS 5
#define SPARSITY_THRESHOLD 0.1
#define SPACED_ENDL "    " << endl
#define MAX_TURNS 6
#define PRECOMPUTE_LEVEL 5
// #define MIN_DIGIT_PADDING 2

// Template function that evaluates an expectation of the given function over the words in the wordlist
template<typename WL>
float information_gain(Guess guess, Word word, WL& wordlist) {
    float entropy_before = log2(wordlist.size());
    float entropy_after = 0;
    Hint hint = get_hint(word, guess);
    entropy_after += log2(num_compatible_words(guess, hint, wordlist));
    return entropy_before - entropy_after;
}

template<typename WL, typename GL>
tuple<Guess, float> highest_information_gain_guess(Word word, WL& wordlist, GL& guesslist) {
    float max_ig = 0;
    Guess max_ig_guess;
    for (auto guess : guesslist) {
        float ig = information_gain(guess, word, wordlist);
        if (ig > max_ig) {
            max_ig = ig;
            max_ig_guess = guess;
        }
    }
    return make_tuple(max_ig_guess, max_ig);
}

template<typename WL>
float expected_information_gain(Guess guess, WL& wordlist) {
    return evaluate_expectation([&](Word word) {
        return information_gain(guess, word, wordlist);
    }, wordlist);
}

template<typename WL, typename GL>
tuple<Guess, float> highest_expected_information_gain_guess(WL& wordlist, GL& guesslist) {
    float max_ig = 0;
    Guess max_ig_guess = -1;
    for (auto guess : guesslist) {
        // Use a lambda function to evaluate the expected information gain of this guess
        float ig = evaluate_expectation([&](Word word) {
            return information_gain(guess, word, wordlist);
        }, wordlist);
        if (ig > max_ig) {
            max_ig = ig;
            max_ig_guess = guess;
        }
    }
    return make_tuple(max_ig_guess, max_ig);
}

template<typename WL, typename GL>
Strategy highest_expected_information_gain_strategy(WL& wordlist, GL& guesslist, bool verbose=true, bool parallel=false) {
    if (wordlist.size() == NUM_WORDS) {
        return Strategy(get_guess_index("soare"), 2.54);
    }
    auto [best_guess, best_ig] = highest_expected_information_gain_guess(wordlist, guesslist);
    Guess have_to_do_this_to_get_it_to_compile_best_guess = best_guess; // Clang/GCC's fault, not mine
    float expected_turns_to_win = evaluate_expectation([&](Word word) {
        if (word == have_to_do_this_to_get_it_to_compile_best_guess) {
            return 0.0;
        } else {
            Hint hint = get_hint(word, have_to_do_this_to_get_it_to_compile_best_guess);
            auto wordlist_remaining = get_compatible_words(have_to_do_this_to_get_it_to_compile_best_guess, hint, wordlist);
            if (wordlist_remaining.size() == 1) {
                return 1.0;
            } else if (wordlist_remaining.size() == 2) {
                return 1.5;
            } else {
                if (verbose) {
                    cout << "Calculating turns to win for " << get_word(word);
                    cout << " with maximum entropy gain guess " << get_guess(have_to_do_this_to_get_it_to_compile_best_guess);
                    cout << endl;
                }
                auto expected_MTW_next_turn = highest_expected_information_gain_strategy(wordlist_remaining, guesslist, false, true).get_expected_turns_to_win();
                if (verbose) cout << "\033[F\33[2K\r";
                return 1.0 + expected_MTW_next_turn;
            }
        }
    }, wordlist, verbose, parallel, parallel);
    return Strategy(best_guess, expected_turns_to_win);
}