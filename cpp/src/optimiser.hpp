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
#include <user.hpp>

#include <iostream>
using namespace std;

#pragma once

class Strategy {
private:
    Guess guess;
    map<Hint, vector<Strategy>> substrategies;
public:
    float expected_turns_to_win = 0;
    Strategy(Guess guess=0) {
        this->guess = guess;
    }
    void add_substrategy(Hint hint, Strategy strategy) {
        substrategies[hint].push_back(strategy);
    }
    Strategy get_substrategy(Hint hint, int index=0) {
        return substrategies[hint][index];
    }
    void remove_substrategies(Hint hint) {
        substrategies.erase(hint);
    }
    Guess get_guess() {
        return guess;
    }
    int size() {
        int result = 1;
        for (auto& [hint, strategies] : substrategies) {
            for (auto& strategy : strategies) {
                result *= strategy.size();
            }
        }
        return result;
    }
};

Strategy find_optimal_strategy(PackedWordlist& wordlist, float max_Exp_turns_remaining_stop, int levels_to_print) {
    Strategy best_strategy;
    best_strategy.expected_turns_to_win = max_Exp_turns_remaining_stop;
    for (Guess guess = 0; guess < NUM_GUESSES; guess++) {
        Strategy strategy(guess);
        for (Word hyp_word = 0; hyp_word < NUM_WORDS; hyp_word++) {
            if (not wordlist[hyp_word]) {
                break;
            }
            if (best_strategy.expected_turns_to_win < strategy.expected_turns_to_win) {
                break;
            }
            Hint hyp_hint = get_hint(hyp_word, guess);
            PackedWordlist hyp_wordlist_remaining = get_compatible_words(guess, hyp_hint, wordlist);
            // if (levels_to_print>0) {
            //     cout << "Hypothesis: " << words[hyp_word] << ", guess: " << guesses[guess] << " " << hint_to_string(hyp_hint) << " " << hyp_wordlist_remaining << endl;
            // }
            switch (hyp_wordlist_remaining.count()) {
                case 1:
                    // If there is only one word remaining, there are zero turns left
                    break;
                case 2:
                    // If there are two words remaining, there is a probability of 1/2 that we guess correctly
                    strategy.expected_turns_to_win += 0.5 / wordlist.count();
                    break;
                default:
                    // If there are more than two words remaining, we need to guess
                    Strategy hyp_strategy = find_optimal_strategy(hyp_wordlist_remaining, best_strategy.expected_turns_to_win-1, levels_to_print-1);
                    strategy.expected_turns_to_win += (1 + hyp_strategy.expected_turns_to_win) / wordlist.count();
                    break;
            }
        }
        if (strategy.expected_turns_to_win < best_strategy.expected_turns_to_win) {
            best_strategy = strategy;
            if (levels_to_print > 0) {
                cout << "New best strategy: " << guesses[guess] << " " << strategy.expected_turns_to_win << endl;
            }
        } else if (levels_to_print > 0) {
            cout << "No improvement: " << guesses[guess] << " " << strategy.expected_turns_to_win << endl;
        }
    }
    if (levels_to_print > 0) {
        cout << "Best strategy: " << guesses[best_strategy.get_guess()] << " " << best_strategy.expected_turns_to_win << endl;
    }
    return best_strategy;
}

Strategy find_optimal_strategy() {
    return find_optimal_strategy(ALL_WORDS, BIG_NUMBER, 1);
}