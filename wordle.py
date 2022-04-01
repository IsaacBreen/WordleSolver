import json
import math


class Condition:
    def __init__(self, in_word=None, not_in_word=None):
        self.in_word = in_word or {}
        self.not_in_word = not_in_word or set()

class WordleGame:
    def __init__(self, words, valid_guesses, base_entropy=-1):
        self.words = words
        self.valid_guesses = valid_guesses
        if base_entropy == -1:
            self.base_entropy = self.conditional_entropy(words)
        else:
            self.base_entropy = base_entropy

    def word_is_compatible(self, word, condition):
        is_compatible = True
        for letter, position_and_presence in condition.in_word.items():
            # Letter should be in word
            if word.find(letter) == -1:
                return False
            for position, is_present in position_and_presence.items():
                if is_present != (word[position] == letter):
                    return False
        for letter in condition.not_in_word:
            # Letter should not be in word
            if word.find(letter) != -1:
                return False
        return True

    def get_compatible_words(self, condition):
        compatible_words = []
        for word in self.words:
            if self.word_is_compatible(word, condition):
                compatible_words.append(word)
        return compatible_words

    def conditional_entropy(self, words, condition=None, verbose=False):
        compatible_words = self.get_compatible_words(condition) if condition else self.words
        # Entropy is the sum of the probabilities of each word multiplied by the log of the probability. Assume each word remaining is equally likely.
        entropy = 0
        for word in compatible_words:
            probability = 1.0 / len(compatible_words)
            entropy += probability * math.log2(probability)
        return -entropy

    def evaluate_guess_to_condition(self, ground_truth, guess):
        condition = Condition()
        for i in range(5):
            if ground_truth.find(guess[i]) != -1:
                condition.in_word[guess[i]] = {i: guess[i] == ground_truth[i]}
            else:
                condition.not_in_word.add(guess[i])
        return condition

    def combine_conditions(self, condition1, condition2):
        condition = Condition()
        for letter, position_and_presence in condition1.in_word.items():
            for position, is_present in position_and_presence.items():
                condition.in_word[letter] = {position: is_present}
        for letter in condition1.not_in_word:
            condition.not_in_word.add(letter)
        for letter, position_and_presence in condition2.in_word.items():
            for position, is_present in position_and_presence.items():
                condition.in_word[letter] = {position: is_present}
        for letter in condition2.not_in_word:
            condition.not_in_word.add(letter)
        # Remove from condition.not_in_word any letters that are in condition.in_word
        for letter, position_and_presence in condition.in_word.items():
            condition.not_in_word.discard(letter)
        return condition

    def expected_information_gain(self, guess, condition=None):
        information_gains = []
        for i in range(len(self.words)):
            ground_truth = self.words[i]
            new_condition = self.evaluate_guess_to_condition(ground_truth, guess)
            combined_condition = self.combine_conditions(condition, new_condition) if condition else new_condition
            information_gains.append(self.base_entropy - self.conditional_entropy(self.words, combined_condition))
        return sum(information_gains) / len(information_gains)

    def convert_pretty_condition_to_condition(self, guess, pretty_condition):
        # pretty_condition is a string of the form "nnypn" where:
        # g means the corresponding letter in guess is in the word at the corresponding position
        # y means the corresponding letter in guess is in the word but not at the corresponding position
        # b means the corresponding letter in guess is not in the word
        condition = Condition()
        for i in range(5):
            if pretty_condition[i] == 'g':
                condition.in_word[guess[i]] = {i: True}
            elif pretty_condition[i] == 'y':
                condition.in_word[guess[i]] = {i: False}
            elif pretty_condition[i] == 'b':
                condition.not_in_word.add(guess[i])
        return condition

    def apply_condition(self, condition):
        # Remove words that don't match the condition
        compatible_words = self.get_compatible_words(condition)
        self.words = compatible_words
        self.base_entropy = self.conditional_entropy(self.words)

    def find_optimal_guesses(self, condition):
        # Find the optimal guess for information gain and for compatibility
        best_guess_for_information_gain = ""
        best_guess_for_win = ""
        best_information_gain = 0
        best_information_gain_for_win = 0
        information_gains = dict()
        for guess in self.valid_guesses:
            this_information_gain = self.expected_information_gain(guess, condition)
            if this_information_gain == this_information_gain and this_information_gain != 0:
                information_gains[guess] = this_information_gain
        for guess, information_gain in information_gains.items():
            if information_gain > best_information_gain:
                best_guess_for_information_gain = guess
                best_information_gain = information_gain
            if information_gain > best_information_gain_for_win and self.word_is_compatible(guess, condition):
                best_guess_for_win = guess
                best_information_gain_for_win = information_gain
        return best_guess_for_information_gain, best_guess_for_win, best_information_gain, best_information_gain_for_win

    def won(self):
        return len(self.words) == 1

    def apply_guess_and_result(self, guess, result):
        new_condition = self.convert_pretty_condition_to_condition(guess, result)
        self.apply_condition(new_condition)

