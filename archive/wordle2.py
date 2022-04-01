import json
import numpy as np
import jax
import jax.numpy as jnp
from jax import vmap, jit
from wordle import WordleWords
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

# Convert into numpy arrays of integers between 0 and 25 (for 26 letters)
def word_to_numpy(word):
    return jnp.array([ord(c) - ord('a') for c in word])

def numpy_to_word(guess_numpy):
    return ''.join([chr(c + ord('a')) for c in guess_numpy])

wordlist_array = jnp.array([word_to_numpy(word) for word in wordlist])
valid_guesses_array = jnp.array([word_to_numpy(word) for word in valid_guesses])

# Convert a word guess result into an array of positional matches, positional match masks, positionally exclusive matches, positionally exclusive match masks, and complete exclusions, and check that it's compatible with the word.
# - A positional match is a character that is in the given position. It is coloured green.
# - A positionally exclusive match is a character that is in the word but not in the given position. It is yellow.
# - A complete exclusion is a character that is not in the word. It is black.
# - A mask is a list of booleans indicating whether the match vector applies to the given position.
# A guess result is given as a string of the form 'ggyyb' where 'g' stands for green (positional matche), 'y' for yellow (positionally exclusive match), and 'b' for black (complete exclusion).
result_enum = {'g': 0, 'y': 1, 'b': 2}
result_enum_inverse = {0: 'g', 1: 'y', 2: 'b'}
def guess_result_to_numpy(guess_str):
    return jnp.array([result_enum[c] for c in guess_str])

def numpy_to_guess_result(guess_numpy):
    return ''.join([result_enum_inverse[int(guess_numpy[i])] for i in range(guess_numpy.shape[0])])

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

def make_guess_result_numpy(word, guess):
    g = word == guess
    y = ~g & (word[jnp.newaxis, :] == guess[:, jnp.newaxis]).any(axis=-1)
    b = ~jnp.logical_or(g, y)
    return 0*g + 1*y + 2*b

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

# @vmap
def word_is_compatible_with_guess_result_numpy(word_hyp, guess, guess_result):
    g_mask = guess_result == result_enum['g']
    g_pass = jnp.logical_or(word_hyp == guess, ~g_mask).all()
    y_mask = guess_result == result_enum['y']
    b_mask = guess_result == result_enum['b']
    cartesian_equalites = word_hyp[jnp.newaxis, :] == guess[:, jnp.newaxis]
    cartesian_inequalites = ~cartesian_equalites
    cartesian_inequalites_mask = jnp.diag(y_mask)
    cartesian_inequalites_mask |= b_mask[:, jnp.newaxis]
    cartesian_inequality_pass = jnp.logical_or(cartesian_inequalites, ~cartesian_inequalites_mask).all()
    cartesian_equality_mask = ~(jnp.diag(y_mask) | ~y_mask[:, jnp.newaxis])
    cartesian_equality_pass = ((cartesian_equalites & cartesian_equality_mask).any(axis=1) | ~y_mask).all()
    if TEST_MODE:
        if isinstance(word_hyp, jax.interpreters.batching.BatchTracer) and not any(isinstance(arg, jax.interpreters.batching.BatchTracer) for arg in [guess, guess_result]):
            pass
            # # Broken
            # for i in range(word_hyp.shape[0]):
            #     assert g_pass.val[i] & cartesian_inequality_pass.val[i] & cartesian_equality_pass.val[i] == word_is_compatible_with_guess_result(numpy_to_word(word_hyp.val[i]), numpy_to_word(guess), numpy_to_guess_result(guess_result)), f"{g_pass} & {cartesian_inequality_pass} & {cartesian_equality_pass} != word_is_compatible_with_guess_result({numpy_to_word(word_hyp.val[i])}, {numpy_to_word(guess)}, {numpy_to_guess_result(guess_result)}) for word_hyp.val[i] = {word_hyp.val[i]}, guess = {guess}, guess_result = {guess_result}"
        elif not any(isinstance(arg, jax.interpreters.batching.BatchTracer) for arg in [word_hyp, guess, guess_result]):
            assert g_pass & cartesian_inequality_pass & cartesian_equality_pass == word_is_compatible_with_guess_result(numpy_to_word(word_hyp), numpy_to_word(guess), numpy_to_guess_result(guess_result)), f"{g_pass} & {cartesian_inequality_pass} & {cartesian_equality_pass} != word_is_compatible_with_guess_result({numpy_to_word(word_hyp)}, {numpy_to_word(guess)}, {numpy_to_guess_result(guess_result)}) for word_hyp = {word_hyp}, guess = {guess}, guess_result = {guess_result}"

    return g_pass & cartesian_inequality_pass & cartesian_equality_pass



def get_compatible_words_numpy(guess, guess_result, wordlist_numpy):
    compatible_words = vmap(lambda word_hyp: word_is_compatible_with_guess_result_numpy(word_hyp, guess, guess_result))(wordlist_numpy)
    return compatible_words

def get_compatible_words(guess, guess_result, wordlist):
    compatible_words = [word for word in wordlist if word_is_compatible_with_guess_result(word, guess, guess_result)]
    return compatible_words

