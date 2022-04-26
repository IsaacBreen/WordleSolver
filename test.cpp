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

using Hint = int;
using Word = int;
using GuessHint = int;

constexpr int WORD_LENGTH = 5;

template<class T, size_t N>
constexpr size_t size(T (&)[N]) { return N; }

constexpr char words[3][WORD_LENGTH+1] = {
    {"aaaaa"}, {"bbbbb"}, {"ccccc"}
};

constexpr char guesses[5][WORD_LENGTH+1] = {
    "aaaaa", "bbbbb", "ccccc", "aaabb", "ddddd"
};

constexpr auto NUM_WORDS = size(words);
constexpr auto NUM_GUESSES = size(guesses);

template<typename Base>
constexpr Base mypow(Base base, int exponent) {
    return (exponent == 0) ? 1 : base * mypow(base, exponent - 1);
}

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

constexpr const array<array<Hint, NUM_GUESSES>, NUM_WORDS> precalculate_hints(auto words, auto guesses) {
    // Returns an array of dimensions NUM_WORDS x NUM_GUESSES
    array<array<Hint, NUM_GUESSES>, NUM_WORDS> hints;
    for (int i = 0; i < NUM_WORDS; i++) {
        for (int j = 0; j < NUM_GUESSES; j++) {
            hints[i][j] = make_hint(words[i], guesses[j]);
        }
    }
    return hints;
}

// Not an error, despite what VSCode says
constexpr auto hints = precalculate_hints(words, guesses);

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

constexpr void test_hints() {
    static_assert(make_hint("aaaaa","bbbbb") == 0);
    static_assert(make_hint("aabbc", "dbbba") == string_to_hint("bbggy"));
}

int main()
{
    test_hints();
    cout << "Hints:" << endl;
    cout << "Word, Guess, Hint" << endl;
    for (int i = 0; i < NUM_WORDS; i++) {
        for (int j = 0; j < NUM_GUESSES; j++) {
            cout << words[i] << ", " << guesses[j] << ", " << hint_to_string(hints[i][j]) << endl;
        }
    }
    return 0;
}