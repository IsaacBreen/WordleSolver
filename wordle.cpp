
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <random>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include "tqdm/tqdm.h"
#include <omp.h>
// For timing
#include <ctime>

using namespace std;
using namespace nlohmann;

#define MAX_THREADS 2
#define LARGE_NUMBER 1000000

// Read in the solutions file, a json list of words that can be word of the day
vector<string> load_wordlist(string wordlist_filename) {
    ifstream wordlist_file(wordlist_filename);
    json wordlist_json;
    wordlist_file >> wordlist_json;
    vector<string> wordlist;
    for (auto word : wordlist_json) {
        wordlist.push_back(word);
    }
    return wordlist;
}

// Add all words in the wordlist to valid_guesses
auto load_wordle(string wordlist_filename, string valid_guesses_filename) {
    vector<string> wordlist = load_wordlist(wordlist_filename);
    vector<string> valid_guesses = load_wordlist(valid_guesses_filename);
    for (string word : wordlist) {
        valid_guesses.push_back(word);
    }
    return make_tuple(wordlist, valid_guesses);
}

template <typename T>
void warn_empty_vector(vector<T> v, string name="") {
    if (v.empty()) {
        cerr << "Warning: empty vector " << name << endl;
    }
}

#define WARN_EMPTY_VECTOR(v) warn_empty_vector(v, #v)

// Take a grouth truth word and a guess and return the guess result
string make_guess_hint(string word, string guess) {
    // Initialise to "bbbb..." of same length as word
    string guess_hint = "";
    for (int i = 0; i < word.length(); i++) {
        // If neither of the below, the guessed character is 'b' for 'black' by defualt
        guess_hint += "b";
    }
    for (int i = 0; i < int(guess.length()); i++) {
        // If the guessed character is correct, it is 'g' for 'green'
        if (guess[i] == word[i]) {
            guess_hint[i] = 'g';
        }
    }
    for (int i = 0; i < int(guess.length()); i++) {
        for (int j = 0; j < int(word.length()); j++) {
            // If the guessed character is in the word at a position that is not already guesses, it is 'y' for 'yellow'
            if (i != j and guess[i] == word[j] and guess[i] != word[i] and guess[j] != word[j]) {
                guess_hint[i] = 'y';
                break;
            }
        }
    }
    return guess_hint;
}

string hint_to_string(string hint) {
    stringstream ss;
    for (int i = 0; i < hint.length(); i++) {
        if (hint[i] == 'g') {
            // Green
            ss << "\033[32m" << hint[i] << "\033[0m";
        } else if (hint[i] == 'y') {
            // Yellow
            cout << "\033[33m" << hint[i] << "\033[0m";
        } else {
            // Gray
            ss << "\033[37m" << hint[i] << "\033[0m";
        }
    }
    return ss.str();
}

bool guess_is_useless(string guess, set<char> eliminated_letters) {
    // A guess is useless if all its letters have been eliminated already
    for (char letter : guess) {
        if (eliminated_letters.find(letter) == eliminated_letters.end()) {
            return false;
        }
    }
    return true;
}

// Check if a word is compatible with a guess and guess result
bool word_is_compatible_with_guess_hint(string word_hyp, string guess, string guess_hint) {
    for (int i = 0; i < int(guess.length()); i++) {
        bool letter_is_in_word_at_another_nongreen_position = false;
        for (int j = 0; j < int(guess.length()); j++) {
            if (i != j and word_hyp[j] == guess[i] and guess_hint[j] != 'g') {
                letter_is_in_word_at_another_nongreen_position = true;
                break;
            }
        }
        if (guess_hint[i] == 'g') {
            if (word_hyp[i] != guess[i]) {
                return false;
            }
        } else if (guess_hint[i] == 'y') {
            if (word_hyp[i] == guess[i] or !letter_is_in_word_at_another_nongreen_position) {
                return false;
            }
        } else if (guess_hint[i] == 'b') {
            if (word_hyp[i] == guess[i] or letter_is_in_word_at_another_nongreen_position) {
                return false;
            }
        } else {
            cout << "Invalid guess result: " << guess_hint << endl;
            return false;
        }
    }
    return true;
}

// Get all words compatible with a guess and guess result
vector<string> get_compatible_words(string guess, string guess_hint, vector<string>& wordlist) {
    vector<string> compatible_words;
    for (auto word_hyp : wordlist) {
        if (word_is_compatible_with_guess_hint(word_hyp, guess, guess_hint)) {
            compatible_words.push_back(word_hyp);
        }
    }
    return compatible_words;
}

vector<int> get_compatible_word_indices(string guess, string guess_hint, vector<string>& wordlist) {
    vector<int> compatible_word_indices;
    for (int i = 0; i < int(wordlist.size()); i++) {
        if (word_is_compatible_with_guess_hint(wordlist[i], guess, guess_hint)) {
            compatible_word_indices.push_back(i);
        }
    }
    return compatible_word_indices;
}

int get_num_compatible_words(string guess, string guess_hint, vector<string>& wordlist) {
    int num_compatible_words = 0;
    for (auto word_hyp : wordlist) {
        if (word_is_compatible_with_guess_hint(word_hyp, guess, guess_hint)) {
            num_compatible_words++;
        }
    }
    return num_compatible_words;
}

vector<string> get_useful_guesses_using_guess_history(vector<string>& guesses, set<char>& eliminated_letters) {
    vector<string> useful_guesses;
    for (string guess : guesses) {
        if (!guess_is_useless(guess, eliminated_letters)) {
            useful_guesses.push_back(guess);
        }
    }
    return useful_guesses;
}