def get_num_compatible_words_numpy(word_gt, guess, wordlist_numpy):
    guess_result = make_guess_result_numpy(word_gt, guess)
    return get_compatible_words_numpy(guess, guess_result, wordlist_numpy).sum()

def get_num_compatible_words(word_gt, guess, wordlist):
    guess_result = make_guess_result(word_gt, guess)
    return len(get_compatible_words(guess, guess_result, wordlist))

def get_entropy_numpy(word_gt, guess, wordlist_numpy):
    num_compatible_words = get_num_compatible_words_numpy(word_gt, guess, wordlist_numpy)
    return jnp.log2(num_compatible_words)

def get_entropy(word_gt, guess, wordlist):
    num_compatible_words = get_num_compatible_words(word_gt, guess, wordlist)
    return np.log2(num_compatible_words)

def get_expected_entropy_numpy(guess, wordlist_numpy):
    return vmap(lambda word_gt: get_entropy_numpy(word_gt, guess, wordlist_numpy))(wordlist_numpy).mean()

def get_expected_entropy(guess, wordlist):
    return np.mean([get_entropy(word_gt, guess, wordlist) for word_gt in wordlist])

def get_information_gain_numpy(word_gt, guess, wordlist_numpy):
    base_entropy = jnp.log2(len(wordlist_numpy))
    entropy_after_guess = get_entropy_numpy(word_gt, guess, wordlist_numpy)
    return base_entropy - entropy_after_guess

def get_information_gain(word_gt, guess, wordlist):
    base_entropy = np.log2(len(wordlist))
    entropy_after_guess = get_entropy(word_gt, guess, wordlist)
    return base_entropy - entropy_after_guess

@jit
def get_expected_information_gain_numpy(guess, wordlist_numpy):
    return vmap(lambda word_gt: get_information_gain_numpy(word_gt, guess, wordlist_numpy))(wordlist_numpy).mean()

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
    # expected_information_gains = {guess_candidate: get_expected_information_gain(guess_candidate, wordlist_remaining) for guess_candidate in tqdm(valid_guesses)}
    wordlist_numpy = jnp.array([word_to_numpy(word) for word in wordlist])
    expected_information_gains = {guess_candidate: get_expected_information_gain_numpy(word_to_numpy(guess_candidate), wordlist_numpy) for guess_candidate in valid_guesses}
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
    recommended_for_information = (["soare"], 5.885202884674072)
    recommended_for_immediate_win = (["raise"], 5.878303050994873)
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
        recommended_for_quickest_expected_win = get_optimal_guess_for_immediate_win(wordlist_remaining, valid_guesses)



