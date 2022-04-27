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
#include <utils.hpp>
#include <data/data.hpp>
#include <hint.hpp>

#pragma once


bool word_is_compatible_with_guess_hint(WordString word, WordString guess, Hint hint) {
    // Initialise to "bbbb..." of same length as word
    Hint word_hint = 0;
    for (int i = 0; i < WORD_LENGTH; i++) {
        // If the guessed character is correct, it is 'g' for 'green'
        if (guess[i] == word[i]) {
            // word_hint[i] = 'g';
            word_hint += mypow(3,i)*2;
        }
    }
    for (int i = 0; i < WORD_LENGTH; i++) {
        for (int j = 0; j < WORD_LENGTH; j++) {
            // If the guessed character is in the word at a position that is not already guesses, it is 'y' for 'yellow'
            if (i != j and guess[i] == word[j] and guess[i] != word[i] and guess[j] != word[j]) {
                // word_hint[i] = 'y';
                word_hint += mypow(3,i);
                break;
            }
        }
    }
    return word_hint == hint;
}

CONST_TYPE auto precompute_compatibility_matrix() {
    array<array<PackedWordlist, NUM_GUESSES>, NUM_HINT_CONFIGS> compatibility_matrix;
    for (int h = 0; h < NUM_HINT_CONFIGS; h++) {
        for (int g = 0; g < NUM_GUESSES; g++) {
            for (int w = 0; w < NUM_WORDS; w++) {
                compatibility_matrix[h][g][w] = word_is_compatible_with_guess_hint(words[w], guesses[g], h);
            }
        }
    }
    return compatibility_matrix;
}

CONST_TYPE auto compatibility_matrix = precompute_compatibility_matrix();

bool word_is_compatible_with_guess_hint(Word word, Word guess, Hint hint) {
    return compatibility_matrix[hint][guess][word];
}

PackedWordlist get_compatible_words(Word guess, Hint hint, PackedWordlist wordlist) {
    PackedWordlist compatible_words;
    return wordlist & compatibility_matrix[hint][guess];
}

size_t size(PackedWordlist wordlist) {
    return wordlist.count();
}