bool guess_is_useful(string guess, string hint, vector<string>& wordlist) {
    // A guess is useful if it can eliminate any word from the wordlist
    for (auto word_hyp : wordlist) {
        if (not word_is_compatible_with_guess_hint(word_hyp, guess, hint)) {
            return true;
        }
    }
    return false;
}

auto guess_may_be_useful(string guess, vector<string>& wordlist) {
    for (string word_hyp : wordlist) {
        if (guess_is_useful(guess, make_guess_hint(word_hyp, guess), wordlist)) {
            return true;
        }
    }
    return false;
}

vector<string> get_useful_guesses(vector<string>& wordlist, vector<string>& valid_guesses) {
    vector<string> useful_guesses;
    for (string guess : valid_guesses) {
        if (guess_may_be_useful(guess, wordlist)) {
            useful_guesses.push_back(guess);
        }
    }
    return useful_guesses;
}

double get_expected_information_gain(string guess, vector<string>& wordlist) {
    // WARN_EMPTY_VECTOR(wordlist);
    double cumulative_information_gain = 0;
    double base_entropy = log2(wordlist.size());
    for (auto word_hyp : wordlist) {
        vector<string> compatible_words = get_compatible_words(guess, make_guess_hint(word_hyp, guess), wordlist);
        // WARN_EMPTY_VECTOR(compatible_words);
        double entropy_after_guess = log2(compatible_words.size());
        cumulative_information_gain += (base_entropy - entropy_after_guess);
    }
    return cumulative_information_gain / wordlist.size();
}

tuple<vector<string>, double> get_recommendation(vector<string>& wordlist, vector<string>& valid_guesses) {
    // auto t0 = chrono::high_resolution_clock::now();
    vector<double> expected_information_gains(valid_guesses.size(), false);
    #pragma omp parallel for schedule(dynamic) num_threads(MAX_THREADS)
    for (auto word_hyp : wordlist) {
        for (int i = 0; i < int(valid_guesses.size()); i++) {
            auto guess_candidate = valid_guesses[i];
            int num_compatible_words = get_num_compatible_words(guess_candidate, make_guess_hint(word_hyp, guess_candidate), wordlist);
            double base_entropy = log2(wordlist.size());
            double entropy_after_guess = log2(num_compatible_words);
            expected_information_gains[i] += (base_entropy - entropy_after_guess) / wordlist.size();
        }
    }
    // Find the best guess(es) for information gain. Return a list of guesses if there are multiple, otherwise a single guess.
    double highest_information_gain = 0;
    vector<string> words_with_highest_information_gain;
    for (int i = 0; i < int(valid_guesses.size()); i++) {
        if (expected_information_gains[i] > highest_information_gain) {
            highest_information_gain = expected_information_gains[i];
            words_with_highest_information_gain = {valid_guesses[i]};
        } else if (expected_information_gains[i] == highest_information_gain) {
            words_with_highest_information_gain.push_back(valid_guesses[i]);
        }
    }
    // auto t1 = chrono::high_resolution_clock::now();
    // auto duration = chrono::duration_cast<chrono::microseconds>(t1 - t0).count();
    // cout << "get_recommendation took " << duration << " microseconds" << endl;
    return make_tuple(words_with_highest_information_gain, highest_information_gain);
}

vector<string> get_recommended_words(vector<string> wordlist, vector<string> valid_guesses) {
    auto [recommended_words, highest_information_gain] = get_recommendation(wordlist, valid_guesses);
    return recommended_words;
}

string get_recommended_word(vector<string> wordlist, vector<string> valid_guesses) {
    return get_recommended_words(wordlist, valid_guesses)[0];
}

tuple<string, double> get_expected_num_turns_to_win(vector<string>& wordlist, vector<string>& valid_guesses) {
    // Calculate the minimum number of turns to win if using the strategy of guessing the word with the highest information gain 
    // out of either all valid guesses or just the words remaining in the wordlist.
    // Note: the winning guess counts as one guess.
    // Calculate the minimum number of turns recursively.
    // First, guess the word with the highest information gain out of all valid guesses.
    double expected_min_turns1 = 0;
    string guess1 = get_recommended_word(wordlist, valid_guesses);
    for (auto word_hyp : wordlist) {
        auto wordlist_remaining = get_compatible_words(guess1, make_guess_hint(word_hyp, guess1), wordlist);
        if (wordlist_remaining.size() == 1) {
            expected_min_turns1 += 1;
        } else {
            auto [best_guess, expected_min_turns] = get_expected_num_turns_to_win(wordlist_remaining, valid_guesses);
            expected_min_turns1 += expected_min_turns;
        }
    }
    expected_min_turns1 /= wordlist.size();
    expected_min_turns1 += 1;
    // Now, guess the word with the highest information gain out of the words remaining in the wordlist.
    double expected_min_turns2 = 0;
    string guess2 = get_recommended_word(wordlist, wordlist);
    for (auto word_hyp : wordlist) {
        auto wordlist_remaining = get_compatible_words(guess2, make_guess_hint(word_hyp, guess2), wordlist);
        if (wordlist_remaining.size() == 1) {
            expected_min_turns2 += 1;
        } else {
            auto [best_guess, expected_min_turns] = get_expected_num_turns_to_win(wordlist_remaining, wordlist);
            expected_min_turns2 += expected_min_turns;
        }
    }
    expected_min_turns2 /= wordlist.size();
    expected_min_turns2 += 1;
    return make_tuple(guess1, expected_min_turns1 < expected_min_turns2 ? expected_min_turns1 : expected_min_turns2);
}

