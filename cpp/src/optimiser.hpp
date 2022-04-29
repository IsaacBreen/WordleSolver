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

#include <iostream>
using namespace std;

#pragma once

#define PRINT_LEVELS 3

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

void poison(Strategy& strategy) {
    strategy.expected_turns_to_win = -1;
}

bool is_poisoned(Strategy& strategy) {
    return strategy.expected_turns_to_win == -1;
}

Strategy find_optimal_strategy(PackedWordlist& wordlist, float max_Exp_turns_remaining_stop, int levels_to_print) {
    Strategy best_strategy();
    poison(best_strategy);
    best_strategy.expected_turns_to_win = max_Exp_turns_remaining_stop;
    for (Guess guess = 0; guess < NUM_GUESSES; guess++) {
        if (levels_to_print > 0) {
            // Whitespace-pad the guess number
            string guess_str = to_string(guess);
            string num_guesses_str = to_string(NUM_GUESSES);
            while (guess_str.length() < num_guesses_str.length()) {
                guess_str = " " + guess_str;
            }
            cout << "Trying guess " << guesses[guess] << " (" << guess_str << "/" << num_guesses_str << ")" << endl;
            cout << "Best strategy so far: " << best_strategy.get_guess() << " with " << best_strategy.expected_turns_to_win << " turns remaining" << endl;
        }
        Strategy strategy(guess);
        int i = 0;
        for (Word hyp_word = 0; hyp_word < NUM_WORDS; hyp_word++) {
            if (strategy.expected_turns_to_win >= best_strategy.expected_turns_to_win) break;
            if (is_poisoned(strategy)) break;
            if (not wordlist[hyp_word]) continue;

            Hint hyp_hint = get_hint(hyp_word, guess);
            PackedWordlist hyp_wordlist_remaining = get_compatible_words(guess, hyp_hint, wordlist);

            if (levels_to_print > 0) {
                // Whitespace-pad the word number
                string word_str = to_string(i);
                i++;
                string num_words_str = to_string(wordlist.count());
                while (word_str.length() < num_words_str.length()) {
                    word_str = " " + word_str;
                }
                cout << "Trying word " << words[hyp_word] << " (" << word_str << "/" << num_words_str << ") " << "expected turns to win so far: " << strategy.expected_turns_to_win << endl;
            }
            switch (hyp_wordlist_remaining.count()) {
                case 0:
                    // something is wrong.
                    cout << "Something is wrong. No compatible words remaining." << endl;
                    exit(1);
                case 1:
                    // If there is only one word remaining, there are zero turns left
                    break;
                case 2:
                    // If there are two words remaining, there is a probability of 1/2 that we guess correctly
                    strategy.expected_turns_to_win += 0.5 / wordlist.count();
                    break;
                default:
                    // If there are more than two words remaining, we need to guess
                    if (hyp_wordlist_remaining.count() < 0) cout << "Something is wrong. Number of compatible words remaining is negative." << endl;
                    Strategy hyp_strategy = find_optimal_strategy(hyp_wordlist_remaining, best_strategy.expected_turns_to_win-1, levels_to_print-1);
                    // cout << " " << hyp_strategy.expected_turns_to_win;
                    if (is_poisoned(hyp_strategy)) {
                        poison(strategy);
                    } else {
                        strategy.expected_turns_to_win += (1 + hyp_strategy.expected_turns_to_win) / wordlist.count();
                    }
                    break;
            }
            if (levels_to_print > 0) {
                cout << "\033[F";
            }
        }
        if (not is_poisoned(strategy) and strategy.expected_turns_to_win < best_strategy.expected_turns_to_win) {
            best_strategy = strategy;
            if (levels_to_print > 0) {
                // cout << "\033[F";
                // cout << "New best strategy: " << guesses[guess] << " " << strategy.expected_turns_to_win << endl;
            }
        } else if (levels_to_print > 0) {
            // cout << "\033[F";
            // cout << "No improvement: " << guesses[guess] << " " << strategy.expected_turns_to_win << endl;
        }
        // if (levels_to_print > 0) {
        //     cout << "\033[F";
        //     cout << "Best strategy so far: " << guesses[best_strategy.get_guess()] << " " << best_strategy.expected_turns_to_win << endl;
        // }
        // Undo previous line printed using escape code
        if (levels_to_print > 0) {
            cout << "\033[F\033[F";
        }
        if (best_strategy.expected_turns_to_win <= 0) {
            break;
        }
    }
    if (levels_to_print > 0) {
        // cout << "Best strategy: " << guesses[best_strategy.get_guess()] << " " << best_strategy.expected_turns_to_win << endl;
        // cout << "\033[F";
    }
    return best_strategy;
}

Strategy find_optimal_strategy() {
    return find_optimal_strategy(ALL_WORDS, 2.5, PRINT_LEVELS);
}