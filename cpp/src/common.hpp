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

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

using Hint = int;
using Word = int;
using GuessHint = int;

#define CONST_TYPE constexpr
#define READ_CSV
// CONST_TYPE must be const if READ_CSV is defined
#ifdef READ_CSV
#define CONST_TYPE const
#endif

#define WORDLIST_CSV_PATH "data/wordlist.csv"
#define GUESSES_CSV_PATH "data/guesslist.csv"

#endif // CONSTANTS_HPP
