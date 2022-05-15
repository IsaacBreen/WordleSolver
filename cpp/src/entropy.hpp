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

#include <common.hpp>
#include <data/data.hpp>
#include <hint.hpp>
#include <compatibility.hpp>

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
template <typename F, typename WL>
double evaluate_expectation(F f, WL& wordlist)
{
    double expectation = 0;
    for (auto word : wordlist)
    {
        expectation += f(word);
    }
    return expectation / wordlist.size();
}

template<typename WL>
float information_gain(Guess guess, Word word, WL& wordlist) {
    float entropy_before = log(wordlist.size());
    float entropy_after = 0;
    for (auto hyp_word : wordlist) {
        Hint hint = get_hint(hyp_word, guess);
        entropy_after += log(num_compatible_words(guess, hint, wordlist)) / wordlist.size();
    }
    float ig = entropy_before - entropy_after;
    return ig;
}

template<typename WL, typename GL>
tuple<Guess, float> highest_information_gain_guess(Word word, WL& wordlist, GL& guesslist) {
    float max_ig = 0;
    Guess max_ig_guess = 0;
    for (auto guess : guesslist) {
        float ig = information_gain(guess, word, wordlist);
        if (ig > max_ig) {
            max_ig = ig;
            max_ig_guess = guess;
        }
    }
    return make_tuple(max_ig_guess, max_ig);
}

template<typename WL, typename GL>
tuple<Guess, float> highest_expected_information_gain_guess(WL& wordlist, GL& guesslist) {
    float max_ig = 0;
    Guess max_ig_guess = 0;
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

template<typename WL>
float expected_information_gain(Guess guess, WL& wordlist) {
    return evaluate_expectation([&](Word word) {
        return information_gain(guess, word, wordlist);
    }, wordlist);
}