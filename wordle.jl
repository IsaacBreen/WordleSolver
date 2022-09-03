asserting() = false # when set to true, this will enable all `@mayassert`s

macro mayassert(test...)
  esc(:(if $(@__MODULE__).asserting()
    @assert($test)
   end))
end

f(x) = @mayassert x < 2 


using Pkg
# Pkg.add("ProgressBars")
# Pkg.add("LoopVectorization")
# Pkg.add("ResumableFunctions")
# Pkg.add("OMEinsum")
# Pkg.add("BenchmarkTools")
# Pkg.add("EndpointRanges")
# Pkg.add("Shuffle")
# Pkg.add("Plots")
# Pkg.add("FlameGraphs")
# Pkg.add("StaticArrays")
using ProgressBars
using LoopVectorization
using ResumableFunctions
using OMEinsum
using BenchmarkTools
using EndpointRanges
using Shuffle
using Random
using StaticArrays

# Use Julia's Base.filesystem to navigate two parents up from "/Users/isaacbreen/Documents/Projects/WordleSolver/cpp/src"
# to get to the root of the project.

path = "/Users/isaacbreen/Documents/Projects/WordleSolver/cpp/src"

Random.seed!(15447)

RAW_DATA_FOLDER = path*"/../../data"
CPP_DATA_FOLDER = path*"/../../cpp/src/data"
const USE_CONST = false

const ng = 5
const nw = 10

# Import words from solutions_nyt.txt and nonsolutions_nyt.txt. They are quoted (e.g. "soare") and separated by ", ".
words = open(RAW_DATA_FOLDER * "/solutions_nyt.txt") do io read(io, String) end
words = replace(words, "\"" => "")
words = replace(words, " " => "")
words = split(words, ",")
# words = sort(words)
Shuffle.shuffle!(words)
words = words[begin:nw]
append!(words, ["crane", "soare"])

guesses = open(RAW_DATA_FOLDER * "/nonsolutions_nyt.txt") do io read(io, String) end
guesses = replace(guesses, "\"" => "")
guesses = replace(guesses, " " => "")
guesses = split(guesses, ",")
# guesses = sort(guesses)
Shuffle.shuffle!(guesses)
guesses = guesses[begin:ng]

guesses = vcat(words, guesses)
guesses = words

# Ensure each guess is unique
@mayassert length(unique(guesses)) == length(guesses)
println("There are $(length(words)) words and $(length(guesses)) guesses.")
# Print the first 10 words and guesses with a ... to indicate that there are more
println("wordlist  = [$(join(words[1:10], ", ")) ...]")
println("guesslist = [$(join(guesses[1:10], ", ")) ...]")
# Convert to int8 arrays
lowercase_letters = "abcdefghijklmnopqrstuvwxyz"
lowercase_letters_map = Dict(zip(lowercase_letters, range(1, length(lowercase_letters) + 1)))
function str_to_int8(s)
    return hcat([convert(UInt8, lowercase_letters_map[s[i]]) for i in 1:length(s)])
end
function int8_to_str(a)
    return join([lowercase_letters[i] for i in a], "")
end
words = vcat([str_to_int8(w) for w in words])
guesses = vcat([str_to_int8(g) for g in guesses])
words_map = Dict(zip(words, 1:length(words)))
guesses_map = Dict(zip(guesses, 1:length(guesses)))

if USE_CONST
    const WORD_LENGTH = 5
    const NUM_HINTS = 3^WORD_LENGTH
    const NUM_WORDS = length(words)
    const NUM_GUESSES = length(guesses)
    const MAX_TURNS = 6
    const PRINT_TURNS = 0
else
    WORD_LENGTH = 5
    NUM_HINTS = 3^WORD_LENGTH
    NUM_WORDS = length(words)
    NUM_GUESSES = length(guesses)
    MAX_TURNS = 3
    PRINT_TURNS = 0
end

const FULL_WORDLIST = BitVector(ones(Bool, NUM_WORDS))

function make_hint(word, guess)
    hint = 0
    for i in 1:WORD_LENGTH::Int
        if guess[i] == word[i]
            hint += 3^(i-1)*2
        end
    end
    for i in 1:WORD_LENGTH::Int
        for j in 1:WORD_LENGTH::Int
            if guess[i] == word[j] && guess[i] != word[i] && guess[j] != word[j]
                hint += 3^(i-1)
                break
            end
        end
    end
    return hint
end

function hint_to_str(hint)
    s = ""
    for i in 1:WORD_LENGTH
        s += str(hint % 3)
        hint //= 3
    end
    return s
end

