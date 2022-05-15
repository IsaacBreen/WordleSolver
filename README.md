To my knowledge, this is the first correct Wordle solver to calculate an optimal guess, defined as the guess that minimizes the expected number of turns to win. No tricks - this algorithm is not a heuristic. Most other solution attempts define an 'optimal guess' as that which gives the highest expected information-gain to make the search tractable (although it does use maximum information-gain as a heuristic to make the search faster). While this is a useful heuristic, it is not equivalent to minimizing the expected number of turns to win. Given that the goal of Wordle is generally considered to find the solution in the fewest turns, the information-gain heuristic is not the optimal strategy.

The core algorithm is simple. In Pythonic pseudocode:

```python
guesslist = ['aahed', 'aalii', 'aargh', 'aarti', 'abaca', ...]
wordlist_initial  = ['aback', 'abase', 'abate', 'abbey', 'abbot' ...]

def find_optimal_guess(wordlist):
    # Keep track of the best guess and its expected number of turns to win
    best_guess = None
    best_ETW = math.inf # Initializing to infinity ensures it's immediately replaced by the first guess
    for guess in guesslist:
        ETW = 0
        # Calculate the expectation over all words in the wordlist
        for hypothesised_word in wordlist:
            if guess == hypothesised_word:
                ETC += 0 # This guess is correct; the number of turns left to win is 0
                continue
            # Calculate the hint: a list of colours (e.g. ['green', 'yellow', 'gray, 'gray', 'yellow']
            hint = make_hint(guess, hypothesised_word)
            remaining_wordlist = [word for word in wordlist if are_compatible(word, guess, hint)] # Eliminate words that aren't compatible with the guess/hint
            ETW += expected_turns_to_win(remaining_wordlist) # Update the ETW
        if ETW < best_ETW:
            best_guess = guess
            best_ETW = ETW
    return best_guess, best_ETW
```
    
This is only to give an idea of the algorithm - the function names above aren't consistent with the actual code. The actual implementation is more complex. It is a depth-first search that employs early-stopping rules, precomputes `make_hint(guess, word)` and 'are_compatible(word, guess, hint)' over all inputs, and uses integer arithmetic and bitsets as instead of strings wherever possible to speed up the search.

Despite this, the algorithm is still very slow. I have estimated that it would take about 54 years to find the optimal first word, or 6 months on 100-cores. Getting up to a reasonable speed on a typical single CPU core is the next step.

It is perfectly useable for computing optimal second guesses, however - although it still needs a user-friendly interface.

## Setup

Prerequisites:
- Boost ([Windows](https://www.boost.org/doc/libs/1_79_0/more/getting_started/unix-variants.html) or [Linux & MacOS](https://www.boost.org/doc/libs/1_79_0/more/getting_started/unix-variants.html)).
- OpenMP
- Python 3.6+

Make sure your C++ compiler can see Boost and OpenMP. I prefer to add their directories to CPP_INCLUDE_PATH and CPP_LIBRARY_PATH.

Download the repository:
```bash
git clone https://github.com/IsaacBreen/WordleSolver
cd WordleSolver
```
Optionally, preprocess the word- and guess-lists. This repo ships with the preprocessed data, so you can skip this step if you just want to run the solver.

```bash
python preprocess_data.py
```
Finally, to build and run using GCC, execute:
```bash
cd cpp/src
g++ -std=c++20 -O3 -lomp -fopenmp -lboost_serialization -lboost_iostreams -fpermissive main.cpp -o solver
./solver
```

If using Clang\'s `g++`-like interface, replace -fopenmp with -Xpreprocessor -fopenmp.

## Acknowledgements

- (LaurentLessard/wordlesolver)[https://github.com/LaurentLessard/wordlesolver] - Provides word- and guess-lists