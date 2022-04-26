#ifndef UTILS_HPP
#define UTILS_HPP

template<class T, size_t N>
constexpr size_t size(T (&)[N]) { return N; }

template<size_t N>
constexpr size_t length(char const (&)[N]) { return N-1; }

#endif // UTILS_HPP