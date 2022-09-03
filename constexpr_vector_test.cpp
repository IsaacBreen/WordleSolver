#include <string>
#include <vector>

constexpr auto check()
{
    std::vector<std::string> ss;
    ss.emplace_back(std::string{"Hello"});
    ss.emplace_back(std::string{"World"});

    return ss[0] != ss[1];
}

int main()
{
    static_assert(check());
}
