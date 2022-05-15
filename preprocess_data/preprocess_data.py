# TODO: needs a cleanup

import numpy as np
import pathlib
import shutil

# Save to cpp/src/data/wordlist.hpp and cpp/src/data/guesslist.hpp
RAW_DATA_FOLDER = pathlib.Path(__file__).parent.parent / "data"
CPP_DATA_FOLDER = pathlib.Path(__file__).parent.parent / "cpp" / "src" / "data"
USE_CONSTEXPR = False

nw = None
ng = None

# Import words from solutions_nyt.txt and nonsolutions_nyt.txt. They are quoted (e.g. "soare") and separated by ", ".
with open(RAW_DATA_FOLDER / "solutions_nyt.txt", "r") as f:
    # Remove all spaces and quotes
    words = f.read().replace(" ", "").replace('"', "").split(",")[:nw]
    words = words
with open(RAW_DATA_FOLDER / "nonsolutions_nyt.txt", "r") as f:
    guesses = f.read().replace(" ", "").replace('"', "").split(',')[:ng]
    guesses = guesses
guesses = words + guesses
# Ensure each guess is unique
assert len(set(guesses)) == len(guesses)
print(f"There are {len(words)} words and {len(guesses)} guesses.")
# Print the first 10 words and guesses with a ... to indicate that there are more
print(f"wordlist  = [{', '.join(sorted(words)[:10])} ...]")
print(f"guesslist = [{', '.join(sorted(guesses)[:10])} ...]")

# Template for a c++ file that contains the data as a constexpr
constexpr_template = """\
#include <array>

constexpr std::array {name} = {{{words}}};
"""

def save_constexpr_cpp(path, name, words, words_per_line=10):
    with open(path, "w") as f:
        words_str = []
        for i in range(0, len(words), words_per_line):
            words_str.append(", ".join(f'"{word}"' for word in words[i:i+words_per_line]))
        words_str = "\n    " + ",\n    ".join(words_str)
        f.write(constexpr_template.format(name=name, words=words_str, DATA_FOLDER=CPP_DATA_FOLDER))
                
save_constexpr_cpp(CPP_DATA_FOLDER / "wordlist.hpp", "words", words)
save_constexpr_cpp(CPP_DATA_FOLDER / "guesslist.hpp", "guesses", guesses)
# Save as regular csv files to the same folder
np.savetxt(CPP_DATA_FOLDER / "wordlist.csv", words, delimiter=",", fmt="%s")
np.savetxt(CPP_DATA_FOLDER / "guesslist.csv", guesses, delimiter=",", fmt="%s")

# Save some constants to generated_constants.hpp
constants_template = """\
constexpr int WORD_LENGTH = {word_length};
const int NUM_WORDS = {num_words};
const int NUM_GUESSES = {num_guesses};
"""

with open(CPP_DATA_FOLDER / "generated_constants.hpp", "w") as f:
    f.write(constants_template.format(word_length=len(words[0]), num_words=len(words), num_guesses=len(guesses)))
    
# Clear and remake the cache folder
shutil.rmtree(CPP_DATA_FOLDER / "cache", ignore_errors=True)
pathlib.Path(CPP_DATA_FOLDER / "cache").mkdir(parents=True, exist_ok=True)