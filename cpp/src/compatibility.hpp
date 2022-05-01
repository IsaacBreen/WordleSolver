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
#include <filesystem>
#include <fstream>
// Boost boost::archive::binary_oarchive for array and bitset
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
// Boost boost::serialization for vector
#include <boost/serialization/vector.hpp>
// Boost boost::serialization for bitset
#include <boost/serialization/bitset.hpp>
// Boost boost::serialization for array
#include <boost/serialization/array.hpp>
// For compression
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
// To link with the right libraries, run with -lboost_serialization -lboost_iostreams
#include <omp.h>
#if defined(_OPENMP)
    #include <omp.h>
#endif

#include <common.hpp>
#include <utils.hpp>
#include <data/data.hpp>
#include <hint.hpp>

#pragma once

using CompatibilityMatrix = array<array<PackedWordlist, NUM_GUESSES>, NUM_HINT_CONFIGS>;


bool word_is_compatible_with_guess_hint(WordString word, WordString guess, Hint hint) {
    // Initialise to "bbbb..." of same length as word
    return make_hint(word, guess) == hint;
}

CONST_TYPE auto precompute_compatibility_matrix() {
    CompatibilityMatrix compatibility_matrix;
    for (int h = 0; h < NUM_HINT_CONFIGS; h++) {
        if (DEBUG and h%3==0) cout << "Precomputing compatibility matrix: " << (float)h/NUM_HINT_CONFIGS*100 << "%" << endl;
        #pragma omp parallel for
        for (int g = 0; g < NUM_GUESSES; g++) {
            for (int w = 0; w < NUM_WORDS; w++) {
                compatibility_matrix[h][g][w] = word_is_compatible_with_guess_hint(words[w], guesses[g], h);
            }
        }
    }
    return compatibility_matrix;
}

void save_compatibility_matrix(string path, CompatibilityMatrix& compatibility_matrix) {
    // Saves the compatibility matrix to a compressed binary file
    boost::iostreams::filtering_ostream out;
    out.push(boost::iostreams::gzip_compressor());
    out.push(boost::iostreams::file_sink(path));
    boost::archive::binary_oarchive oa(out);
    oa << compatibility_matrix;
}

auto load_compatibility_matrix(string path) {
    // Loads the compatibility matrix from a compressed binary file
    boost::iostreams::filtering_istream in;
    in.push(boost::iostreams::gzip_decompressor());
    in.push(boost::iostreams::file_source(path));
    boost::archive::binary_iarchive ia(in);
    CompatibilityMatrix compatibility_matrix;
    ia >> compatibility_matrix;
    return compatibility_matrix;
}

CONST_TYPE auto precompute_compatibility_matrix_cached() {
    // Load from file if it exists
    string path = "data/cache/compatibility_matrix.bin.gz";
    if (filesystem::exists(path)) {
        cout << "Loading compatibility matrix from " << path << endl;
        auto compatibility_matrix = load_compatibility_matrix(path);
        cout << "Loaded compatibility matrix" << endl;
        return compatibility_matrix;
    } else {
        cout << "Precomputing compatibility matrix and saving to " << path << endl;
        auto compatibility_matrix = precompute_compatibility_matrix();
        save_compatibility_matrix(path, compatibility_matrix);
        return compatibility_matrix;
    }
}

CONST_TYPE auto compatibility_matrix = precompute_compatibility_matrix_cached();

bool word_is_compatible_with_guess_hint(Word& word, Word& guess, Hint& hint) {
    return compatibility_matrix[hint][guess][word];
}

PackedWordlist get_compatible_words(Word guess, Hint hint, PackedWordlist& wordlist) {
    PackedWordlist compatible_words;
    return wordlist & compatibility_matrix[hint][guess];
}


size_t size(PackedWordlist wordlist) {
    return wordlist.count();
}

int num_compatible_words(Word guess, Hint hint, PackedWordlist& wordlist) {
    return size(get_compatible_words(guess, hint, wordlist));
}
