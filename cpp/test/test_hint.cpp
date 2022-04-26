#include <hint.hpp>

constexpr void test_hints() {
    static_assert(make_hint("aaaaa","bbbbb") == 0);
    static_assert(make_hint("aabbc", "dbbba") == string_to_hint("bbggy"));
}
