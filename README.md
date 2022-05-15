To my knowledge, this is the first correct Wordle solver to calculate an optimal guess, defined as the guess that minimizes the expected number of turns to win. No tricks - this algorithm is not a heuristic. Nor does it rely on any questionable definition of an 'optimal guess', such as that which gives the highest expected information-gain to make the search tractable (although it does use maximum information-gain as a heuristic to make the search faster). The core algorithm is simple (Pythonic pseudocode):
```python
guesslist = ['aahed', 'aalii', 'aargh', 'aarti', 'abaca', ...]
wordlist_initial  = ['aback', 'abase', 'abate', 'abbey', 'abbot' ...]

def find_optimal_guess(wordlist):
    best_guess = None
    best_ETW = math.inf # Expected number of turns to win after the best guess. Initialized to infinity ensures it's replaced by the first guess.
    for guess in guesslist:
        ETW = 0
        for hypothesised_word in wordlist: # Calculate the expectation over all words in the wordlist
            if guess == hypothesised_word:
                ETC += 0 # This guess is correct; the number of turns left to win is 0
                continue
            hint = make_hint(guess, hypothesised_word) # Calculate the hint: a list of colours (e.g. ['green', 'yellow', 'gray, 'gray', 'yellow']
            remaining_wordlist = [word for word in wordlist if are_compatible(word, guess, hint)] # Eliminate words that aren't compatible with the guess/hint
            ETW += expected_turns_to_win(remaining_wordlist) # Update the ETW
        if ETW < best_ETW:
            best_guess = guess
            best_ETW = ETW
    return best_guess, best_ETW
```
    


## Dependencies

- (LaurentLessard/wordlesolver)[https://github.com/LaurentLessard/wordlesolver] - Provides word- and guess-lists