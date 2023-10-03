#pragma once

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

namespace vren
{
    std::vector<char const*> to_c_str_vector(std::vector<std::string> const& std_string_vector);

    template<typename T, size_t N>
    std::vector<T>& insert_c_array(std::vector<T>& vector, T const(&c_array)[N])
    {
        vector.insert(vector.end(), c_array, c_array + N);
        return vector;
    }

    template<typename T>
    std::vector<T>& concat_vector(std::vector<T>& subject, std::vector<T> const& tail)
    {
        if (!tail.empty())
            subject.insert(subject.end(), tail.begin(), tail.end());
        return subject;
    }

} // namespace vren
