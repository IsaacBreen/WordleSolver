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


template<unsigned long N>
class DenseWordlist {
private:
    bitset<N> data;
public:
    DenseWordlist() {
        data.set();
    }
    // For copying
    DenseWordlist(const DenseWordlist& other) {
        data = other.data;
    }
    DenseWordlist(const bitset<N>& other_data) {
        data = other_data;
    }
    // Implicit conversion to vector<Word>
    operator vector<Word>() const {
        vector<Word> result;
        for (int i = 0; i < N; i++) {
            if (data[i]) {
                result.push_back(i);
            }
        }
        return result;
    }
    // Bitwise operators
    DenseWordlist operator&(const DenseWordlist& other) {
        return DenseWordlist(data & other.data);
    }
    DenseWordlist operator|(const DenseWordlist& other) {
        return DenseWordlist(data | other.data);
    }
    // Equality
    bool operator==(DenseWordlist& other) {
        return data == other.data;
    }
    int size() {
        return data.count();
    }
    // Getting and setting with square brackets
    bool operator[](int index) const {
        return data[index];
    }
    void set(int index, bool value) {
        data[index] = value;
    }
    // Convert to bitset<N> implicitly
    operator bitset<N>() {
        return data;
    }
    // Iterate over indices of set bits. Works by going through each bit and checking if it is set.
    // If it is set, it should be iterated over.
    class iterator {
    private:
        bitset<N> data;
        int index;
    public:
        iterator(bitset<N>& data, int index) {
            this->data = data;
            this->index = index;
        }
        int operator*() {
            return index;
        }
        iterator& operator++() {
            index++;
            while (index < N && !data[index]) {
                index++;
            }
            return *this;
        }
        bool operator!=(iterator other) {
            return index != other.index;
        }
    };
    iterator begin() {
        int index = 0;
        while (index < N && !data[index]) {
            index++;
        }
        return iterator(data, index);
    }
    iterator end() {
        return iterator(data, N);
    }
    float density() {
        return (float)size() / N;
    }
};

class SparseWordlist {
private:
    set<Word> data;
public:
    SparseWordlist() {}
    // For copying
    SparseWordlist(const SparseWordlist& other) {
        data = other.data;
    }
    SparseWordlist(const vector<Word>& data) {
        for (Word word : data) {
            this->data.insert(word);
        }
    }
    template<unsigned long N>
    SparseWordlist(const DenseWordlist<N>& data) {
        for (int i = 0; i < N; i++) {
            if (data[i]) {
                this->data.insert(i);
            }
        }
    }
    template<unsigned long N>
    SparseWordlist(const bitset<N>& data) {
        for (int i = 0; i < N; i++) {
            if (data[i]) {
                this->data.insert(i);
            }
        }
    }
    // Implicit conversion to vector<Word>
    operator vector<Word>() {
        vector<Word> result;
        for (Word word : data) {
            result.push_back(word);
        }
        return result;
    }
    // Operators with bitset and DenseWordlist
    template<unsigned long N>
    SparseWordlist operator&(const bitset<N>& other) {
        vector<Word> result;
        for (Word word : data) {
            if (other[word]) {
                result.push_back(word);
            }
        }
        return SparseWordlist(result);
    }
    template<unsigned long N>
    SparseWordlist operator|(const bitset<N>& other) {
        SparseWordlist result;
        for (Word word : data) {
            result.set(word, true);
        }
        for (int i = 0; i < N; i++) {
            if (other[i]) {
                result.set(i, true);
            }
        }
        return SparseWordlist(result);
    }
    template<unsigned long N>
    SparseWordlist operator&(const DenseWordlist<N>& other) {
        return *this & bitset<N>(other);
    }
    template<unsigned long N>
    SparseWordlist operator|(const DenseWordlist<N>& other) {
        return *this | bitset<N>(other);
    }
    // Equality
    bool operator==(const SparseWordlist& other) {
        return data == other.data;
    }
    int size() {
        return data.size();
    }
    // Getting and setting with square brackets
    Word operator[](int index) const {
        // Check if the index is in the set
        if (data.find(index) != data.end()) {
            return index;
        }
    }
    void set(int index, bool value) {
        bool is_in_set = data.find(index) != data.end();
        if (value && !is_in_set) {
            data.insert(index);
        } else if (!value && is_in_set) {
            data.erase(index);
        }
    }
    // Iterator
    auto begin() {
        return data.begin();
    }
    auto end() {
        return data.end();
    }
    float density() {
        return 1.0;
    }
};

// using PackedWordlist = PackedList<NUM_WORDS>;
using PackedWordlist = bitset<NUM_WORDS>;





// Initialize bitset to all 1s
PackedWordlist ALL_WORDS = PackedWordlist().set();
DenseWordlist<NUM_GUESSES> ALL_GUESSES;

#define BIG_NUMBER 1000000

#define DEBUG true

// #define CONST_TYPE constexpr
// #define CONST_TYPE const
#define CONST_TYPE
// #define READ_CSV
// CONST_TYPE must be const if READ_CSV is defined
// #ifdef READ_CSV
// #define CONST_TYPE const
// #endif

constexpr int NUM_HINT_CONFIGS = mypow(3,WORD_LENGTH);


string get_word(Word word) {
    if (word < 0 or word >= NUM_WORDS) {
        return "ERROR";
    }
    return words[word];
}

string get_guess(Guess guess) {
    if (guess < 0 or guess >= NUM_GUESSES) {
        return "ERROR";
    }
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
