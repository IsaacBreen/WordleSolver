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
double evaluate_expectation(F f, T& iterable)
{
    double expectation = 0;
    for (auto item : iterable)
    {
        expectation += f(item);
    }
    return expectation / iterable.size();
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

#endif // UTILS_HPP