map<string, int> get_hint_frequencies(string guess, vector<string>& wordlist) {
    map<string, int> hint_frequencies;
    for (auto word_hyp : wordlist) {
        hint_frequencies[make_guess_hint(word_hyp, guess)]++;
    }
    return hint_frequencies;
}

// A class that manages a single game of Wordle. Provides methods for playing a single turn of the game either interactively or automatically,
// for printing the instructions, asking for user input, and for printing the results of the game. To play a game interactively, call the methods
// in your own (external) game loop.
class WordleSolver {
public:
    vector<string> wordlist;
    vector<string> valid_guesses;
    vector<string> wordlist_remaining;
    vector<string> valid_guesses_remaining;
    bool verbose=false;
    set<char> eliminated_letters;
    double expected_num_turns_to_win=-1;
    string recommended_for_information_gain;
    string recommended_for_immediate_win;
    string recommended_overall;
    double highest_information_gain;
    double highest_information_gain_for_immediate_win;
    double highest_information_gain_overall;
    WordleSolver(vector<string> wordlist, vector<string> valid_guesses) {
        this->wordlist = wordlist;
        this->valid_guesses = valid_guesses;
        this->wordlist_remaining = wordlist;
        this->valid_guesses_remaining = valid_guesses;
        this->recommended_for_information_gain = "soare";
        this->recommended_for_immediate_win = "raise";
        this->recommended_overall = this->recommended_for_information_gain;
        this->highest_information_gain = get_expected_information_gain(this->recommended_for_information_gain, this->wordlist);
        this->highest_information_gain_for_immediate_win = get_expected_information_gain(this->recommended_for_immediate_win, this->wordlist);
        this->highest_information_gain_overall = this->highest_information_gain;
        // Make sure the recommended word is in the list of valid guesses. If not, get calculate the recommended guesses.
        if (find(valid_guesses.begin(), valid_guesses.end(), recommended_overall) == valid_guesses.end()) {
            cout << "Recommended word '" << recommended_overall << "' is not in the list of valid guesses. Calculating new recommended guesses." << endl;
            update_recommended();
        }
    }

    void set_verbosity(bool verbose) {
        this->verbose = verbose;
    }

    void print_instructions() {
        cout << "Welcome to the Wordle Solver!" << endl;
        cout << "Wordle is a game played by guessing a random 5-letter word." << endl;
        cout << "The game ends when you guess the word correctly. When you guess incorrectly, the game will give you a hint by telling you which letters are in the word, whether they are in the correct position (in which case they will be coloured green), in the word but in the wrong position (yellow), or not in the word at all (gray)." << endl;
        cout << "To use this solver, you will need to enter your guesses one at a time. The solver will give you a suggestion for the next guess based on maximising information gain. After you choose a guess, the solver will ask you to enter the result. The solver will then give you a new suggestion by maximizing information gain on the remaining words." << endl;
        cout << "You should enter your guesses in the format 'ggyyb' where each letter corresponds to one of the three hint colours: 'g' for green (right letter, right position), 'y' for yellow (right letter, wrong position), and 'b' for blue (wrong letter)." << endl;
        cout << "Let's begin!" << endl;
    }

    bool guess_is_valid(string guess, bool verbose=true) {
        // Ensure guess is valid.
        if (find(valid_guesses_remaining.begin(), valid_guesses_remaining.end(), guess) == valid_guesses_remaining.end()) {
            if (verbose) {
                cout << "Invalid guess. Please enter a guess from the list of valid guesses." << endl;
            }
            return false;
        }
        if (guess.empty() or guess.length() != 5) {
            if (verbose) {
                cout << "Please enter a 5-letter string." << endl;
            }
            return false;
        }
        return true;
    }

    string next_step_details(string guess, string hint, int frequency) {
        stringstream ss;
        auto compatible_words = get_compatible_words(guess, hint, wordlist_remaining);
        string best_next_guess = get_recommended_word(compatible_words, valid_guesses_remaining);
        string frequency_str = to_string(frequency);
        while (frequency_str.length() < 3) {
            frequency_str = " " + frequency_str;
        }
        float frequency_proportion = (float)frequency / wordlist_remaining.size();
        // Print as a percentage with 2 decimal places.
        string frequency_proportion_str = to_string(frequency_proportion).substr(0, 5);
        // Also print a solid bar of length proportional to the frequency proportion
        const int MAX_BAR_LENGTH = 50;
        int bar_length = round(frequency_proportion * MAX_BAR_LENGTH);
        string bar = "[";
        for (int i = 0; i < MAX_BAR_LENGTH; i++) {
            if (i < bar_length - 1) {
                bar += "■";
            } else {
                bar += "·";
            }
        }
        bar += "]";
        ss << hint_to_string(hint) << " " << best_next_guess << " " << frequency_str << "  " << frequency_proportion_str << "% " << bar << endl;
        return ss.str();
    }

