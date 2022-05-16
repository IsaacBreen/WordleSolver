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

template<typename WL, typename GL>
Strategy _find_optimal_strategy_for_guess(Guess guess, WL& wordlist, GL& guesslist, Strategy& best_strategy, OptimizerConfig& config) {
    Strategy strategy(guess, 0);
    int i = 0;
    for (auto hyp_word : wordlist) {
        if (strategy.expected_turns_to_win > best_strategy.expected_turns_to_win) break;
        if (strategy.is_poisoned()) break;
        if (hyp_word == guess) continue;

        Hint hyp_hint = get_hint(hyp_word, guess);
        auto hyp_wordlist_remaining = get_compatible_words(guess, hyp_hint, wordlist);

        switch (hyp_wordlist_remaining.size()) {
            case 0:
                // something is wrong.
                throw "Something is wrong. No compatible words remaining.";
            case 1:
                // If there is only one word remaining, there are zero turns left
                strategy.expected_turns_to_win += 1.0 / wordlist.size();
                break;
            case 2:
                // If there are two words remaining, there is a probability of 1/2 that we guess correctly
                strategy.expected_turns_to_win += 1.5 / wordlist.size();
                break;
            default:
                // If there are more than two words remaining, we need to guess
                if (hyp_wordlist_remaining.size() < 0) {
                    throw "Something is wrong. Number of compatible words remaining is negative.";
                }
                // In the best case, where there are three words remaining and they are mutually excluding upon misguess, it will take 2/3 turns to win on average.
                // If strategy.expected_turns_to_win + 2/3/wordlist.size() > best_strategy.expected_turns_to_win, then strategy can't possibly beat best_strategy.
                strategy.expected_turns_to_win += (1 + 2.0/3.0)/wordlist.size();
                if (config.turn+1>=MAX_TURNS) {
                    strategy.poison();
                }
                break;
        }
    }
    for (auto hyp_word : wordlist) {
        if (strategy.expected_turns_to_win > best_strategy.expected_turns_to_win) break;
        if (strategy.is_poisoned()) break;
        if (hyp_word == guess) continue;

        Hint hyp_hint = get_hint(hyp_word, guess);
        auto hyp_wordlist_remaining = get_compatible_words(guess, hyp_hint, wordlist);

        if (config.verbose) {
            string word_str = to_string(i+1);
            i++;
            string num_words_str = to_string(wordlist.size());
            while (word_str.length() < num_words_str.length()) {
                word_str = " " + word_str;
            }
            cout << indentations(config.turn) << "Trying word " << get_word(hyp_word) << " (" << word_str << "/" << num_words_str << ") " << "expected turns to win so far: " << strategy.expected_turns_to_win << endl;
        }
        if (hyp_wordlist_remaining.size() > 2) {
            auto useful_guesslist = guesslist;
            useful_guesslist.set(guess, false);
            auto hyp_wordlist_remaining_variant = maybe_sparsify(hyp_wordlist_remaining);
            auto useful_guesslist_variant = maybe_sparsify(useful_guesslist);
            OptimizerConfig next_config = config;
            next_config.verbose = config.verbose and config.turn < PRINT_LEVELS and hyp_wordlist_remaining.size()>WORDLIST_SIZE_PRINT_THRESHOLD;
            next_config.turn++;
            next_config.max_Exp_turns_remaining_stop = (best_strategy.expected_turns_to_win - strategy.expected_turns_to_win) * wordlist.size() - 1;
            next_config.max_Exp_turns_remaining_stop = min(next_config.max_Exp_turns_remaining_stop, float(MAX_TURNS-config.turn-1));
            if (config.verbose) cout << indentations(config.turn) << "Recursing with " << hyp_wordlist_remaining.size() << " words remaining." << endl;
            Strategy hyp_strategy = visit(
                [&](auto& wordlist_variant, auto& guesslist_variant) {
                    return _find_optimal_strategy(wordlist_variant, guesslist_variant, next_config);
                },
                hyp_wordlist_remaining_variant, useful_guesslist_variant);
            if (hyp_strategy.is_poisoned()) {
                strategy.poison();
            } else {
                strategy.expected_turns_to_win += (hyp_strategy.expected_turns_to_win - 2.0/3.0) / wordlist.size();
            }
            if (config.verbose) cout << "\033[F\33[2K\r";
        }
        if (config.verbose) cout << "\033[F\33[2K\r";
    }
    return strategy;
}

template<typename WL, typename GL>
Strategy _find_optimal_strategy(WL& wordlist, GL& guesslist, OptimizerConfig& config) {
    if (config.verbose) {
        cout << indentations(config.turn) << "config.turn " << config.turn << ": ";
        cout << "wordlist (" << sparsity_string(wordlist) << ") size: " << wordlist.size() << "; ";
        cout << "guesslist (" << sparsity_string(guesslist) << ") size: " << guesslist.size();
        cout << endl;
    }
    // Initialise best_strategy with the maximum expected information gain guess
    Strategy best_strategy = highest_expected_information_gain_strategy(wordlist, guesslist, config.verbose);
    if (config.verbose) cout << indentations(config.turn) << "Highest information gain strategy: " << get_guess(best_strategy.get_guess()) << " (" << best_strategy.get_expected_turns_to_win() << ")" << endl;
    if (is_poisoned(best_strategy)) {
        best_strategy.expected_turns_to_win = config.max_Exp_turns_remaining_stop;
    } else {
        best_strategy.expected_turns_to_win = min(best_strategy.get_expected_turns_to_win(), config.max_Exp_turns_remaining_stop);
    }

    // Parallelise if not config.verbose. Need to use canonical to iterate over guesses.
    int i_guess = -1;
    for (auto guess : guesslist) {
        if (config.verbose) {
            i_guess++;
            // Whitespace-pad the guess number
            string guess_str = to_string(i_guess+1);
            string num_guesses_str = to_string(guesslist.size());
            auto num_digits = max(guess_str.size(), num_guesses_str.size());
            while (guess_str.length() < num_digits) {
                guess_str = " " + guess_str;
            }
            while (num_guesses_str.length() < num_digits) {
                num_guesses_str = " " + num_guesses_str;
            }
            cout << indentations(config.turn) << "Trying guess " << get_guess(guess) << " (" << guess_str << "/" << num_guesses_str << ")" << endl;
            cout << indentations(config.turn) << "Best strategy so far: " << get_guess(best_strategy.get_guess()) << " with " << best_strategy.expected_turns_to_win << " turns remaining." << endl;
        }
        Strategy strategy = _find_optimal_strategy_for_guess(guess, wordlist, guesslist, best_strategy, config);
        if (not strategy.is_poisoned() and strategy.expected_turns_to_win < best_strategy.expected_turns_to_win) {
            best_strategy = strategy;
        }
        if (config.verbose) cout << "\033[F\33[2K\r\033[F\33[2K\r";
    }
    if (config.verbose) cout << "\033[F\33[2K\r\033[F\33[2K\r";
    return best_strategy;
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