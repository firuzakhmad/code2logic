//
// Created by Akhmad on 1/2/26.
//

#include "algorithms/core/algorithm_variable.hpp"

#include <unordered_map>
#include <string>
#include <array>

namespace c2l::algorithms
{
    std::string AlgorithmVariable::get_type_string() const
    {
        static constexpr std::array<const char*, 8> names = {
            "int", "size_t", "float", "double",
            "bool", "string", "vector<int>", "vector<size_t>"
        };
        return names[m_value.index()];
    }

    std::string AlgorithmVariable::to_string() const
    {
        return std::visit([](auto&& arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, int> ||
                          std::is_same_v<T, size_t> ||
                          std::is_same_v<T, float> ||
                          std::is_same_v<T, double>) {
                return std::to_string(arg);
            }
            else if constexpr (std::is_same_v<T, bool>) {
                return arg ? "true" : "false";
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                return arg;
            }
            else if constexpr (std::is_same_v<T, std::vector<int>>) {
                std::string result = "[";
                for (size_t i = 0; i < arg.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += std::to_string(arg[i]);
                }
                result += "]";
                return result;
            }
            else if constexpr (std::is_same_v<T, std::vector<size_t>>) {
                std::string result = "[";
                for (size_t i = 0; i < arg.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += std::to_string(arg[i]);
                }
                result += "]";
                return result;
            }
            else {
                return "unprintable";
            }
        }, m_value);
    }

} // namespace c2l::algorithms