    string prompt_for_guess(string suggestion="") {
        string guess;
        while (true) {
            if (suggestion.empty()) {
                cout << "Enter a guess: ";
            } else {
                cout << "Enter a guess (or press Enter to use '" << suggestion << "'): ";
            }
            getline(cin, guess);
            if (guess.empty() and !suggestion.empty()) {
                guess = suggestion;
            }
            if (guess_is_valid(guess)) {
                break;
            }
        }
        // Print the information gain for the guess and ask the user if they want to continue.
        if (guess != suggestion) {
            while (true) {
                cout << "Information gain for guess " << guess << ": " << get_expected_information_gain(guess, wordlist_remaining) << " bits" << endl;
                map<string, int> hint_frequencies = get_hint_frequencies(guess, wordlist_remaining);
                cout << "Number of compatible hint configurations: " << hint_frequencies.size() << endl;
                // Order from most frequent to least frequent
                vector<pair<string, int>> hint_frequencies_ordered;
                for (auto hint_frequency : hint_frequencies) {
                    hint_frequencies_ordered.push_back(make_pair(hint_frequency.first, hint_frequency.second));
                }
                sort(hint_frequencies_ordered.begin(), hint_frequencies_ordered.end(), [](const pair<string, int>& a, const pair<string, int>& b) {
                    return a.second > b.second;
                });
                for (auto hint_frequency : hint_frequencies_ordered) {
                    cout << next_step_details(guess, hint_frequency.first, hint_frequency.second);
                }
                cout << "Do you want to continue? (press Enter or type another guess): ";
                string continue_str;
                getline(cin, continue_str);
                if (continue_str.empty()) {
                    break;
                } else {
                    string new_guess = continue_str;
                    if (guess_is_valid(new_guess)) {
                        guess = new_guess;
                    }
                }
            }
        }
        // Convert to lowercase and return
        transform(guess.begin(), guess.end(), guess.begin(), ::tolower);
        return guess;
    }

    string prompt_for_result() {
        string result;
        while (true) {
            cout << "Enter the result: ";
            getline(cin, result);
            if (result.size() != 5) {
                cout << "Please enter a 5-letter string." << endl;
                continue;
            }
            if (result.find_first_not_of("byg") != string::npos) {
                cout << "Please enter a 5-letter string consisting of the letters 'b', 'y', and 'g'." << endl;
                continue;
            }
            break;
        }
        return result;
    }

    string current_state() {
        if (wordlist_remaining.size() == 0) {
            return "No more words remaining. Something has gone wrong. Did you enter an invalid guess?";
        }
        if (wordlist_remaining.size() == 1) {
            return "The word is " + wordlist_remaining[0] + ".";
        }
        if (wordlist_remaining.size() == 2) {
            return "The words is either " + wordlist_remaining[0] + " or " + wordlist_remaining[1] + ".";
        }
        stringstream ss;
        ss << "The words remaining are: ";
        for (int i = 0; i < min(10, int(wordlist_remaining.size())); i++) {
            ss << wordlist_remaining[i];
            if (i+1 < min(10, int(wordlist_remaining.size()))) {
                ss << ", ";
            }
        }
        if (wordlist_remaining.size() > 10) {
            ss << " and " << wordlist_remaining.size() - 10 << " more.";
        }
        ss << endl;

        ss << "1. Recommended guess overall: " << recommended_overall << " (" << highest_information_gain_overall << " bits)" << endl;
        ss << "2. Recommended guess for information gain: " << recommended_for_information_gain << " (" << highest_information_gain << " bits)" << endl;
        ss << "3. Recommended guess for immediate win: " << recommended_for_immediate_win << " (" << highest_information_gain_for_immediate_win << " bits)";
        if (expected_num_turns_to_win > 0) {
            ss << endl << "Expected number of turns to win: " << expected_num_turns_to_win;
        }
        return ss.str();
    }

    bool play_turn_interactively(string word_gt="") {
        cout << current_state() << endl;
        string guess;
        guess = prompt_for_guess(recommended_overall);
        string hint;
        if (word_gt.empty()) {
            hint = prompt_for_result();
        } else {
            hint = make_guess_hint(word_gt, guess);
            // Print the letters of the hint in their respective font colour
            cout << "Hint: " << hint_to_string(hint) << endl;
        }
        // cout << pretty_print_hint(guess, hint);
        update(guess, hint);
        cout << endl;
        return solved();
    }

    // Automatic (and silent) version of play_turn_interactively()
    void play_turn_automatically(string word_gt, string guess="") {
        if (guess.empty()) {
            guess = recommended_overall;
        }
        string hint = make_guess_hint(word_gt, guess);
        if (verbose) {
            cout << "Hint: " << hint_to_string(hint) << endl;
        }
        update(guess, hint);
    }

    void update_recommended() {
        auto [words_for_information_gain, highest_information_gain_] = get_recommendation(wordlist_remaining, valid_guesses_remaining);
        recommended_for_information_gain = words_for_information_gain[0];
        highest_information_gain = highest_information_gain_;
        auto [words_for_immediate_win, highest_information_gain_for_immediate_win_] = get_recommendation(wordlist_remaining, wordlist_remaining);
        recommended_for_immediate_win = words_for_immediate_win[0];
        highest_information_gain_for_immediate_win = highest_information_gain_for_immediate_win_;
        if (2 < wordlist_remaining.size() and wordlist_remaining.size() * log(wordlist_remaining.size()) * valid_guesses.size() < pow(10, 5)) {
            // cout << "Valid guesses remaining: " << valid_guesses_remaining.size() << endl;
            auto [best_word_overall, expected_num_turns_to_win_] = get_expected_num_turns_to_win(wordlist_remaining, valid_guesses);
            expected_num_turns_to_win = expected_num_turns_to_win_;
            recommended_overall = best_word_overall;
            if (recommended_overall == recommended_for_information_gain) {
                highest_information_gain_overall = highest_information_gain;
            } else {
                highest_information_gain_overall = highest_information_gain_for_immediate_win;
            }
        } else{
            if (highest_information_gain==highest_information_gain_for_immediate_win) {
                recommended_overall = recommended_for_immediate_win;
                highest_information_gain_overall = highest_information_gain_for_immediate_win;
            } else {
                recommended_overall = recommended_for_information_gain;
                highest_information_gain_overall = highest_information_gain;
            }
        }
    }

