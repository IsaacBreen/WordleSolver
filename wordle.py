import numpy as np

# Import words from solutions_nyt.txt. They are quoted (e.g. "soare") and separated by ", ".
with open("solutions_nyt.txt", "r") as f:
    words = f.read().split(", ")

words