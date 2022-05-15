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
#include <stdexcept>

// hint.cpp
#include <common.hpp>
#include <data/data.hpp>
#include <hint.hpp>
#include <compatibility.hpp>
#include <user.hpp>
#include <optimiser.hpp>
#include <word_types.hpp>
#include <entropy.hpp>

using namespace std;


int main()
{
    auto guesslist = DenseWordlist(ALL_GUESSES);
    auto wordlist = DenseWordlist(ALL_WORDS);
    wordlist = get_compatible_words(get_guess_index("soare"), string_to_hint("bybyb"), wordlist);
    auto wordlist_str = wordlist_to_strings(wordlist);
    for (auto word : wordlist_str) {
        cout << word << endl;
    }
    cout << "Done printing wordlist of length " << wordlist_str.size() << endl;
    auto est = estimate_execution_time_find_optimal_strategy(DenseWordlist(ALL_WORDS), DenseWordlist(ALL_GUESSES));
    cout << "Estimated execution time: " << est << " seconds" << endl;
    auto [max_eig_guess, max_eig] = highest_expected_information_gain_guess(wordlist, wordlist);
    cout << "Highest expected information gain guess: " << get_guess(max_eig_guess) << " with expected information gain of " << max_eig << endl;

    // wordlist = DenseWordlist(ALL_WORDS);
    // auto [max_eig_guess2, max_eig2] = highest_expected_information_gain_guess(wordlist, guesslist);
    // cout << "Highest expected information gain guess: " << get_guess(max_eig_guess2) << " with expected information gain of " << max_eig2 << endl;

    wordlist = DenseWordlist(ALL_WORDS);
    Strategy optimal_strategy = find_optimal_strategy(wordlist, guesslist);
    cout << "Optimal strategy is " << get_guess(optimal_strategy.get_guess()) << ", which wins in an average of " << optimal_strategy.get_expected_turns_to_win() << " turns" << endl;
    cout << "Information gain of optimal strategy is " << expected_information_gain(optimal_strategy.get_guess(), wordlist) << endl;
    cout << "---- Done computing optimal strategy ----" << endl;

    // find_optimal_strategy();
    // cout << PackedWordlist().set() << endl;
    return 0;
}