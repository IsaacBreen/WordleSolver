import json
import numpy as np
from tqdm import tqdm

TEST_MODE = True

def load_wordle(wordlist_filename, valid_guesses_filename):
    # Read in the solutions file, a json list of words that can be word of the day
    with open(wordlist_filename) as solutions_file:
        wordlist_json = json.load(solutions_file)
        wordlist = set(wordlist_json)
    # Read in the valid guesses file
    with open(valid_guesses_filename) as valid_guesses_file:
        valid_guesses_json = json.load(valid_guesses_file)
        valid_guesses = set(valid_guesses_json)
    # Add all words in the wordlist to valid_guesses
    valid_guesses.update(wordlist)
    return list(wordlist), list(valid_guesses)

wordlist, valid_guesses = load_wordle("wordlesolver/solutions_nyt.json", "wordlesolver/nonsolutions_nyt.json")
print("There are " + str(len(wordlist)) + " solutions and " + str(len(valid_guesses)) + " valid guesses.")

# A guess result is given as a string of the form 'ggyyb' where 'g' stands for green (positional matche), 'y' for yellow (positionally exclusive match), and 'b' for black (complete exclusion).
# - A positional match is a character that is in the given position. It is coloured green.
# - A positionally exclusive match is a character that is in the word but not in the given position. It is yellow.
# - A complete exclusion is a character that is not in the word. It is black.
# - A mask is a list of booleans indicating whether the match vector applies to the given position.
def make_guess_result(word, guess):
    guess_result = []
    for i in range(len(guess)):
        if guess[i] == word[i]:
            guess_result.append('g')
        elif guess[i] in word:
            guess_result.append('y')
        else:
            guess_result.append('b')
    return ''.join(guess_result)

def word_is_compatible_with_guess_result(word_hyp, guess, guess_result):
    for i in range(len(guess)):
        if guess_result[i] == "g":
            if word_hyp[i] != guess[i]:
                return False
        elif guess_result[i] == "y":
            if word_hyp[i] == guess[i] or guess[i] not in word_hyp:
                return False
        elif guess_result[i] == "b":
            if guess[i] in word_hyp:
                return False
    return True

def get_compatible_words(guess, guess_result, wordlist):
    compatible_words = [word for word in wordlist if word_is_compatible_with_guess_result(word, guess, guess_result)]
    return compatible_words

def get_num_compatible_words(word_gt, guess, wordlist):
    guess_result = make_guess_result(word_gt, guess)
    return len(get_compatible_words(guess, guess_result, wordlist))

def get_entropy(word_gt, guess, wordlist):
    num_compatible_words = get_num_compatible_words(word_gt, guess, wordlist)
    return np.log2(num_compatible_words)

def get_expected_entropy(guess, wordlist):
    return np.mean([get_entropy(word_gt, guess, wordlist) for word_gt in wordlist])

def get_information_gain(word_gt, guess, wordlist):
    base_entropy = np.log2(len(wordlist))
    entropy_after_guess = get_entropy(word_gt, guess, wordlist)
    return base_entropy - entropy_after_guess

def get_expected_information_gain(guess, wordlist):
    return np.mean([get_information_gain(word_gt, guess, wordlist) for word_gt in wordlist])

def helper_wordlist_to_english_list(wordlist, max_display=None, quote=True):
    if quote==True:
        wordlist = [f"'{word}'" for word in wordlist]
    if len(wordlist) == 1:
        return wordlist[0]
    if max_display is None or len(wordlist) <= max_display:
        return ", ".join(wordlist[:-1]) + " and " + wordlist[-1]
    else:
        return ", ".join(wordlist[:max_display]) + " and " + str(len(wordlist) - max_display) + " others"

def get_recommended(wordlist, valid_guesses):
    expected_information_gains = {guess_candidate: get_expected_information_gain(guess_candidate, wordlist) for guess_candidate in tqdm(valid_guesses)}
    # Find the best guess(es) for information gain. Return a list of guesses if there are multiple, otherwise a single guess.
    def helper(expected_information_gains):
        highest_information_gain = max(expected_information_gains.values())
        words_with_highest_information_gain = [word for word, expected_information_gain in expected_information_gains.items() if expected_information_gain == highest_information_gain]
        recommended = (words_with_highest_information_gain, highest_information_gain)
        return recommended
    recommended_for_information = helper(expected_information_gains)
    recommended_for_immediate_win = helper({word: expected_information_gains[word] for word in wordlist})
    # Find the best guess for an immediate win
    # TODO: Find the best guess overall (assume greedily maximising information gain is the best strategy)
    if recommended_for_information[1] > recommended_for_immediate_win[1]:
        recommended_overall = recommended_for_information
    else:
        recommended_overall = recommended_for_immediate_win
    return recommended_for_information, recommended_for_immediate_win, recommended_overall

