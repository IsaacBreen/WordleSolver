#ifndef UTILS_HPP
#define UTILS_HPP

template<class T, size_t N>
constexpr size_t size(T (&)[N]) { return N; }

template<size_t N>
constexpr size_t length(char const (&)[N]) { return N-1; }

template<typename Base>
constexpr Base mypow(Base base, int exponent) {
    return (exponent == 0) ? 1 : base * mypow(base, exponent - 1);
}

template <typename F, typename T>
double evaluate_expectation(F f, T& iterable, bool verbose=false, bool parallel=false, bool vectorise=false)
{
    // Can't have parallel verbose
    if (vectorise and parallel) {
        vector<int> iterable_vector = iterable;
        double expectation = 0;
        #pragma omp parallel for reduction(+:expectation) schedule(dynamic)
        for (int i = 0; i < iterable_vector.size(); i++) {
            expectation += f(iterable_vector[i]);
        }
        return expectation / iterable.size();
    } else {
        double expectation = 0;
        int i = 0;
        // #pragma omp parallel for reduction(+:expectation) if(parallel and iterable.size() > 1000) schedule(dynamic)
        for (auto item : iterable) {
            if (verbose) cout << "Evaluating " << item << " (" << i << " / " << iterable.size() << ")" << endl;
            expectation += f(item);
            if (verbose) cout << "\033[F\33[2K\r";
            if (verbose) i++;
            
        }
        return expectation / iterable.size();
    }
}

string indentations(int n)
{
    string result = "";
    for (int i = 0; i < n; i++)
    {
        result += "    ";
    }
    return result;
}

template<typename T>
T sample(T& iterable, int n)
{
    // If n is the size of iterable, then we can just return iterable
    if (n == iterable.size()) {
        return iterable;
    }
    // Sample n elements from the iterable by setting elements to false
    set<int> sampled_indices;
    while (sampled_indices.size() < n) {
        auto i = rand() % iterable.size();
        if (iterable[i]) {
            sampled_indices.insert(i);
        }
    }
    vector<int> sampled_indices_vector(sampled_indices.begin(), sampled_indices.end());
    // Set the sampled elements to false
    T result(sampled_indices_vector);
    return result;
}

#endif // UTILS_HPP