def prettify_condition(condition):
    # The string to return. We will add to this using << >>
    s = ""
    for letter, position_and_presence in condition.in_word.items():
        s += letter + " is"
        for position, is_present in position_and_presence.items():
            if is_present:
                s += " in position " + str(position)
            else:
                s += " not in position " + str(position)
            s += ","
        s += "\n"
    s += "Not in word: "
    for letter in condition.not_in_word:
        s += letter + " "
    return s

class WordleWords:
    def __init__(self, solutions_filename, valid_guesses_filename):
        self.words = []
        self.valid_guesses = []
        # Read in the solutions file, a json list of words
        with open(solutions_filename) as solutions_file:
            solutions_json = json.load(solutions_file)
            for solution in solutions_json:
                self.words.append(solution)
        # Read in the valid guesses file
        with open(valid_guesses_filename) as valid_guesses_file:
            valid_guesses_json = json.load(valid_guesses_file)
            for guess in valid_guesses_json:
                self.valid_guesses.append(guess)
        # Add all words to valid_guesses
        for word in self.words:
            self.valid_guesses.append(word)
        print("There are " + str(len(self.words)) + " solutions and " + str(len(self.valid_guesses)) + " valid guesses.")
        self.base_entropy = WordleGame(self.words, self.valid_guesses).base_entropy

    def new_game(self):
        return WordleGame(self.words, self.valid_guesses, self.base_entropy)

def main():
    wordle_words = WordleWords("wordlesolver/solutions_nyt.json", "wordlesolver/nonsolutions_nyt.json")
    wordle = wordle_words.new_game()
    # Get information of some words
    # words = "light goths liars trial oiler trail soare lores adieu sassy".split()
    # for word in words:
    #     print(word + ": " + str(wordle.expected_information_gain(word)))

    # # Interactive solver
    # print("Optimal first guess is cached")
    # print("Enter results as a 5-letter string where each letter is:")
    # print("  'b' for black/gray (not present)")
    # print("  'y' for yellow (present but not at the specified position)")
    # print("  'g' for green (present and at the specified position)")
    # print("Choose '1. Best guess for information gain' if you want the most information, or '2. Best guess for compatibility' if you're feeling lucky and want a quick win.")
    # print("For example, a result of yellow, yellow, green, green, gray corresponds to the result string 'yyggb'")
    # print("Enter soare")

    # # Start with the empty condition
    # condition = Condition()
    # word = "soare"
    # while True:
    #     result = ""
    #     while True:
    #         result = input("Result: ")
    #         # Validate the result
    #         result_ok = True
    #         if len(result) != 5:
    #             print("Result must be 5 letters long; got " + str(len(result)))
    #             result_ok = False
    #         for i in range(5):
    #             if result[i] != 'b' and result[i] != 'y' and result[i] != 'g':
    #                 print("Result must be a combination of the characters 'b', 'y', and 'g'; got " + result)
    #                 result_ok = False
    #                 break
    #         if result_ok:
    #             break
    #     new_condition = wordle.convert_pretty_condition_to_condition(word, result)
    #     condition = wordle.combine_conditions(condition, new_condition)
    #     print(word + " is " + result)
    #     print("New condition:\n" + prettify_condition(new_condition))
    #     print("Combined condition:\n" + prettify_condition(condition))
    #     wordle.apply_condition(condition)
    #     num_compatible_in_wordlist = len(wordle.get_compatible_words(condition))
    #     MAX_COMPATIBLE_WORDS = 10
    #     if num_compatible_in_wordlist == 0:
    #         print("No words are compatible with this condition")
    #         break
    #     if num_compatible_in_wordlist == 1:
    #         print("The word is " + wordle.words[0])
    #         break
    #     print("There are " + str(num_compatible_in_wordlist) + " compatible words remaining", end="")
    #     if num_compatible_in_wordlist > MAX_COMPATIBLE_WORDS:
    #         print()
    #     else:
    #         print(": ", end="")
    #         for word in wordle.words:
    #             print(word + " ", end="")
    #         print()
    #     best_guess_for_information_gain, best_guess_for_win, infogain1, infogain2 = wordle.find_optimal_guesses(condition)
    #     if infogain1 <= infogain2:
    #         print("The best guess is " + best_guess_for_win + " with an information gain of " + str(infogain1))
    #         word = best_guess_for_win
    #     else:
    #         print("1. Best guess for information gain: " + best_guess_for_information_gain + " (" + str(infogain1) + " bits)")
    #         print("2. Best guess for compatibility: " + best_guess_for_win + " (" + str(infogain2) + " bits)")
    #         print("Which guess do you want to use? (1 or 2)")
    #         while True:
    #             choice = input()
    #             if choice == "1":
    #                 word = best_guess_for_information_gain
    #                 break
    #             elif choice == "2":
    #                 word = best_guess_for_win
    #                 break
    #             else:
    #                 print("Invalid choice")

if __name__ == "__main__":
    main()
