#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <random>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include </opt/homebrew/Cellar/libomp/13.0.1/include/omp.h>
#include <nlohmann/json.hpp>

using namespace std;
using namespace nlohmann;

class Wordle
{
public:
    // vector<string> words;
    // vector<string> valid_guesses;

    Wordle(const string &solutions_filename, const string &valid_guesses_filename)
    {
        // Read in the solutions file, a json list of words
        ifstream solutions_file(solutions_filename);
        json solutions_json;
        solutions_file >> solutions_json;
        for (auto &solution : solutions_json)
        {
            words.push_back(solution.get<string>());
        }
        // Read in the valid guesses file
        ifstream valid_guesses_file(valid_guesses_filename);
        json valid_guesses_json;
        valid_guesses_file >> valid_guesses_json;
        for (auto &guess : valid_guesses_json)
        {
            valid_guesses.push_back(guess.get<string>());
        }
        cout << "There are " << words.size() << " solutions and " << valid_guesses.size() << " valid guesses." << endl;
        base_entropy = conditional_entropy(words);
    }

    bool is_word_compatible(const string &word, const tuple<map<char, map<int, bool>>, set<char>> &condition)
    {
        auto [in_word, not_in_word] = condition;
        bool is_compatible = true;
        for (auto &[letter, position_and_presence] : in_word)
        {
            // Letter should be in word
            if (word.find(letter) == string::npos)
            {
                return false;
            }
            for (auto &[position, is_present] : position_and_presence)
            {
                // Tautological: If letter is present, it should be present; if letter is not present, it should not be present.
                if (is_present != (word[position] == letter))
                {
                    return false;
                }
            }
        }
        for (const auto &letter : not_in_word)
        {
            // Letter should not be in word
            if (word.find(letter) != string::npos)
            {
                return false;
            }
        }
        return true;
    }

    vector<string> get_compatible_words(const tuple<map<char, map<int, bool>>, set<char>> &condition)
    {
        vector<string> compatible_words;
        for (const string &word : words)
        {
            if (is_word_compatible(word, condition))
            {
                compatible_words.push_back(word);
            }
        }
        return compatible_words;
    }

    double conditional_entropy(const vector<string> &words, const tuple<map<char, map<int, bool>>, set<char>> &condition = make_tuple(map<char, map<int, bool>>(), set<char>()), bool verbose = false)
    {
        auto compatible_words = get_compatible_words(condition);
        // Entropy is the sum of the probabilities of each word multiplied by the log of the probability. Assume each word remaining is equally likely.
        double entropy = 0;
        for (const string &word : compatible_words)
        {
            double probability = 1.0 / compatible_words.size();
            entropy += probability * log2(probability);
        }
        return -entropy;
    }

    auto evaluate_guess_to_condition(const string &ground_truth, const string &guess)
    {
        map<char, map<int, bool>> in_word;
        set<char> not_in_word;
        for (int i = 0; i < 5; i++)
        {
            if (ground_truth.find(guess[i]) != string::npos)
            {
                in_word[guess[i]][i] = guess[i] == ground_truth[i];
            }
            else
            {
                not_in_word.insert(guess[i]);
            }
        }
        return make_tuple(in_word, not_in_word);
    }

    auto combine_conditions(const tuple<map<char, map<int, bool>>, set<char>> &condition1, const tuple<map<char, map<int, bool>>, set<char>> &condition2)
    {
        auto [in_word1, not_in_word1] = condition1;
        auto [in_word2, not_in_word2] = condition2;
        map<char, map<int, bool>> in_word;
        set<char> not_in_word;
        for (const auto &[letter, position_and_presence] : in_word1)
        {
            for (const auto &[position, is_present] : position_and_presence)
            {
                in_word[letter][position] = is_present;
            }
        }
        for (const auto &letter : not_in_word1)
        {
            not_in_word.insert(letter);
        }
        for (const auto &[letter, position_and_presence] : in_word2)
        {
            for (const auto &[position, is_present] : position_and_presence)
            {
                in_word[letter][position] = is_present;
            }
        }
        for (const auto &letter : not_in_word2)
        {
            not_in_word.insert(letter);
        }
        // Remove from not_in_word any letters that are in in_word
        for (const auto &[letter, position_and_presence] : in_word)
        {
            not_in_word.erase(letter);
        }
        return make_tuple(in_word, not_in_word);
    }

    double expected_information_gain(const string &guess, const tuple<map<char, map<int, bool>>, set<char>> &condition = make_tuple(map<char, map<int, bool>>(), set<char>()))
    {
        vector<double> information_gains(words.size());
#pragma omp parallel for num_threads(4)
        for (int i = 0; i < words.size(); i++)
        {
            string ground_truth = words[i];
            auto new_condition = evaluate_guess_to_condition(ground_truth, guess);
            auto combined_condition = combine_conditions(condition, new_condition);
            information_gains[i] = base_entropy - conditional_entropy(words, combined_condition);
        }
        return accumulate(information_gains.begin(), information_gains.end(), 0.0) / information_gains.size();
    }

    vector<string> words;
    vector<string> valid_guesses;
    double base_entropy;

    auto convert_pretty_condition_to_condition(string guess, string pretty_condition)
    {
        // pretty_condition is a string of the form "nnypn" where:
        // n means the corresponding letter in guess is not in the word
        // y means the corresponding letter in guess is in the word at the corresponding position
        // p means the corresponding letter in guess is in the word but not at the corresponding position

        map<char, map<int, bool>> in_word;
        set<char> not_in_word;
        for (int i = 0; i < 5; i++)
        {
            if (pretty_condition[i] == 'n')
            {
                not_in_word.insert(guess[i]);
            }
            else if (pretty_condition[i] == 'y')
            {
                in_word[guess[i]][i] = true;
            }
            else if (pretty_condition[i] == 'p')
            {
                in_word[guess[i]][i] = false;
            }
        }
        return make_tuple(in_word, not_in_word);
    }

