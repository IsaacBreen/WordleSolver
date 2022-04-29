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

using namespace std;

#include <utils.hpp>
#include <data/data.hpp>

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

using Hint = int;
using Word = int;
using Guess = int;
using GuessHint = int;
using HintString = string;
using WordString = string;
using GuessHintString = string;

using PackedWordlist = bitset<NUM_WORDS>;

// Initialize bitset to all 1s
PackedWordlist ALL_WORDS = PackedWordlist().set();

#define BIG_NUMBER 1000000

#define DEBUG true

// #define CONST_TYPE constexpr
// #define CONST_TYPE const
#define CONST_TYPE
#define READ_CSV
// CONST_TYPE must be const if READ_CSV is defined
// #ifdef READ_CSV
// #define CONST_TYPE const
// #endif

#define WORDLIST_CSV_PATH "data/wordlist.csv"
#define GUESSES_CSV_PATH "data/guesslist.csv"

constexpr int NUM_HINT_CONFIGS = mypow(3,WORD_LENGTH);


string get_word(Word word) {
    return words[word];
}

string get_guess(Guess guess) {
    return guesses[guess];
}

Word get_word_index(string word) {
    for (int i = 0; i < NUM_WORDS; i++) {
        if (word == words[i]) {
            return i;
        }
    }
    exit(1);
}

Guess get_guess_index(string guess) {
    for (int i = 0; i < NUM_GUESSES; i++) {
        if (guess == guesses[i]) {
            return i;
        }
    }
    exit(1);
}


#endif // CONSTANTS_HPP