    void update_wordlists(string guess, string result) {
        wordlist_remaining = get_compatible_words(guess, result, wordlist_remaining);
        for (int i = 0; i < int(guess.length()); i++) {
            if (result[i] == 'b') {
                eliminated_letters.insert(guess[i]);
            }
        }
        valid_guesses_remaining = get_useful_guesses_using_guess_history(valid_guesses_remaining, eliminated_letters);
    }

    void update(string guess, string result) {
        update_wordlists(guess, result);
        update_recommended();
    }

    int num_words_remaining() {
        return wordlist_remaining.size();
    }

    bool solved() {
        return wordlist_remaining.size() <= 2;
    }

    float num_turns_remaining() {
        if (wordlist_remaining.size() == 1) {
            return 1-1;
        } else if (wordlist_remaining.size() == 2) {
            return 1.5-1;
        } else if (expected_num_turns_to_win > 0) {
            return expected_num_turns_to_win-1;
        } else {
            return -1;
        }
    }

    string get_the_word() {
        if (wordlist_remaining.size() == 1) {
            return wordlist_remaining[0];
        } 
        cout << "WARNING: get_the_word() called when there is more than one word remaining. Returning empty string." << endl;
        return "";
    }
};

// Rate a player's guesses by comparing each the expected information gain of each of their guesses
// with the expected information gain of the highest-information-gain guess at that step. The final
// score is the average difference between these two values.
vector<double> rate_guesses(vector<string> wordlist, vector<string> valid_guesses, vector<string> guesses={}, vector<string> results={}) {
    vector<double> expected_information_gains;
    vector<string> words_remaining = wordlist;
    for (int i = 0; i < int(guesses.size()); i++) {
        string guess = guesses[i];
        string result = results[i];
        expected_information_gains.push_back(get_expected_information_gain(guesses[guesses.size()-1], words_remaining));
        if (i!=0) {
            words_remaining = get_compatible_words(guess, result, words_remaining);
        }
    }
    return expected_information_gains;
}

double rate_guess(vector<string> wordlist, vector<string> valid_guesses, string guess, vector<string> prev_guesses={}, vector<string> prev_results={}) {
    prev_guesses.push_back(guess);
    vector<string> results = prev_results;
    return rate_guesses(wordlist, valid_guesses, prev_guesses, results)[-1];
}


int play_automatically(WordleSolver solver, string word_gt, bool verbose=false, int num_guesses=-1, int num_words_allowed_remaining=1) {
    // Play the game automatically
    int num_turns = 0;
    while (solver.num_words_remaining()>num_words_allowed_remaining and (num_guesses == -1 or num_turns < num_guesses)) {
        if (verbose) {
            cout << "----------------------------------------" << endl;
            cout << solver.current_state() << endl;
        }
        string guess = solver.recommended_overall;
        if (verbose) {
            cout << "Guessing " << guess << endl;
        }
        solver.play_turn_automatically(word_gt, guess);
        num_turns++;
    }
    if (verbose) {
        cout << "----------------------------------------" << endl;
        cout << solver.current_state() << endl;
    }
    return num_turns + solver.num_turns_remaining();
}

float get_expected_num_turns_to_win_by_greedy_strategy(WordleSolver& solver, bool verbose=false) {
    float expected_num_turns_to_win = 0;
    for (string word : solver.wordlist_remaining) {
        auto num_turns_to_win = play_automatically(solver, word, false);
        if (verbose) {
            cout << "Expected number of turns to win using greedy strategy when the word is " << word << ": " << num_turns_to_win << endl;
        }
        expected_num_turns_to_win += num_turns_to_win;
    }
    return expected_num_turns_to_win / solver.wordlist_remaining.size();
}

float get_num_turns_to_win(WordleSolver& solver, string word_gt, bool verbose=false) {
    // Play the game automatically
    if (verbose) {
        cout << "----------------------------------------" << endl;
        cout << "--------- BEGIN AUTOMATIC PLAY ---------" << endl;
        cout << "The word is " << word_gt << endl;
    }
    float num_turns = 0;
    while (solver.num_turns_remaining()==-1) {
        string guess = solver.recommended_overall;
        if (verbose) {
            cout << "----------------------------------------" << endl;
            cout << solver.current_state() << endl;
            cout << "Guessing " << guess << endl;
        }
        solver.play_turn_automatically(word_gt, guess);
        num_turns++;
    }
    if (verbose) {
        cout << "----------------------------------------" << endl;
        cout << solver.current_state() << endl;
    }
    return num_turns + solver.num_turns_remaining();
}

vector<float> get_num_turns_to_win_by_word(WordleSolver& solver, bool verbose=false) {
    vector<float> num_turns_to_win;
    for (auto word_gt : solver.wordlist_remaining) {
        // Make a copy of the solver
        WordleSolver solver_copy = solver;
        num_turns_to_win.push_back(get_num_turns_to_win(solver_copy, word_gt, verbose));
    }
    return num_turns_to_win;
}