    void apply_condition(const tuple<map<char, map<int, bool>>, set<char>> &condition)
    {
        // Remove words that don't match the condition
        auto compatible_words = get_compatible_words(condition);
        words = compatible_words;
        base_entropy = conditional_entropy(words);
    }
};

int main()
{
    Wordle wordle("wordlesolver/solutions_nyt.json", "wordlesolver/nonsolutions_nyt.json");

    auto condition = wordle.convert_pretty_condition_to_condition("soare", "nnnpy");
    // auto new_condition = wordle.convert_pretty_condition_to_condition("pudic", "yynnn");
    // condition = wordle.combine_conditions(condition, new_condition);
    // new_condition = wordle.convert_pretty_condition_to_condition("vuggy", "nynyn");
    // condition = wordle.combine_conditions(condition, new_condition);
    // wordle.apply_condition(condition);

    auto [in_word, not_in_word] = condition;
    cout << "Condition: " << endl;
    cout << "In word: " << endl;
    for (const auto &[letter, position_and_presence] : in_word)
    {
        cout << letter << ": ";
        for (const auto &[position, is_present] : position_and_presence)
        {
            cout << "{" << position << ": " << is_present << "}";
        }
        cout << endl;
    }
    cout << "Not in word: " << endl;
    for (const auto &letter : not_in_word)
    {
        cout << letter << " ";
    }
    cout << endl;

    // Check whether "purge" is compatible with the condition
    cout << "Is purge compatible with the condition? " << wordle.is_word_compatible("purge", condition) << endl;
    auto compatible_words = wordle.get_compatible_words(condition);
    cout << "Number of compatible words: " << compatible_words.size() << endl;
    if (compatible_words.size() > 0 and compatible_words.size()<20)
    {
        cout << "Compatible words: " << endl;
        for (const auto &word : compatible_words)
        {
            cout << word << endl;
        }
    }

    wordle.apply_condition(condition);

    // Print base entropy
    // cout << "Base entropy: " << wordle.base_entropy << endl;
    // // Try some words
    // cout << "Information gain for \"roate\": " << wordle.expected_information_gain("roate") << endl;
    // cout << "Information gain for \"tares\": " << wordle.expected_information_gain("roate") << endl;
    // cout << "Information gain for \"oiler\": " << wordle.expected_information_gain("oiler") << endl;
    // cout << "Information gain for \"adeiu\": " << wordle.expected_information_gain("adeiu") << endl;
    // cout << "Information gain for \"zzzzz\": " << wordle.expected_information_gain("zzzzz") << endl;
    cout << "Information gain for \"purge\": " << wordle.expected_information_gain("purge") << endl;
    // // Get the condition for "roate" when the ground truth is "tares"
    // auto [in_word, not_in_word] = wordle.evaluate_guess_to_condition("tares", "roate");
    // cout << "Condition for \"roate\" when the ground truth is \"tares\": " << endl;
    // cout << "In word: " << endl;
    // for (const auto& [letter, position_and_presence] : in_word) {
    //     cout << letter << ": ";
    //     for (const auto& [position, is_present] : position_and_presence) {
    //         cout << is_present << " ";
    //     }
    //     cout << endl;
    // }
    // cout << "Not in word: " << endl;
    // for (const auto& letter : not_in_word) {
    //     cout << letter << " ";
    // }
    // cout << endl;

    // Test every guess in the wordlist and find the one with the highest information gain
    string best_guess = "";
    double best_information_gain = 0;
    map<string, double> information_gains;
    for (const string &guess : wordle.valid_guesses)
    {
        double this_information_gain = wordle.expected_information_gain(guess, condition);
        if (this_information_gain == this_information_gain and this_information_gain != 0)
        {
            // Print word number and pad with spaces
            // cout << guess << setw(5) << setfill(' ') << "(" << information_gains.size() << "/" << wordle.valid_guesses.size() << ")"
            //      << ": " << this_information_gain << " bits" << endl;
            if (this_information_gain > best_information_gain)
            {
                cout << "New best guess: " << guess << endl;
                best_guess = guess;
                best_information_gain = this_information_gain;
            }
            information_gains[guess] = this_information_gain;
        }
    }
    if (information_gains.size() == 0)
    {
        cout << "No useful guesses found. Possible words: " << endl;
        for (const string &guess : wordle.get_compatible_words(condition))
        {
            cout << guess << endl;
        }
    }
    else
    {
        cout << "Top 10 guesses for expected information gain:" << endl;
        vector<pair<string, double>> sorted_information_gains(information_gains.begin(), information_gains.end());
        sort(sorted_information_gains.begin(), sorted_information_gains.end(), [](const pair<string, double> &a, const pair<string, double> &b)
             { return a.second > b.second; });
        for (int i = 0; i < min(10, (int)sorted_information_gains.size()); i++)
        {
            cout << i + 1 << ". " << sorted_information_gains[i].first << ": " << sorted_information_gains[i].second << endl;
        }
    }

    // Filter out that are not compatible with the condition and print the top 10
    cout << "Top 10 guesses in the word list:" << endl;
    vector<string> compatible = wordle.get_compatible_words(condition);
    int i = 0;
    while (i < compatible.size() and i < 10)
    {
        if (wordle.is_word_compatible(compatible[i], condition))
        {
            cout << i + 1 << ". " << compatible[i] << endl;
            i++;
        }
    }

    return 0;
}

// /opt/homebrew/Cellar/libomp/13.0.1/
// Compile with g++ -std=c++20 -O3 -fopenmp -o wordle wordle.cpp