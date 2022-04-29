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

#pragma once

// Take a ground truth word and a guess and return the guess result
constexpr auto make_hint(auto word, auto guess) {
    // Initialise to "bbbb..." of same length as word
    Hint hint = 0;
    for (int i = 0; i < WORD_LENGTH; i++) {
        // If the guessed character is correct, it is 'g' for 'green'
        if (guess[i] == word[i]) {
            // hint[i] = 'g';
            hint += mypow(3,i)*2;
        }
    }
    for (int i = 0; i < WORD_LENGTH; i++) {
        for (int j = 0; j < WORD_LENGTH; j++) {
            // If the guessed character is in the word at a position that is not already guesses, it is 'y' for 'yellow'
            if (i != j and guess[i] == word[j] and guess[i] != word[i] and guess[j] != word[j]) {
                // hint[i] = 'y';
                hint += mypow(3,i);
                break;
            }
        }
    }
    return hint;
}

constexpr const Hint string_to_hint(const char* hint) {
    Hint result = 0;
    for (int i = 0; i < WORD_LENGTH; i++) {
        if (hint[i] == 'g') {
            result += mypow(3,i)*2;
        } else if (hint[i] == 'y') {
            result += mypow(3,i);
        }
    }
    return result;
}

constexpr const char* hint_to_string(Hint hint) {
    // A letter is either 'g' for 'green' or 'y' for 'yellow' or 'b' for 'black'
    // TODO constexpr: string result;
    char* result = new char[WORD_LENGTH+1];
    for (int i = 0; i < WORD_LENGTH; i++) {
        if (hint % 3 == 0) {
            result[i] = 'b';
        } else if (hint % 3 == 1) {
            result[i] = 'y';
        } else {
            result[i] = 'g';
        }
        hint /= 3;
    }
    result[WORD_LENGTH] = '\0';
    return result;
}

constexpr const char* hint_to_pretty_string(Hint hint) {
    // Like hint_to_string but uses colour escape sequences
    auto result = hint_to_string(hint);
    char* pretty_result = new char[WORD_LENGTH+1];
    for (int i = 0; i < WORD_LENGTH; i++) {
        if (result[i] == 'b') {
            pretty_result[i] = '\033[1;30m';
        } else if (result[i] == 'y') {
            pretty_result[i] = '\033[1;33m';
        } else {
            pretty_result[i] = '\033[1;32m';
        }
        pretty_result[i] += result[i];
        pretty_result[i] += '\033[0m';
    }
    pretty_result[WORD_LENGTH] = '\0';
    return pretty_result;
}

CONST_TYPE array<array<Hint, NUM_GUESSES>, NUM_WORDS> precalculate_hints(auto words, auto guesses) {
    // Returns an array of dimensions NUM_WORDS x NUM_GUESSES
    array<array<Hint, NUM_GUESSES>, NUM_WORDS> hints;
    #pragma omp parallel for
    for (int i = 0; i < NUM_WORDS; i++) {
        if (DEBUG and i%100==0) cout << "Precomputing hints: " << (float)i/NUM_WORDS*100 << "%" << flush << "\r";
        for (int j = 0; j < NUM_GUESSES; j++) {
            hints[i][j] = make_hint(words[i], guesses[j]);
        }
    }
    cout << "Precomputing hints: 100%" << endl;
    return hints;
}

// Not an error, despite what VSCode says
CONST_TYPE auto hints = precalculate_hints(words, guesses);

Hint get_hint(Word word, Word guess) {
    return hints[word][guess];
}