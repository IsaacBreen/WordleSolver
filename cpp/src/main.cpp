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
#include<optimiser.hpp>

using namespace std;


int main()
{
    // print_compatibility_matrix(0,100);
    // cout << "Number of words compatible with guess 'fluid' and hint 'bybgb' is " << num_compatible_words(100, string_to_hint("bybgb"), ALL_WORDS) << endl;
    // cout << "Number of words compatible with guess 'soare' and hint 'bbybb' is " << num_compatible_words(get_guess_index("soare"), string_to_hint("bbybb"), ALL_WORDS) << endl;
    // cout << "Is the word 'ninja' compatible with guess 'soare' and hint 'bbybb'? " << word_is_compatible_with_guess_hint(get_word_index("ninja"), get_guess_index("soare"), string_to_hint("bbybb")) << endl;
    // cout << words[get_word_index("ninja")] << " " << guesses[get_guess_index("soare")] << endl;
    // cout << hint_to_string(string_to_hint("bbybb")) << endl;
    // PackedWordlist wordlist = ALL_WORDS;
    // wordlist = get_compatible_words(get_guess_index("soare"), string_to_hint("ybbby"), wordlist);
    // wordlist = get_compatible_words(get_guess_index("clint"), string_to_hint("bbbby"), wordlist);
    find_optimal_strategy(wordlist, 3, 2);
    // find_optimal_strategy();
    // cout << PackedWordlist().set() << endl;
    return 0;
}