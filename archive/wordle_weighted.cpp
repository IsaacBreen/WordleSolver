#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <random>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include </opt/homebrew/Cellar/libomp/13.0.1/include/omp.h>

using namespace std;

class Wordle {
public:
    Wordle(const string& wordlist_filename) {
        ifstream wordlist_file(wordlist_filename);
        string word;
        while (wordlist_file >> word) {
            words.push_back(word);
        }
        wordlist_file.close();

        // Get word frequencies from ./words-by-frequency/english.txt. This file contains frequencies for all English words.
        // Frequencies are in the first column, followed by the word itself in the second. Filter out words that don't have 5 characters.
        ifstream word_freq_file("words-by-frequency/english.txt");
        string word_freq_line;
        while (getline(word_freq_file, word_freq_line)) {
            // Split the line into parts by tab.
            vector<string> word_freq_parts;
            stringstream word_freq_line_stream(word_freq_line);
            string word_freq_part;
            while (getline(word_freq_line_stream, word_freq_part, '\t')) {
                word_freq_parts.push_back(word_freq_part);
            }
            // If the word has 5 characters, add it to the map.
            if (word_freq_parts[1].size() == 5) {
                word_freqs[word_freq_parts[1]] = stod(word_freq_parts[0]);
            }
        }
        cout << "Total number of words in word list: " << words.size() << endl;
        cout << "Total number of words in word frequency file: " << word_freqs.size() << endl;
        // Normalize frequencies
        // Remove words that don't occur in the wordlist
        for (auto it = word_freqs.begin(); it != word_freqs.end();) {
            if (find(words.begin(), words.end(), it->first) == words.end()) {
                it = word_freqs.erase(it);
            } else {
                ++it;
            }
        }
        // Remove words that occur with 0 frequency
        for (auto it = word_freqs.begin(); it != word_freqs.end();) {
            if (it->second == 0) {
                it = word_freqs.erase(it);
            } else {
                ++it;
            }
        }
        // Remove words that occur in the wordlist but not in the word frequency file
        for (auto it = words.begin(); it != words.end();) {
            if (word_freqs.find(*it) == word_freqs.end()) {
                it = words.erase(it);
            } else {
                ++it;
            }
        }
        double total_freq = 0;
        for (auto& word_freq : word_freqs) {
            total_freq += word_freq.second;
        }
        for (auto& word_freq : word_freqs) {
            word_freq.second /= total_freq;
        }
        // Print words with 0 frequency
        for (auto& word_freq : word_freqs) {
            if (word_freq.second == 0) {
                // cout << word_freq.first << endl;
            }
        }
        // Order the wordlist alphabetically
        sort(words.begin(), words.end());
        cout << "Total number of words in word list after filtering: " << words.size() << endl;
        base_entropy = conditional_entropy(words);
    }

    double conditional_entropy(const vector<string>& words, const map<char, map<int, bool>>& condition = map<char, map<int, bool>>(), bool verbose = false) {
        vector<string> compatible_words;
        for (const string& word : words) {
            bool is_compatible = true;
            for (const auto& letter_positions : condition) {
                for (const auto& position_is_present : letter_positions.second) {
                    if (position_is_present.second && (word.find(letter_positions.first) == string::npos || word[position_is_present.first] != letter_positions.first)) {
                        is_compatible = false;
                        break;
                    }
                    if (!position_is_present.second && word[position_is_present.first] == letter_positions.first) {
                        is_compatible = false;
                        break;
                    }
                }
                if (!is_compatible) {
                    break;
                }
            }
            if (is_compatible) {
                compatible_words.push_back(word);
            }
        }
        if (verbose) {
            if (compatible_words.size() < 20) {
                cout << "Compatible words: ";
                for (const string& word : compatible_words) {
                    cout << word << " ";
                }
                cout << endl;
            } else {
                cout << "Number of compatible words: " << compatible_words.size() << " (too many to print)" << endl;
            }
        }
        // Entropy is the sum of the probabilities of each word multiplied by the log of the probability.
        double entropy = 0;
        for (const string& word : compatible_words) {
            double word_freq = word_freqs[word];
            entropy += word_freq * log(word_freq);
        }
        return -entropy;
    }

    double information_gain(const vector<string>& words, const map<char, map<int, bool>>& condition, const map<char, map<int, bool>>& condition_initial = map<char, map<int, bool>>(), bool verbose = false) {
        return conditional_entropy(words, condition_initial, verbose) - conditional_entropy(words, condition, verbose);
    }

