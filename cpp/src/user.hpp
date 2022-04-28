#include <common.hpp>
#include <data/data.hpp>
#include <hint.hpp>
#include <compatibility.hpp>
#include <optimiser.hpp>
#include <iostream>

#pragma once

void print_compatibility_matrix_header() {
    // Print all compatibilities in a table with word and guess on the vertical axis and hints on the horizontal axis
    string hint_result_lines[5];
    for (int k = 0; k < NUM_HINT_CONFIGS; k++) {
        string hint = hint_to_string(k);
        for (int i = 0; i < 5; i++) {
            char letter = hint[i];
            // Colour using escape sequences
            if (letter == 'g') {
                // Green
                hint_result_lines[i] += "\033[38;5;2m";
            } else if (letter == 'y') {
                hint_result_lines[i] += "\033[38;5;3m";
            } else {
                hint_result_lines[i] += "\033[38;5;233m";
            }
            hint_result_lines[i] += "█";
            hint_result_lines[i] += "\033[0m";
        }
    }
    for (int i = 0; i < 5; i++) {
        cout << "            " << hint_result_lines[i] << endl;
    }
}

void _print_compatibility_matrix(Word guess, Word word) {
    cout << words[word] << " " << words[guess] << " ";
    for (int h = 0; h < NUM_HINT_CONFIGS; h++) {
        if (word_is_compatible_with_guess_hint(word, guess, h)) {
            cout << "\033[38;5;2m█";
        } else {
            cout << "\033[38;5;233m█";
        }
    }
    cout << "\033[0m" << endl;
}

void _print_compatibility_matrix(Word guess) {
    for (int w = 0; w < NUM_WORDS; w++) {
        _print_compatibility_matrix(guess, w);
    }
}

void print_compatibility_matrix(Word guess, Word word) {
    print_compatibility_matrix_header();
    _print_compatibility_matrix(guess, word);
}

void print_compatibility_matrix(Word guess) {
    print_compatibility_matrix_header();
    _print_compatibility_matrix(guess);
}

void print_compatibility_matrix() {
    print_compatibility_matrix_header();
    for (int g = 0; g < NUM_GUESSES; g++) {
        _print_compatibility_matrix(g);
    }
    cout << endl;
}

void print_best_strategy() {
    // Print the best strategy for each word
    Strategy optimal_strategy = find_optimal_strategy(ALL_WORDS, 3, 1);
    cout << "Best strategy for first guess: " << optimal_strategy.get_guess() << endl;
}
