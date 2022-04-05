# Makefile for wordle solver
# Equivalent to running /opt/homebrew/opt/gcc/bin/g++-11 -std=c++20 -fopenmp -lomp -O2 wordle.cpp -o wordle.out && ./wordle.out [args]

# Compile
g++ -std=c++20 -fopenmp -lomp -O2 wordle.cpp -o wordle.out

# Run
./wordle.out $@