def test():
    import string
    wordlist_numpy = jnp.array([word_to_numpy(word) for word in wordlist])
    valid_guesses_numpy = jnp.array([word_to_numpy(word) for word in valid_guesses])
    # Suppose that "venom" is the ground truth and we have the result of a guess for "venom" (i.e. a correct guess).
    # First, make sure that "venom" is in the wordlist
    assert "venom" in wordlist
    # Check whether "mince" is compatible. Should be trivially false.
    result = word_is_compatible_with_guess_result(word_hyp='mince', guess='venom', guess_result=make_guess_result('venom', 'venom'))
    print(f"Is a ground truth of 'mince' compatible with result of guess 'venom' given that 'venom' is the ground truth (non-numpy)? {result}")
    assert not result
    result = word_is_compatible_with_guess_result_numpy(word_hyp=word_to_numpy('mince'), guess=word_to_numpy('venom'), guess_result=make_guess_result_numpy(word_to_numpy('venom'), word_to_numpy('venom')))
    print(f"Is a ground truth of 'mince' compatible with result of guess 'venom' given that 'venom' is the ground truth (numpy)? {result}")
    assert not result
    # Explain why
    result = make_guess_result('venom', 'venom')
    print(f"The guess result is {result}")
    assert result == 'ggggg'
    result = make_guess_result_numpy(word_to_numpy('venom'), word_to_numpy('venom'))
    print(f"The guess result is {result}")
    assert numpy_to_guess_result(result) == 'ggggg'
    # Check whether "venom" is compatible. Should be trivially true.
    result = word_is_compatible_with_guess_result(word_hyp='venom', guess='venom', guess_result=make_guess_result('venom', 'venom'))
    print(f"Is a ground truth of 'venom' compatible with result of guess 'venom' given that 'venom' is the ground truth (non-numpy)? {result}")
    assert result
    result = word_is_compatible_with_guess_result_numpy(word_hyp=word_to_numpy('venom'), guess=word_to_numpy('venom'), guess_result=make_guess_result_numpy(word_to_numpy('venom'), word_to_numpy('venom')))
    print(f"Is a ground truth of 'venom' compatible with result of guess 'venom' given that 'venom' is the ground truth (numpy)? {result}")
    assert result

    # Print the words compatible with result of guess "venom" given that "venom" is the ground truth
    guess_result = make_guess_result_numpy(word_to_numpy('venom'), word_to_numpy('venom'))
    compatible = [word for word, compatible in zip(wordlist, get_compatible_words_numpy(word_to_numpy('venom'), make_guess_result_numpy, wordlist_numpy)) if compatible]
    print(f"Words compatible with result of guess 'venom' given that 'venom' is the ground truth: {compatible}")
    assert compatible == ["venom"]

    # Print the expected entropy of 'soare', 'sassy', 'zzzzz'
    result = get_expected_information_gain_numpy(word_to_numpy('soare'), wordlist_numpy)
    print(f"Expected information gain of 'soare' is {result}")
    assert np.isclose(result, 5.885201930999756)
    result = get_expected_information_gain_numpy(word_to_numpy('sassy'), wordlist_numpy)
    print(f"Expected information gain of 'sassy' is {result}")
    assert np.isclose(result, 3.3399784564971924)
    result = get_expected_information_gain_numpy(word_to_numpy('zzzzz'), wordlist_numpy)
    print(f"Expected information gain of 'zzzzz' is {result}")
    assert np.isclose(result, 0.1475479155778885)

    # Print the entropy of 'zzzzz' given that 'venom' is the ground truth
    print(f"Information gain of guess 'zzzzz' given that 'venom' is the ground truth is {get_information_gain_numpy(word_to_numpy('venom'), word_to_numpy('zzzzz'), wordlist_numpy)}")

    # Fuzz test for word_is_compatible_with_guess_result and word_is_compatible_with_guess_result_numpy
    # Enumerate all possible guess results by taking the cartesian product of ["g", "y", "b"]
    print("Fuzz testing word_is_compatible_with_guess_result_numpy")
    for i in tqdm(range(1000)):
        # Generate two random 'words' of 5 characters
        word_gt = ''.join(np.random.choice([c for c in string.ascii_lowercase], size=5))
        guess = ''.join(np.random.choice([c for c in string.ascii_lowercase], size=5))
        # Generate a random guess result
        guess_result = ''.join(np.random.choice(['g', 'y', 'b'], size=5))
        # Check whether the guess result is compatible with the ground truth
        result = word_is_compatible_with_guess_result(word_gt, guess, guess_result)
        # Check whether the guess result is compatible with the ground truth
        result_numpy = word_is_compatible_with_guess_result_numpy(word_to_numpy(word_gt), word_to_numpy(guess), guess_result_to_numpy(guess_result))
        assert result == result_numpy, f"{result} != {result_numpy} for ground truth {word_gt}, guess {guess}, and guess result {guess_result}"

    # Fuzz test for get_information_gain and get_information_gain_numpy
    print("Fuzz testing get_information_gain")
    for i in tqdm(range(1000)):
        # Choose a ground truth and a guess
        word_gt = np.random.choice(wordlist)
        guess = np.random.choice(wordlist)
        result = get_information_gain(word_gt, guess, wordlist)
        result_numpy = get_information_gain_numpy(word_to_numpy(word_gt), word_to_numpy(guess), wordlist_numpy)
        assert np.isclose(result, result_numpy), f"{result} != {result_numpy} for ground truth {word_gt}, guess {guess}"

    # Find the n best starting words
    n = 10
    print(f"Finding the {n} best starting words")
    # expected_information_gains = [get_expected_information_gain_numpy(guess, wordlist_numpy) for guess in tqdm(valid_guesses_numpy)]
    # Batch the guesses
    BATCH_SIZE = 1
    valid_guesses_batched_numpy = np.array_split(valid_guesses_numpy, len(valid_guesses_numpy) // BATCH_SIZE)
    expected_information_gains = []
    get_expected_information_gain_numpy_batched = jit(vmap(get_expected_information_gain_numpy, in_axes=(0, None)))
    for guesses in tqdm(valid_guesses_batched_numpy):
        for expected_information_gain in get_expected_information_gain_numpy_batched(guesses, wordlist_numpy):
            expected_information_gains.append(expected_information_gain)
    # Get top n guesses by sorting the expected information gains
    sorted_guesses = sorted(zip(valid_guesses_numpy, expected_information_gains), key=lambda x: x[1], reverse=True)
    print(f"[word]: [expected information gain]")
    for i, (guess, expected_information_gain) in enumerate(sorted_guesses[:n]):
        print(f"{i}: {guess}: {expected_information_gain}")


    

if __name__ == '__main__':
    # test()
    main()

# f = vmap(lambda guess: vmap(lambda guess_result: get_num_compatible_words_numpy(guess, guess_result))(valid_guesses_numpy))
# f = jit(f)

# Batch the guesses in 10s
# valid_guesses_numpy_batched = jnp.array_split(valid_guesses_numpy, len(valid_guesses_numpy) // 10)
# entropies = 0
# for guess_batch in tqdm(valid_guesses_numpy_batched):
#     entropies += f(guess_batch).mean(0)


# expected_guess_entropies /= len(valid_guesses)

# 10 words with the highest entropy
# argmax_entropy = jnp.argsort(expected_guess_entropies)[::-1]
# for i in range(10):
#     print(valid_guesses[argmax_entropy[i]] + ": " + str(expected_guess_entropies[argmax_entropy[i]]))
    