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


using namespace std;

vector<string> wordlist_to_strings(PackedWordlist wordlist) {
    vector<string> strings;
    for (int i = 0; i < NUM_WORDS; i++) {
        if (wordlist[i]) {
            strings.push_back(words[i]);
        }
    }
    return strings;
}

vector<string> guesslist_to_strings(PackedWordlist guesslist) {
    vector<string> strings;
    for (int i = 0; i < NUM_GUESSES; i++) {
        if (guesslist[i]) {
            strings.push_back(guesses[i]);
        }
    }
    return strings;
}