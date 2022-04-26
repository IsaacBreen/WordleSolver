#ifndef DATA_DATA_HPP
#define DATA_DATA_HPP

#include <utils.hpp>
#include "generated_constants.hpp"

#ifdef READ_CSV
// Read the data from guesslist.csv and wordlist.csv into array<char*, NUM_WORDS> and array<char*, NUM_GUESSES>
// The csv files are newline-separated with no header.
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>

using namespace std;

// Function to read the csv file into an array of strings.
template<int N>
auto read_csv(auto filename) {
    ifstream file(filename);
    array<char*, N> words;
    for (int i = 0; i < N; i++) {
        string line;
        getline(file, line);
        istringstream iss(line);
        string word;
        iss >> word;
        words[i] = new char[word.length() + 1];
        strcpy(words[i], word.c_str());
    }
    return words;
}

const array words = read_csv<NUM_WORDS>(WORDLIST_CSV_PATH);
const array guesses = read_csv<NUM_GUESSES>(GUESSES_CSV_PATH);

#else
// CONST_TYPE is constexpr; import the following:
#include "wordlist.hpp"
#include "guesslist.hpp"

#endif

#endif // DATA_HPP