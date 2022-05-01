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

#define PRINT_LEVELS 2

class Strategy {
private:
    map<Hint, vector<Strategy>> substrategies;
public:
    Guess guess;
    float expected_turns_to_win;
    Strategy(Guess guess=-1) {
        this->guess = guess;
        this->expected_turns_to_win = 0;
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
    strategy.guess = -1;
}

bool is_poisoned(Strategy& strategy) {
    return strategy.guess == -1;
}

Strategy find_optimal_strategy(PackedWordlist& wordlist=ALL_WORDS, float max_Exp_turns_remaining_stop=3.1, int levels_to_print=PRINT_LEVELS) {
    Strategy best_strategy;
    poison(best_strategy);
    best_strategy.expected_turns_to_win = max_Exp_turns_remaining_stop;
    // cout << endl << endl << endl << endl;
    for (Guess guess = 0; guess < NUM_GUESSES; guess++) {
        if (levels_to_print > 0) {
            // Whitespace-pad the guess number
            string guess_str = to_string(guess+1);
            string num_guesses_str = to_string(NUM_GUESSES);
            while (guess_str.length() < num_guesses_str.length()) {
                guess_str = " " + guess_str;
            }
            cout << "Trying guess " << get_guess(guess) << " (" << guess_str << "/" << num_guesses_str << ")" << endl;
            cout << "Best strategy so far: " << get_guess(best_strategy.get_guess()) << " with " << best_strategy.expected_turns_to_win << " turns remaining. max_Exp_turns_remaining_stop=" << max_Exp_turns_remaining_stop << endl;
        }
        Strategy strategy(guess);
        int i = 0;
        for (Word hyp_word = 0; hyp_word < NUM_WORDS; hyp_word++) {
            if (strategy.expected_turns_to_win >= best_strategy.expected_turns_to_win) break;
            if (is_poisoned(strategy)) break;
            if (not wordlist[hyp_word]) continue;
            if (hyp_word == guess) continue;

            Hint hyp_hint = get_hint(hyp_word, guess);
            PackedWordlist hyp_wordlist_remaining = get_compatible_words(guess, hyp_hint, wordlist);

            if (levels_to_print > 0) {
                string word_str = to_string(i+1);
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
                    strategy.expected_turns_to_win += 1.0 / wordlist.count();
                    break;
                case 2:
                    // If there are two words remaining, there is a probability of 1/2 that we guess correctly
                    strategy.expected_turns_to_win += 1.5 / wordlist.count();
                    break;
                default:
                    // If there are more than two words remaining, we need to guess
                    if (hyp_wordlist_remaining.count() < 0) {
                        cout << "Something is wrong. Number of compatible words remaining is negative." << endl;
                        exit(1);
                    }
                    // In the best case, where there are three words remaining and they are mutually excluding upon misguess, it will take 2/3 turns to win on average.
                    // If strategy.expected_turns_to_win + 2/3/wordlist.count() > best_strategy.expected_turns_to_win, then strategy can't possibly beat best_strategy.
                    if (strategy.expected_turns_to_win + 2.0/3.0/wordlist.count() > best_strategy.expected_turns_to_win) {
                        poison(strategy);
                        break;
                    } else {
                        float max_Exp_turns_remaining_stop_hyp = (best_strategy.expected_turns_to_win - strategy.expected_turns_to_win) * wordlist.count() - 1;
                        Strategy hyp_strategy = find_optimal_strategy(hyp_wordlist_remaining, max_Exp_turns_remaining_stop_hyp, levels_to_print-1);
                        if (is_poisoned(hyp_strategy)) {
                            poison(strategy);
                        } else {
                            strategy.expected_turns_to_win += (1 + hyp_strategy.expected_turns_to_win) / wordlist.count();
                        }
                    }
                    break;
            }
            if (levels_to_print > 0) cout << "\033[F";
        }
        if (levels_to_print > 0) cout << endl;
        if (not is_poisoned(strategy) and strategy.expected_turns_to_win < best_strategy.expected_turns_to_win) {
            best_strategy = strategy;
            // if (levels_to_print > 0) cout << "\033[FNew best strategy: " << get_guess(guess) << " " << strategy.expected_turns_to_win << endl;
        }
        if (levels_to_print > 0) cout << "Best strategy so far: " << guesses[best_strategy.get_guess()] << " " << best_strategy.expected_turns_to_win << endl;
        if (levels_to_print > 0) cout << "\033[F\033[F\033[F\033[F";
    }
    if (levels_to_print == PRINT_LEVELS) {
        for (int i = 0; i < PRINT_LEVELS; i++) {
            cout << endl << endl << endl;
        }
        // cout << "Best strategy: " << guesses[best_strategy.get_guess()] << " (" << best_strategy.get_guess() << ") " << best_strategy.expected_turns_to_win << endl;
    }
    return best_strategy;
}

