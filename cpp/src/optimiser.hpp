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

#include <iostream>
using namespace std;

#pragma once

#define PRINT_LEVELS 5
#define SPARSITY_THRESHOLD 0.1
#define SPACED_ENDL "    " << endl
#define MAX_TURNS 6
#define PRECOMPUTE_LEVEL 5
// #define MIN_DIGIT_PADDING 2

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

template<typename T>
auto maybe_sparsify(T wordlist) {
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

template<typename T>
string sparsity_string(T wordlist) {
    if constexpr (is_same<T, SparseWordlist>::value) {
        return "sparse";
    } else {
        return "dense";
    }
}

template<typename WL, typename GL>
Strategy find_optimal_strategy_for_guess(Guess guess, int i_guess, WL& wordlist, GL& guesslist, Strategy best_strategy, int turn) {
    if (turn < PRINT_LEVELS) {
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
        cout << "Trying guess " << get_guess(guess) << " (" << guess_str << "/" << num_guesses_str << ")" << SPACED_ENDL;
        cout << "Best strategy so far: " << get_guess(best_strategy.get_guess()) << " with " << best_strategy.expected_turns_to_win << " turns remaining." << SPACED_ENDL;
    }
    Strategy strategy(guess);
    int i = 0;
    for (auto hyp_word : wordlist) {
        if (strategy.expected_turns_to_win > best_strategy.expected_turns_to_win) break;
        if (is_poisoned(strategy)) break;
        if (hyp_word == guess) continue;

        Hint hyp_hint = get_hint(hyp_word, guess);
        auto hyp_wordlist_remaining = get_compatible_words(guess, hyp_hint, wordlist);

        switch (hyp_wordlist_remaining.size()) {
            case 0:
                // something is wrong.
                cout << "Something is wrong. No compatible words remaining." << SPACED_ENDL;
                exit(1);
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
                    cout << "Something is wrong. Number of compatible words remaining is negative." << SPACED_ENDL;
                    exit(1);
                }
                // In the best case, where there are three words remaining and they are mutually excluding upon misguess, it will take 2/3 turns to win on average.
                // If strategy.expected_turns_to_win + 2/3/wordlist.size() > best_strategy.expected_turns_to_win, then strategy can't possibly beat best_strategy.
                strategy.expected_turns_to_win += (1 + 2.0/3.0)/wordlist.size();
                if (turn+1>=MAX_TURNS) {
                    poison(strategy);
                    break;
                }
                break;
        }
    }
    for (auto hyp_word : wordlist) {
        if (strategy.expected_turns_to_win > best_strategy.expected_turns_to_win) break;
        if (is_poisoned(strategy)) break;
        if (hyp_word == guess) continue;

        Hint hyp_hint = get_hint(hyp_word, guess);
        auto hyp_wordlist_remaining = get_compatible_words(guess, hyp_hint, wordlist);

        if (turn < PRINT_LEVELS) {
            string word_str = to_string(i+1);
            i++;
            string num_words_str = to_string(wordlist.size());
            while (word_str.length() < num_words_str.length()) {
                word_str = " " + word_str;
            }
            cout << "Trying word " << get_word(hyp_word) << " (" << word_str << "/" << num_words_str << ") " << "expected turns to win so far: " << strategy.expected_turns_to_win << SPACED_ENDL;
        }
        if (hyp_wordlist_remaining.size() > 2) {
            Strategy hyp_strategy;
            auto useful_guesslist = guesslist;
            useful_guesslist.set(guess, false);
            if (turn == PRECOMPUTE_LEVEL) {
                // Filter out useless guesses (ones that don't eliminate any words)
                for (auto hyp_guess2 : guesslist) {
                    if (hyp_guess2 == guess) continue;
                    bool is_useful = false;
                    for (auto hyp_word2 : hyp_wordlist_remaining) {
                        auto hyp_hint2 = get_hint(hyp_word2, hyp_guess2);
                        for (auto hyp_word3 : hyp_wordlist_remaining) {
                            if (not word_is_compatible_with_guess_hint(hyp_word3, hyp_guess2, hyp_hint2)) {
                                is_useful = true;
                            }
                            if (is_useful) break;
                        }
                        if (is_useful) break;
                    }
                    if (not is_useful) {
                        useful_guesslist.set(hyp_guess2, false);
                    }
                }
            }
            auto hyp_wordlist_remaining_variant = maybe_sparsify(hyp_wordlist_remaining);
            auto useful_guesslist_variant = maybe_sparsify(useful_guesslist);
            float max_Exp_turns_remaining_stop_hyp = (best_strategy.expected_turns_to_win - strategy.expected_turns_to_win) * wordlist.size() - 1;
            max_Exp_turns_remaining_stop_hyp = min(max_Exp_turns_remaining_stop_hyp, float(MAX_TURNS-turn-1));
            hyp_strategy = visit(
                [&](auto& wordlist_variant, auto& guesslist_variant) {
                    return find_optimal_strategy(wordlist_variant, guesslist_variant, max_Exp_turns_remaining_stop_hyp, turn+1);
                },
                hyp_wordlist_remaining_variant, useful_guesslist_variant);
            // hyp_strategy = find_optimal_strategy(hyp_wordlist_remaining, useful_guesslist, max_Exp_turns_remaining_stop_hyp, turn+1);
            if (is_poisoned(hyp_strategy)) {
                poison(strategy);
            } else {
                strategy.expected_turns_to_win += (hyp_strategy.expected_turns_to_win - 2.0/3.0) / wordlist.size();
            }
        }
        if (turn < PRINT_LEVELS) cout << "\033[F";
    }
    if (turn < PRINT_LEVELS) cout << "\033[F\033[F";
    return strategy;
}