const hints = make_hint.(words, reshape(guesses, (1,:))) # (NUM_WORDS, NUM_GUESSES)
const compatibilities = ein"wg->gw"(hints)[[CartesianIndex()],:,:] .== 0:(NUM_HINTS-1) # (NUM_HINTS, NUM_GUESSES, NUM_WORDS)

function get_compatible_words(guess, hint, compatibilities)
    return compatibilities[hint, guess, :]
end

function get_num_compatible_words_remaining(guess, word, wordlist, hints, compatibilities)
    return sum(wordlist .& compatibilities[hints[word, guess].+1, guess, :])
end

function remaining_after_guess_sequence(word_str, guess_sequence...)
    wordlist = FULL_WORDLIST
    remaining = []
    word = words_map[str_to_int8(word_str)]
    for guess_str in guess_sequence
        # Get index of guess
        guess = guesses_map[str_to_int8(guess_str)]
        wordlist = wordlist .& get_compatible_words(guess, hints[word, guess]+1, compatibilities)
        append!(remaining, findall(wordlist))
    end
    return remaining
end

##

function bincounts(X, len)
    count = zeros(len)
    for x in 1:len
        count[x] += 1
    end
    return count
end

function bincounts(X, values)
    return sum(X .== reshape(values, 1,:), dims=1)[1,:]
end

function get_hint_counts(guess::Int, wordlist::BitVector, hints::Matrix{Int64})
    return bincounts((hints[:, guess] .+ 1) .* wordlist, 1:NUM_HINTS)
end

function get_next_wordlists_and_counts(guess::Int, wordlist::BitVector, hints::Matrix{Int64}, compatibilities::BitArray{3}; remove_nonzero=true)
    hint_counts = get_hint_counts(guess, wordlist, hints)
    wordlists = reshape(wordlist,1,:) .& compatibilities[remove_nonzero ? hint_counts .> 0 : :, guess, :]
    return wordlists, hint_counts[remove_nonzero ? hint_counts .> 0 : :]
end

function get_expected_num_words_remaining(guess, wordlist, hints, compatibilities)
    wordlists, counts = get_next_wordlists_and_counts(guess, wordlist, hints, compatibilities)
    probabilities = counts / sum(counts)
    return sum(probabilities .* wordlists)
end

##
mutable struct IncrementalExpectation
    iterators
    probabilities::Vector{Float64}
end

IncrementalExpectation(iterators, probabilities) = IncrementalExpectation(Iterators.Stateful(Iterators.cycle(iterators)), Iterators.Stateful(Iterators.cycle(probabilities)))

Base.iterate(state::IncrementalExpectation) = begin
    while true
        it_maybe = iterate(iterators)
        if isnothing(it)
            return
        end
        it, _ = it_maybe
        result = iterate(it)
        if isnothing(result)
            continue
        end
        x, _ = result
        p, _ = iterate(probabilities)
        return x*p, nothing
    end
    return
end

Base.iterate(iter::IncrementalExpectation, state) = begin
    # @show state
    return iterate(iter)
end

function calculate_expectation_breadth_first_yield(f, values, probabilities, args...)
    # Calculates the expectation of f(values) given the probabilities.
    iterators = (f(x, args...) for x in values)
    state = IncrementalExpectation(iterators, probabilities)
    return state
end

mutable struct IncrementalMin
    iterators
    results::Vector{Float64}
    upper_bound::Float64
    prev_yielded::Float64
end

IncrementalMin(iterators, upper_bound=Inf) =
    IncrementalMin(
        Iterators.Stateful(Iterators.cycle(enumerate(iterators))),
        zeros(Float64, length(iterators)),
        upper_bound,
        0.0)

Base.iterate(state::IncrementalMin) = begin
    if ~any(state.iterators_active)
        return
    end
    while true
        it_maybe = iterate(iterators)
        if isnothing(it)
            return
        end
        (it, i), _ = it_maybe
        result = iterate(it)
        if isnothing(result)
            state.upper_bound = min(state.upper_bound, state.results[i])
            continue
        end
        x, _ = result
        state.results[i] += x
    end
    # for i in 1:length(state.iterators)
    #     if state.iterators_active[i]
    #         it = state.iterators[i]
    #         next_val = iterate(it)
    #         if ~isnothing(next_val)
    #             state.results[i] += next_val[1]
    #         else
    #             # Remove exhausted iterator
    #             state.iterators_active[i] = false
    #         end
    #     end
    # end
    state.iterators_active = state.iterators_active .& (state.results .< state.upper_bound)
    min_result_idx = argmin(state.results)
    smallest_result = state.results[min_result_idx]
    # to_return = (smallest_result - state.prev_yielded, min_result_idx)
    to_return = smallest_result - state.prev_yielded
    state.prev_yielded = smallest_result
    return to_return, nothing
end

Base.iterate(iter::IncrementalMin, state) = iterate(iter)