float get_expected_num_turns_to_win(WordleSolver& solver, bool verbose=false) {
    vector<float> num_turns_to_win = get_num_turns_to_win_by_word(solver, verbose);
    float expected_num_turns_to_win = 0;
    for (auto num_turns : num_turns_to_win) {
        expected_num_turns_to_win += num_turns;
    }
    expected_num_turns_to_win /= num_turns_to_win.size();
    return expected_num_turns_to_win;
}

float get_expected_num_turns_to_win_given_guess_exhaustive(string guess, vector<string>& wordlist, vector<string>& valid_guesses, int verbosity_levels=0, float max_expectation=10) {
    if (max_expectation <= 0.67) {
        if (verbosity_levels>0) {
            // cout << "Skipping " << guess << " because max_expectation is " << max_expectation << endl;
        }
        return max_expectation + LARGE_NUMBER;
    }
    float expected_num_turns_to_win_accumulator = 0;
    float max_accumulator_value = max_expectation * wordlist.size();
    if (verbosity_levels>0) {
        cout << "----------------------------------------" << endl;
        cout << "guess: " << guess << ", level: " << verbosity_levels << ", num_words_remaining: " << wordlist.size() << ", max_accumulator_value: " << max_accumulator_value << ", max_expectation: " << max_expectation << endl;
    }
    map<string, float> hint_expectation_cache;
    for (int i = 0; i < int(wordlist.size()); i++) {
        string word_hyp = wordlist[i];
        if (word_hyp == guess) {
            // 'zero turns' to win, so add nothing; just continue
            continue;
        }
        string hint = make_guess_hint(word_hyp, guess);
        // If in cache, add cached value to accumulator
        if (hint_expectation_cache.count(hint) > 0) {
            expected_num_turns_to_win_accumulator += hint_expectation_cache[hint];
            continue;
        }
        vector<string> words_remaining = get_compatible_words(guess, hint, wordlist);
        if (words_remaining.size() == 1) {
            expected_num_turns_to_win_accumulator += 1;
            hint_expectation_cache[hint] = 1;
        } else if (words_remaining.size() == 2) {
            expected_num_turns_to_win_accumulator += 1.5;
            hint_expectation_cache[hint] = 1.5;
        } else {
            vector<string> useful_valid_guesses = valid_guesses;
            // if (words_remaining.size() < 10) {
            //     useful_valid_guesses = get_useful_guesses(words_remaining, valid_guesses);
            // }
            float expected_num_turns_to_win_given_guess_and_best_next_guess = max_expectation - 1;
            for (string next_guess : valid_guesses) {
                float expected_num_turns_to_win_given_guess_and_next_guess = get_expected_num_turns_to_win_given_guess_exhaustive(next_guess, words_remaining, valid_guesses, verbosity_levels-1, expected_num_turns_to_win_given_guess_and_best_next_guess) + 1;
                if (expected_num_turns_to_win_given_guess_and_next_guess < expected_num_turns_to_win_given_guess_and_best_next_guess) {
                    expected_num_turns_to_win_given_guess_and_best_next_guess = expected_num_turns_to_win_given_guess_and_next_guess;
                }
            }
            expected_num_turns_to_win_accumulator += expected_num_turns_to_win_given_guess_and_best_next_guess;
            hint_expectation_cache[hint] = expected_num_turns_to_win_given_guess_and_best_next_guess;
        }
        if (max_accumulator_value < expected_num_turns_to_win_accumulator) {
            break;
        }
    }
    if (verbosity_levels>0) {
        cout << "expected_num_turns_to_win_accumulator: " << expected_num_turns_to_win_accumulator / wordlist.size() << endl;
        cout << "++++++++++++++++++++++++++++++++++++++++" << endl;
    }
    return expected_num_turns_to_win_accumulator / wordlist.size();
}

auto get_best_words_exhaustively(vector<string>& wordlist, vector<string>& valid_guesses, bool verbose=true) {
    if (verbose) {
        cout << "Beginning exhaustive search" << endl;
    }
    string best_guess = "";
    map<string, float> expected_num_turns_to_win;
    WordleSolver solver(wordlist, valid_guesses);
    // float best_ettw = get_expected_num_turns_to_win_by_greedy_strategy(solver, true);
    float best_ettw = 2.5;
    bool threaded_verbosity = verbose;
    #if defined(_OPENMP)
        if (verbose) {
            cout << "Using OpenMP in verbose mode will limit verbosity." << endl;
            threaded_verbosity = false;
        }
    #endif
    #pragma omp parallel for schedule(dynamic) num_threads(MAX_THREADS)
    for (string guess : valid_guesses) {
        float ettw = get_expected_num_turns_to_win_given_guess_exhaustive(guess, wordlist, valid_guesses, threaded_verbosity*3, best_ettw);
        if (ettw < best_ettw) {
            best_guess = guess;
            best_ettw = ettw;
            expected_num_turns_to_win[guess] = ettw;
            if (verbose) {
                cout << "New best guess: " << best_guess << " with ettw = " << best_ettw << endl;
            }
        } else if (verbose) {
            cout << "Guess " << guess << " has ettw = " << ettw << ", which is worse than the best guess so far" << endl;
        }
    }
    if (verbose) {
        cout << "Finished exhaustive search. Top 10 guesses: " << endl;
        // Get top 10 guesses
        vector<pair<string, float>> guesses_and_ettws;
        for (auto& guess_and_ettw : expected_num_turns_to_win) {
            guesses_and_ettws.push_back(make_pair(guess_and_ettw.first, guess_and_ettw.second));
        }
        sort(guesses_and_ettws.begin(), guesses_and_ettws.end(), [](const pair<string, float>& a, const pair<string, float>& b) {
            return a.second < b.second;
        });
        for (int i = 0; i < 10; i++) {
            cout << guesses_and_ettws[i].first << ": " << guesses_and_ettws[i].second << endl;
        }
    }
    return expected_num_turns_to_win;
}

