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
using GuessHint = int;
using HintString = string;
using WordString = string;
using GuessHintString = string;

using PackedWordlist = bitset<NUM_WORDS>;

auto PackedWordlistAll = PackedWordlist(1);

#define CONST_TYPE constexpr
#define READ_CSV
// CONST_TYPE must be const if READ_CSV is defined
#ifdef READ_CSV
#define CONST_TYPE const
#endif

#define WORDLIST_CSV_PATH "data/wordlist.csv"
#define GUESSES_CSV_PATH "data/guesslist.csv"

constexpr int NUM_HINT_CONFIGS = mypow(3,WORD_LENGTH);

#endif // CONSTANTS_HPP