    map<char, map<int, bool>> evaluate_guess_to_condition(const string& ground_truth, const string& guess) {
        // assert(ground_truth.size() == 5 && guess.size() == 5);
        map<char, map<int, bool>> condition;
        for (int i = 0; i < 5; i++) {
            condition[guess[i]][i] = guess[i] == ground_truth[i];
        }
        return condition;
    }

    map<char, map<int, bool>> simplify_condition(const map<char, map<int, bool>>& condition) {
        set<int> determined_positions;
        for (const auto& letter_positions : condition) {
            for (const auto& position_is_present : letter_positions.second) {
                if (position_is_present.second) {
                    determined_positions.insert(position_is_present.first);
                }
            }
        }
        map<char, map<int, bool>> simplified_condition;
        for (const auto& letter_positions : condition) {
            for (const auto& position_is_present : letter_positions.second) {
                if (position_is_present.second || determined_positions.find(position_is_present.first) == determined_positions.end()) {
                    simplified_condition[letter_positions.first][position_is_present.first] = position_is_present.second;
                }
            }
        }
        // remove all letters that have no positions left
        for (auto it = simplified_condition.begin(); it != simplified_condition.end();) {
            if (it->second.empty()) {
                it = simplified_condition.erase(it);
            } else {
                it++;
            }
        }
        return simplified_condition;
    }

    map<char, map<int, bool>> combine_conditions(const map<char, map<int, bool>>& condition1, const map<char, map<int, bool>>& condition2) {
        map<char, map<int, bool>> condition = condition1;
        for (const auto& letter_positions : condition2) {
            for (const auto& position_is_present : letter_positions.second) {
                if (condition[letter_positions.first].find(position_is_present.first) == condition[letter_positions.first].end()) {
                    condition[letter_positions.first][position_is_present.first] = position_is_present.second;
                } else {
                    assert(condition[letter_positions.first][position_is_present.first] == position_is_present.second);
                }
            }
        }
        return simplify_condition(condition);
    }

    map<char, map<int, bool>> evaluate_guesses_to_condition(const string& ground_truth, const vector<string>& guesses) {
        map<char, map<int, bool>> condition;
        for (const string& guess : guesses) {
            condition = combine_conditions(condition, evaluate_guess_to_condition(ground_truth, guess));
        }
        return condition;
    }

    double expected_information_gain_stochastic(const string& guess, int n_gt_samples = 1000, int n_wordlist_samples = 0) {
        double average_information_gain = 0;
        for (int i = 0; i < n_gt_samples; i++) {
            vector<string> wordlist_sample;
            if (n_wordlist_samples) {
                wordlist_sample = vector<string>(words.begin(), words.begin() + min(n_wordlist_samples, (int)words.size()));
            } else {
                wordlist_sample = words;
            }
            string ground_truth = wordlist_sample[rand() % wordlist_sample.size()];
            map<char, map<int, bool>> condition = evaluate_guess_to_condition(ground_truth, guess);
            double this_information_gain = (base_entropy - conditional_entropy(wordlist_sample, condition) - log2(words.size() / wordlist_sample.size())) / n_gt_samples;
            average_information_gain += this_information_gain;
        }
        return average_information_gain;
    }

    double expected_information_gain(const string& guess) {
        double average_information_gain = 0;
        vector<string> ground_truths;
        #pragma omp parallel for
        for (int i = 0; i < words.size(); i++) {
            string ground_truth = words[i];
            map<char, map<int, bool>> condition = evaluate_guess_to_condition(ground_truth, guess);
            double this_information_gain = (base_entropy - conditional_entropy(words, condition)) * word_freqs[ground_truth];
            average_information_gain += this_information_gain;
        }
        return average_information_gain;
    }

    vector<string> words;
    map<string, double> word_freqs;
private:
    double base_entropy;
};

int main() {
    Wordle wordle("words.txt");
    // Test every guess in the wordlist and find the one with the highest information gain
    string best_guess = "";
    double best_information_gain = 0;
    for (const string& guess : wordle.words) {
        double this_information_gain = wordle.expected_information_gain(guess);
        cout << guess << ": " << this_information_gain << endl;
        if (this_information_gain > best_information_gain) {
            cout << "New best guess: " << guess << endl;
            best_guess = guess;
            best_information_gain = this_information_gain;
        }
    }
    cout << "Best guess: " << best_guess << endl;
    cout << "Information gain: " << best_information_gain << endl;

    return 0;
}

// /opt/homebrew/Cellar/libomp/13.0.1/
// Compile with g++ -std=c++20 -O3 -fopenmp -o wordle wordle.cpp