auto get_best_word_exhaustively(vector<string>& wordlist, vector<string>& valid_guesses, bool verbose=true) {
    string best_guess = "";
    float best_ettw = LARGE_NUMBER;
    for (auto [guess, ettw] : get_best_words_exhaustively(wordlist, valid_guesses, verbose)) {
        if (ettw < best_ettw) {
            best_guess = guess;
            best_ettw = ettw;
        }
    }
    return tuple(best_guess, best_ettw);
}


void calculate_best_opening(vector<string> wordlist, vector<string> valid_guesses, bool verbose=false) {
    cout << "Calculating best opening" << endl;
    auto [best_guess, best_ettw] = get_best_word_exhaustively(wordlist, valid_guesses, verbose);
    cout << "Best guess is " << best_guess << " with " << best_ettw << " turns to win" << endl;
}

void play_interactively(WordleSolver& solver, string word_gt) {
    // Choose a random word from the wordlist
    // Play the game automatically
    while (!solver.solved()) {
        solver.play_turn_interactively(word_gt);
    }
    // Print the final state
    cout << solver.current_state() << endl;
}

void calculate_highest_entropy_opening(WordleSolver& solver, bool verbose=false) {
    string option1 = solver.recommended_for_information_gain;
    string option2 = solver.recommended_for_immediate_win;
    WordleSolver solver_copy1 = solver;
    WordleSolver solver_copy2 = solver;
    solver_copy1.recommended_overall = option1;
    solver_copy2.recommended_overall = option2;
    float expected_num_turns_to_win_option1 = get_expected_num_turns_to_win(solver_copy1, verbose);
    float expected_num_turns_to_win_option2 = get_expected_num_turns_to_win(solver_copy2, verbose);
    cout << "Option 1: " << option1 << " (" << expected_num_turns_to_win_option1 << " turns)" << endl;
    cout << "Option 2: " << option2 << " (" << expected_num_turns_to_win_option2 << " turns)" << endl;
    if (expected_num_turns_to_win_option1 < expected_num_turns_to_win_option2) {
        cout << "Option 1 is better" << endl;
    } else if (expected_num_turns_to_win_option1 > expected_num_turns_to_win_option2) {
        cout << "Option 2 is better" << endl;
    } else {
        cout << "Both options are equally good" << endl;
    }
}

void test_make_guess_hint() {
    auto hint1 = make_guess_hint("abcde", "acbfa");
    cout << "make_guess_hint(\"abcde\", \"acbfa\") = " << hint1 << endl;
    assert (hint1 == "gyybb");
    cout << "All tests passed" << endl;
}

void test_word_is_compatible_with_guess_hint() {
    auto iscompatible1 = word_is_compatible_with_guess_hint("abcde", "acbfa", "gyybb");
    cout << "word_is_compatible_with_guess_hint(\"abcde\", \"acbfa\", \"gyybb\") = " << iscompatible1 << endl;
    assert (iscompatible1);
    auto iscompatible2 = word_is_compatible_with_guess_hint("abcde", "acbfa", "ybbgg");
    cout << "word_is_compatible_with_guess_hint(\"abcde\", \"acbfa\", \"ybbgg\") = " << iscompatible2 << endl;
    assert (!iscompatible2);
    auto iscompatible3 = word_is_compatible_with_guess_hint("abcde", "acbfa", "bggyy");
    cout << "word_is_compatible_with_guess_hint(\"abcde\", \"acbfa\", \"bggyy\") = " << iscompatible3 << endl;
    assert (!iscompatible3);
    auto iscompatible4 = word_is_compatible_with_guess_hint("abcde", "acbfa", "ggggg");
    cout << "word_is_compatible_with_guess_hint(\"abcde\", \"acbfa\", \"ggggg\") = " << iscompatible4 << endl;
    assert (!iscompatible4);
    auto iscompatible5 = word_is_compatible_with_guess_hint("abcde", "acbfa", "yyyyy");
    cout << "word_is_compatible_with_guess_hint(\"abcde\", \"acbfa\", \"yyyyy\") = " << iscompatible5 << endl;
    assert (!iscompatible5);
    auto iscompatible6 = word_is_compatible_with_guess_hint("abcde", "acbfa", "bbbbb");
    cout << "word_is_compatible_with_guess_hint(\"abcde\", \"acbfa\", \"bbbbb\") = " << iscompatible6 << endl;
    assert (!iscompatible6);
    cout << "All tests passed" << endl;
}

template<typename T>
string to_string(vector<T>& v) {
    stringstream ss;
    for (auto x : v) {
        ss << x << " ";
    }
    return ss.str();
}

vector<string> make_synthetic_wordlist() {
    // Make a wordlist of all available words of length 5 with the characters 'a', 'b', 'c', 'd', 'e'
    vector<string> wordlist;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            for (int k = 0; k < 5; k++) {
                for (int l = 0; l < 5; l++) {
                    for (int m = 0; m < 5; m++) {
                        wordlist.push_back(string(1, 'a' + i) + string(1, 'b' + j) + string(1, 'c' + k) + string(1, 'd' + l) + string(1, 'e' + m));
                    }
                }
            }
        }
    }
    return wordlist;
}