function min_breadth_first_yield(f, values, args; upper_bound::Float64=Inf, return_arg::Bool=false)
    # Calculates the expectation of f(values) given the probabilities.
    iterators = (f(x, args...) for x in values)
    state = IncrementalMin(iterators, upper_bound)
    # if !return_arg
    #     state = Iterators.map(val -> val[1], state)
    # end
    return state
end

function get_max_ig_guess(wordlist::BitVector) :: Integer
    function helper(guess:: Integer) :: Float64
        wordlists_remaining = reshape(wordlist,1,:) .& compatibilities[hints[wordlist, guess].+1, guess, :]
        return mean(sum(wordlists_remaining, dims=2))
    end
    return argmin(helper(guess) for guess in 1:NUM_GUESSES)
end

function approx_EMTW(wordlist::BitVector) :: Float64
    function helper(word::Integer)
        guess = get_max_ig_guess(wordlist)
        wordlist_remaining = wordlist .& compatibilities[hints[word, guess].+1, guess, :]
        num_words_remaining = sum(wordlist_remaining)
        if guess == word
            return 0
        elseif num_words_remaining == 1
            return 1
        elseif num_words_remaining == 2
            return 1.5
        else
            return 1 + approx_EMTW(wordlist_remaining)
        end
    end
    return mean(helper(word) for word in findall(wordlist))
end

@show int8_to_str(guesses[get_max_ig_guess(FULL_WORDLIST)])
@show approx_EMTW(FULL_WORDLIST)

# mutable struct _calculate_EMTW_given_guess_struct
#     iterator
# end

# Base.iterate(iter::_calculate_EMTW_given_guess_struct) = iterate(iter.iterator)
# Base.iterate(iter::_calculate_EMTW_given_guess_struct, state) = iterate(iter.iterator, state)

function _calculate_EMTW_given_guess(guess::Integer, wordlist::BitVector, num_words::Int, turn::Int)
    next_wordlists, counts = get_next_wordlists_and_counts(guess, wordlist, hints, compatibilities, remove_nonzero=true)
    if num_words > 0
        probabilities = counts / num_words
        @mayassert all(sum(next_wordlists, dims=2) .> 0)
        iter = calculate_expectation_breadth_first_yield(_calculate_EMTW, (convert(BitVector, wl) for wl in eachrow(next_wordlists)), probabilities, turn)
        return iter
        # state = _calculate_EMTW_given_guess_struct(iter)
        # return state
    end
end

# mutable struct _calculate_EMTW_struct
#     iterator
# end

# Base.iterate(iter::_calculate_EMTW_struct) = iterate(iter.iterator)
# Base.iterate(iter::_calculate_EMTW_struct, state) = iterate(iter.iterator, state)

function _calculate_EMTW(wordlist::BitVector, turn::Int)
    """
    Calculates the expected minimum time to win for the given wordlist and the optimal strategy.
    """
    num_words = sum(wordlist)
    @mayassert size(wordlist) == (NUM_WORDS,) "calculate_EMTW: $(size(wordlist)) != ($NUM_WORDS,)"
    if turn <= PRINT_TURNS
        println("Turn $turn with $num_words words remaining")
    end
    if turn >= MAX_TURNS
        return Iterators.Stateful([Inf])
    elseif num_words == 0
        throw("No words left")
    elseif num_words == 1
        return Iterators.Stateful([1.0])
    elseif num_words == 2
        return Iterators.Stateful([1.5])
    else
        iter = min_breadth_first_yield(_calculate_EMTW_given_guess, 1:NUM_GUESSES, (wordlist, num_words, turn+1))
        return Iterators.flatten((Iterators.Stateful([1.0]), iter))
    end
end

function _calculate_best_guess(wordlist::BitVector)
    """
    Calculates the best guess for the given wordlist.
    """
    return min_breadth_first_yield(_calculate_EMTW, 1:NUM_GUESSES, (wordlist, 0, 0), return_arg=true)
end

function calculate_EMTW()
    return _calculate_EMTW(FULL_WORDLIST, 0)
end


function run_calculate_EMTW()
    EMTW = 0
    best_guess = 1
    for x in calculate_EMTW()
        EMTW += length(x)==1 ? x : x[1]
        best_guess = length(x)==1 ? best_guess : x[2]
        @show x, EMTW, int8_to_str(guesses[best_guess])
    end
    @show EMTW
end

@time run_calculate_EMTW()
@time run_calculate_EMTW()

# using Plots, Profile, FlameGraphs

# Profile.clear(); @profile profile_test(10)    # collect profiling data
# @profile run_calculate_EMTW()
# g = flamegraph(C=true)
# img = flamepixels(g);
# img.save("flamegraph.png")