template<typename WL, typename GL>
Strategy find_optimal_strategy(WL& wordlist, GL& guesslist, float max_Exp_turns_remaining_stop=2.6, int turn=0) {
    if (turn < PRINT_LEVELS) {
        cout << endl;
        cout << "Turn " << turn << ": ";
        cout << "wordlist (" << sparsity_string(wordlist) << ") size: " << wordlist.size() << "; ";
        cout << "guesslist (" << sparsity_string(guesslist) << ") size: " << guesslist.size();
        cout << SPACED_ENDL;
    }
    // Initialise best_strategy with the guess that greedily narrows the wordlist down the most
    Guess greedy_guess;
    float greedy_guess_wordlist_size = BIG_NUMBER;
    // if (turn < PRINT_LEVELS) cout << "Finding best starting guess" << SPACED_ENDL;
    int i_guess = -1;
    for (auto guess : guesslist) {
        i_guess++;
        if (turn < PRINT_LEVELS) {
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
            cout << "Finding best starting guess. Trying " << get_guess(guess) << ". Best so far: " << get_guess(greedy_guess) << " (" << guess_str << "/" << num_guesses_str << ")" << SPACED_ENDL;
        }
        float expected_wordlist_size = 0;
        for (auto word : wordlist) {
            Hint hint = get_hint(word, guess);
            expected_wordlist_size += num_compatible_words(guess, hint, wordlist);
        }
        if (expected_wordlist_size < greedy_guess_wordlist_size) {
            greedy_guess = guess;
            greedy_guess_wordlist_size = expected_wordlist_size;
        }
        if (turn < PRINT_LEVELS) cout << "\033[F";
    }
    if (turn < PRINT_LEVELS) cout << endl;
    if (turn < PRINT_LEVELS) cout << "Best starting guess: " << get_guess(greedy_guess) << " (" << greedy_guess_wordlist_size/wordlist.size() << ")" << SPACED_ENDL;
    // if (turn < PRINT_LEVELS) cout << "\033[F\033[F\033[F";
    Strategy dummy_strategy;
    dummy_strategy.expected_turns_to_win = max_Exp_turns_remaining_stop;
    poison(dummy_strategy);
    if (turn < PRINT_LEVELS) cout << "-------- Priming step " << turn << " --------" << SPACED_ENDL;
    Strategy best_strategy = find_optimal_strategy_for_guess(greedy_guess, 0, wordlist, guesslist, dummy_strategy, turn);
    if (turn < PRINT_LEVELS) cout << "\033[F";
    if (is_poisoned(best_strategy)) {
        best_strategy.expected_turns_to_win = max_Exp_turns_remaining_stop;
    } else {
        best_strategy.expected_turns_to_win = min(best_strategy.expected_turns_to_win, max_Exp_turns_remaining_stop);
    }
    i_guess = -1;
    for (auto guess : guesslist) {
        if (turn < PRINT_LEVELS) cout << "Best strategy so far: " << get_guess(best_strategy.get_guess()) << " " << best_strategy.expected_turns_to_win << SPACED_ENDL;
        i_guess++;
        Strategy strategy = find_optimal_strategy_for_guess(guess, i_guess, wordlist, guesslist, best_strategy, turn);
        if (not is_poisoned(strategy) and strategy.expected_turns_to_win < best_strategy.expected_turns_to_win) {
            best_strategy = strategy;
        }
        if (turn < PRINT_LEVELS) cout << "\033[F";
    }
    if (turn == 0) {
        for (int i = 0; i < PRINT_LEVELS; i++) {
            for (int j = 0; j < 6; j++) {
                cout << endl;
            }
        }
    }
    if (turn < PRINT_LEVELS) cout << "\033[F\033[F\033[F\033[F";
    return best_strategy;
}

