#include "misc_utils.hpp"

using namespace vren;

std::vector<char const*> to_c_str_vector(std::vector<std::string> const& std_string_vector)
{
    std::vector<char const*> result;
    std::transform(
        std_string_vector.begin(), std_string_vector.end(), std::back_inserter(result),
        [](std::string const& str) { return str.c_str(); }
    );
    return result;
}