def get_optimal_guess_for_immediate_win(wordlist, valid_guesses, return_moves=False):
    if len(wordlist) == 0:
        print("No candidate words. Are you sure you entered your results correctly?")
        return 999, True if return_moves else 999
    if len(wordlist) == 1:
        return 1, True if return_moves else 1
    recommended_for_information, recommended_for_immediate_win, recommended_overall = get_recommended(wordlist, valid_guesses)
    expected_moves_to_win_with_recommended_for_information = 0
    expected_moves_with_recommended_for_immediate_win = 0
    for word_hyp in wordlist:
        # Suppose we use the recommended word for information gain.
        guess_result = make_guess_result(word_hyp, recommended_for_information[0][0])
        wordlist_remaining = get_compatible_words(recommended_for_information[0][0], guess_result, wordlist)
        new_expected_moves_to_win_with_recommended_for_information, _ = get_optimal_guess_for_immediate_win(wordlist_remaining, valid_guesses, return_moves=True)
        expected_moves_to_win_with_recommended_for_information += new_expected_moves_to_win_with_recommended_for_information

        # Suppose we use the recommended word for win.
        guess_result = make_guess_result(word_hyp, recommended_for_immediate_win[0][0])
        wordlist_remaining = get_compatible_words(recommended_for_immediate_win[0][0], guess_result, wordlist)
        new_expected_moves_with_recommended_for_immediate_win, _ = get_optimal_guess_for_immediate_win(wordlist_remaining, valid_guesses, return_moves=True)
        expected_moves_with_recommended_for_immediate_win += new_expected_moves_with_recommended_for_immediate_win
    best_choice_is_recommended_for_information = expected_moves_to_win_with_recommended_for_information < expected_moves_with_recommended_for_immediate_win
    fewest_expected_moves_to_win = expected_moves_to_win_with_recommended_for_information if best_choice_is_recommended_for_information else expected_moves_with_recommended_for_immediate_win
    best_choice_is_recommended_for_information = False
    if return_moves:
        return 1 + fewest_expected_moves_to_win/len(wordlist), best_choice_is_recommended_for_information
    else:
        return 1 + fewest_expected_moves_to_win/len(wordlist)

def main():
    # Interactive solver
    print("Enter results as a 5-letter string where each letter is:")
    print("  'b' for black/gray (not present)")
    print("  'y' for yellow (present but not at the specified position)")
    print("  'g' for green (present and at the specified position)")
    print("Choose '1. Best guess for information gain' if you want the most information, or '2. Best guess for compatibility' if you're feeling lucky and want a quick win.")
    print("For example, a result of yellow, yellow, green, green, gray corresponds to the result string 'yyggb'")
    recommended_for_information = (["soare"], get_expected_information_gain("soare", wordlist))
    recommended_for_immediate_win = (["raise"], get_expected_information_gain("raise", wordlist))
    recommended_overall = recommended_for_information
    wordlist_remaining = wordlist
    while (True):
        # Print the recommended guesses
        print(f"Recommended for information gain: {helper_wordlist_to_english_list(recommended_for_information[0], max_display=10)} ({recommended_for_information[1]:.2f} bits)")
        print(f"Recommended for an immediate win: {helper_wordlist_to_english_list(recommended_for_immediate_win[0], max_display=10)} ({recommended_for_immediate_win[1]:.2f} bits)")
        # Prompt user to enter their next guess
        while True:
            guess = input(f"Enter your guess (or press Enter to accept '{recommended_overall[0][0]}'): ").strip()
            if guess == "":
                guess = recommended_overall[0][0]
            else:
                guess = guess.lower()
            if len(guess) != 5:
                print("Please enter a 5-letter string.")
                continue
            if guess not in valid_guesses:
                if input("Warning: this guess is not valid. Are you sure you want to use it? (y/n)").strip()[0] != "y":
                    continue
            break
        # Prompt user to enter their result
        while True:
            result = input(f"Enter the result of guessing '{guess}': ").strip()
            if len(result) != 5:
                print("Please enter a 5-letter string.")
                continue
            if not all(c in "byg" for c in result):
                
                print("Please enter a 5-letter string consisting of the letters 'b', 'y', and 'g'.")
                continue
            break
        # Get the best guesses for the next step
        # Narrow down the wordlist
        wordlist_remaining = get_compatible_words(guess, result, wordlist_remaining)
        # If there are no remaining words, something has gone wrong
        if len(wordlist_remaining) == 0:
            print("No candidates left. Are you sure you entered your results correctly?")
            break
        # If there is only one remaining word, we're done
        if len(wordlist_remaining) == 1:
            print(f"The word is '{wordlist_remaining[0]}'")
            break
        # Print the words remaining if there are fewer than 10, otherwise print the number of words remaining
        if len(wordlist_remaining) < 10:
            print(f"Remaining words: {wordlist_remaining}")
        else:
            print(f"{len(wordlist_remaining)} words remaining.")
        # Find the expected information gains of all guesses
        recommended_for_information, recommended_for_immediate_win, recommended_overall = get_recommended(wordlist_remaining, valid_guesses)

if __name__ == '__main__':
    main()