void test_get_compatible_words() {
    vector<string> wordlist = make_synthetic_wordlist();
    auto words1 = get_compatible_words("abcde", "ggggg", wordlist);
    cout << "get_compatible_words(\"abcde\", \"ggggg\", wordlist) = " << to_string(words1) << endl;
    assert ((words1 == vector<string>{"abcde"}));
    auto words2 = get_compatible_words("aaabc", "gggyy", wordlist);
    cout << "get_compatible_words(\"aaabc\", \"gggyy\", wordlist) = " << to_string(words2) << endl;
    assert ((words2 == vector<string>{"aaacb"}));
}

void test_game1() {
    auto valid_guesses = make_synthetic_wordlist();
    // Make wordlist first half of valid_guesses
    auto wordlist = vector<string>();
    for (int i = 0; i < valid_guesses.size() / 2; i++) {
        wordlist.push_back(valid_guesses[i]);
    }
    WordleSolver solver(wordlist, valid_guesses);
    // Set the ground truth to the last word in the wordlist
    string word_gt = wordlist[wordlist.size() - 1];
    auto num_turns1 = play_automatically(solver, word_gt);
    cout << "play_automatically(solver, \"" << word_gt << "\").wordlist_remaining.size() = " << solver.wordlist_remaining.size() << endl;
    assert (solver.num_words_remaining() == 0);
    cout << "play_automatically(solver, \"" << word_gt << "\").get_the_word() = " << solver.get_the_word() << endl;
    assert (solver.get_the_word() == word_gt);
}

// Example:
// $ ./wordle_solver --mode auto --word "raise" --num_guesses 6 --verbose
int main(int argc, char** argv) {
    // test_make_guess_hint();
    // test_word_is_compatible_with_guess_hint();
    // test_game1();

    // Read in the solutions file, a json list of words that can be word of the day, and the valid guesses file, a json list of words that can be guessed
    auto [wordlist, valid_guesses] = load_wordle("wordlesolver/solutions_nyt.json", "wordlesolver/nonsolutions_nyt.json");
    // Parse the command line arguments
    string mode = "interactive";
    string word_gt = "";
    int num_guesses = 6;
    bool verbose = false;
    int use_first_n_words = -1;
    int use_first_n_guesses = -1;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--mode") {
            mode = argv[i+1];
        } else if (arg == "--word") {

            word_gt = argv[i+1];
        } else if (arg == "--num_guesses") {
            num_guesses = stoi(argv[i+1]);
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--num-words") {
            use_first_n_words = stoi(argv[i+1]);
        } else if (arg == "--num-valid-guesses") {
            use_first_n_guesses = stoi(argv[i+1]);
        } else if (arg == "-n") {
            if (use_first_n_words == -1) {
                use_first_n_words = stoi(argv[i+1]);
            }
            if (use_first_n_guesses == -1) {
                use_first_n_guesses = stoi(argv[i+1]);
            }
        }
    }
    if (use_first_n_words > 0) {
        wordlist = vector<string>(wordlist.begin(), wordlist.begin() + use_first_n_words);
    }
    if (use_first_n_guesses > 0) {
        valid_guesses = vector<string>(valid_guesses.begin(), valid_guesses.begin() + use_first_n_guesses);
    }
    if (word_gt == "random") {
        word_gt = wordlist[rand() % wordlist.size()];
    }
    // Initialise the solver
    WordleSolver solver(wordlist, valid_guesses);
    solver.set_verbosity(verbose);
    // In auto mode, play the game automatically
    // In interactive mode, play the game interactively, prompting the user to enter the results for each guess (default)
    if (mode == "auto") {
        play_automatically(solver, word_gt, verbose, 6);
    } else if (mode == "interactive") {
        play_interactively(solver, word_gt);
    } else if (mode == "calculate-highest-entropy-opener") {
        calculate_highest_entropy_opening(solver, verbose);
    } else if (mode == "calculate-best-opener") {
        calculate_best_opening(wordlist, valid_guesses, verbose);
    } else {
        cout << "Invalid mode" << endl;
    } 
    return 0;
}

// To compile and run with libomp:
// g++ -std=c++20 -fopenmp -lomp wordle.cpp -o wordle && ./wordle
// g++ -std=c++20 -fopenmp -lomp wordle.cpp -o wordle && ./wordle --mode calculate-best-opener --verbose
// or
// clang++ -std=c++17 -fopenmp -O3 -march=native -I/usr/local/include -L/usr/local/lib -lomp wordle.cpp -o wordle
// Using Emscripten and Node:
// emcc -O3 -std=c++20 -sASSERTIONS -s NO_DISABLE_EXCEPTION_CATCHING -s NODERAWFS=1 wordle.cpp -o wordle.js && node wordle.js
// Export using emcc
// emcc -O3 -std=c++20 -s ASSERTIONS=1 -s NO_DISABLE_EXCEPTION_CATCHING -s NODERAWFS=1 -s EXPORTED_FUNCTIONS=_main EXPORTED_RUNTIME_METHODS=ccall,cwrap wordle.cpp -o wordle.js && node wordle.js

// clang -Xpreprocessor -fopenmp -lomp -I"$(brew --prefix libomp)/include" -L"$(brew --prefix libomp)/lib" myfile.cxx
