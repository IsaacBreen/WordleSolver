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
        // Add all words to valid_guesses
        for (auto &word : words)
        {
            valid_guesses.push_back(word);
        }
        cout << "There are " << words.size() << " solutions and " << valid_guesses.size() << " valid guesses." << endl;
        base_entropy = conditional_entropy(words);
    }

    bool word_is_compatible(const string &word, const tuple<map<char, map<int, bool>>, set<char>> &condition)
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
            if (word_is_compatible(word, condition))
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
        // b means the corresponding letter in guess is not in the word
        // y means the corresponding letter in guess is in the word but not at the corresponding position
        // g means the corresponding letter in guess is in the word at the corresponding position

        map<char, map<int, bool>> in_word;
        set<char> not_in_word;
        for (int i = 0; i < 5; i++)
        {
            if (pretty_condition[i] == 'b')
            {
                not_in_word.insert(guess[i]);
            }
            else if (pretty_condition[i] == 'g')
            {
                in_word[guess[i]][i] = true;
            }
            else if (pretty_condition[i] == 'y')
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

    tuple<int, string, string, double, double> find_optimal_guesses(const tuple<map<char, map<int, bool>>, set<char>> &condition)
    {
        // Find the optimal guess for information gain and for compatibility
        string best_guess_for_information_gain = "";
        string best_guess_for_compatibility = "";
        double best_information_gain = 0;
        double best_information_gain_for_compatibility = 0;
        int num_compatible_in_wordlist = 0;
        map<string, double> information_gains;
        for (const string &guess : valid_guesses)
        {
            double this_information_gain = expected_information_gain(guess, condition);
            if (this_information_gain == this_information_gain and this_information_gain != 0)
            {
                if (this_information_gain > best_information_gain)
                {
                    best_guess_for_information_gain = guess;
                    best_information_gain = this_information_gain;
                }
                if (this_information_gain > best_information_gain_for_compatibility and word_is_compatible(guess, condition))
                {
                    best_guess_for_compatibility = guess;
                    best_information_gain_for_compatibility = this_information_gain;
                }
                information_gains[guess] = this_information_gain;
            }
            if (word_is_compatible(guess, condition) and find(words.begin(), words.end(), guess) != words.end())
            {
                num_compatible_in_wordlist++;
            }
        }
        return make_tuple(num_compatible_in_wordlist, best_guess_for_information_gain, best_guess_for_compatibility, best_information_gain, best_information_gain_for_compatibility);
    }
};

string prettify_condition(const tuple<map<char, map<int, bool>>, set<char>> &condition)
{
    // The string to return. We will add to this using << >>
    ostringstream s;
    auto &[in_word, not_in_word] = condition;
    for (const auto &[letter, position_and_presence] : in_word)
    {
        s << letter << " is";
        for (const auto &[position, is_present] : position_and_presence)
        {
            if (is_present)
            {
                s << " in position " << position;
            }
            else
            {
                s << " not in position " << position;
            }
            s << ",";
        }
        s << "\n";
    }
    s << "Not in word: ";
    for (auto &letter : not_in_word)
    {
        s << letter << " ";
    }
    return s.str();
}

int main()
{
    Wordle wordle("wordlesolver/solutions_nyt.json", "wordlesolver/nonsolutions_nyt.json");

    // auto condition = wordle.convert_pretty_condition_to_condition("soare", "bbbyg");
    // condition = wordle.combine_conditions(condition, wordle.convert_pretty_condition_to_condition("pudic", "ggbbb"));
    // condition = wordle.combine_conditions(condition, wordle.convert_pretty_condition_to_condition("puree", "gggyg"));
    // wordle.apply_condition(condition);
    // cout << prettify_condition(condition) << endl;
    // cout << "Words left: " << wordle.get_compatible_words(condition).size() << endl;
    // for (const auto &word : wordle.get_compatible_words(condition))
    // {
    //     cout << word << endl;
    // }

    // Interactive solver
    cout << "Optimal first guess is cached" << endl;
    cout << "Enter results as a 5-letter string where each letter is:" << endl;
    cout << "  'b' for black/gray (not present)" << endl;
    cout << "  'y' for yellow (present but not at the specified position)" << endl;
    cout << "  'g' for green (present and at the specified position)" << endl;
    cout << "Choose '1. Best guess for information gain' if you want the most information, or '2. Best guess for compatibility' if you're feeling lucky and want a quick win." << endl;
    cout << "For example, a result of yellow, yellow, green, green, gray corresponds to the result string 'yyggb'" << endl;
    cout << "Enter soare" << endl;

    // Start with the empty condition
    auto condition = make_tuple(map<char, map<int, bool>>(), set<char>());
    string word = "soare";
    while (true)
    {
        string result;
        while (true)
        {
            cout << "Result: ";
            cin >> result;
            // Validate the result
            bool result_ok = true;
            if (result.size() != 5)
            {
                cout << "Result must be 5 letters long; got " << result.size() << endl;
                result_ok = false;
            }
            for (int i = 0; i < 5; i++)
            {
                if (result[i] != 'b' and result[i] != 'y' and result[i] != 'g')
                {
                    cout << "Result must be a combination of the characters 'b', 'y', and 'g'; got " << result << endl;
                    result_ok = false;
                    break;
                }
            }
            if (result_ok)
            {
                break;
            }
        }
        auto new_condition = wordle.convert_pretty_condition_to_condition(word, result);
        condition = wordle.combine_conditions(condition, new_condition);
        cout << word << " is " << result << endl;
        cout << "New condition:\n"
             << prettify_condition(new_condition) << endl;
        cout << "Combined condition:\n"
             << prettify_condition(condition) << endl;
        wordle.apply_condition(condition);
        int num_compatible_in_wordlist = wordle.get_compatible_words(condition).size();
        const int MAX_COMPATIBLE_WORDS = 10;
        if (num_compatible_in_wordlist == 0)
        {
            cout << "No words are compatible with this condition" << endl;
            break;
        }
        if (num_compatible_in_wordlist == 1)
        {
            cout << "The word is " << wordle.words[0] << endl;
            break;
        }
        cout << "There are " << num_compatible_in_wordlist << " compatible words remaining";
        if (num_compatible_in_wordlist > MAX_COMPATIBLE_WORDS)
        {
            cout << endl;
        }
        else
        {
            cout << ": ";
            for (const string &word : wordle.words)
            {
                cout << word << " ";
            }
            cout << endl;
        }
        auto [num_compatible_words, best_guess_for_information_gain, best_guess_for_compatibility, infogain1, infogain2] = wordle.find_optimal_guesses(condition);
        if (infogain1 <= infogain2)
        {
            cout << "The best guess is " << best_guess_for_compatibility << " with an information gain of " << infogain1 << endl;
            word = best_guess_for_compatibility;
        }
        else
        {
            cout << "1. Best guess for information gain: " << best_guess_for_information_gain << " (" << infogain1 << " bits)" << endl;
            cout << "2. Best guess for compatibility: " << best_guess_for_compatibility << " (" << infogain2 << " bits)" << endl;
            cout << "Which guess do you want to use? (1 or 2)" << endl;
            while (true)
            {
                int choice;
                cin >> choice;
                if (choice == 1)
                {
                    word = best_guess_for_information_gain;
                    break;
                }
                else if (choice == 2)
                {
                    word = best_guess_for_compatibility;
                    break;
                }
                else
                {
                    cout << "Invalid choice" << endl;
                }
            }
        }
    }

    return 0;
}

// /opt/homebrew/Cellar/libomp/13.0.1/
// Compile with g++ -std=c++20 -O3 -fopenmp -o wordle wordle.cpp