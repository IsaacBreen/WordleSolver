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

// hint.cpp
#include <common.hpp>
#include <data/data.hpp>
#include <hint.hpp>
#include <compatibility.hpp>
#include <user.hpp>
#include <optimiser.hpp>
#include <word_types.hpp>

using namespace std;


int main()
{
    // Demonstration of \033[F
    // cout << "This is line 1" << endl;
    // cout << "This is line 2" << endl;
    // cout << "\033[FThis is line 3" << endl;
    // cout << "\033[FThis is loin" << endl;
    // cout << "\033[F\033[FThis is line 4" << endl;
    // cout << endl;

    // print_compatibility_matrix(0,100);
    // cout << "Number of words compatible with guess 'fluid' and hint 'bybgb' is " << num_compatible_words(100, string_to_hint("bybgb"), ALL_WORDS) << endl;
    // cout << "Number of words compatible with guess 'soare' and hint 'bbybb' is " << num_compatible_words(get_guess_index("soare"), string_to_hint("bbybb"), ALL_WORDS) << endl;
    // cout << "Is the word 'ninja' compatible with guess 'soare' and hint 'bbybb'? " << word_is_compatible_with_guess_hint(get_word_index("ninja"), get_guess_index("soare"), string_to_hint("bbybb")) << endl;
    // cout << words[get_word_index("ninja")] << " " << guesses[get_guess_index("soare")] << endl;
    // cout << hint_to_string(string_to_hint("bbybb")) << endl;

    PackedWordlist wordlist = ALL_WORDS;
    wordlist = get_compatible_words(get_guess_index("soare"), string_to_hint("ybbby"), wordlist);
    wordlist = get_compatible_words(get_guess_index("clint"), string_to_hint("bbbby"), wordlist);
    auto wordlist_str = wordlist_to_strings(wordlist);
    for (auto word : wordlist_str) {
        cout << word << endl;
    } 
    cout << "Done printing wordlist of length " << wordlist_str.size() << endl;
    find_optimal_strategy(wordlist);
    cout << "---- Done printing optimal strategy ----" << endl;

    // find_optimal_strategy();
    // cout << PackedWordlist().set() << endl;
    return 0;
}