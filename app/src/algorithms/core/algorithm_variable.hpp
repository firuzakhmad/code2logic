//
// Created by Akhmad on 1/2/26.
//

#ifndef CODE2LOGIC_ALGORITHM_VARIABLE_HPP
#define CODE2LOGIC_ALGORITHM_VARIABLE_HPP

#include <variant>
#include <vector>
#include <optional>
#include <string>

namespace c2l::algorithms
{
    class AlgorithmVariable
    {
    public:
        using ValueType = std::variant<
            int, size_t, float, double, bool, std::string,
            std::vector<int>, std::vector<size_t>
        >;

        AlgorithmVariable() = default;

        template<typename T>
        explicit AlgorithmVariable(
            const std::string& name,
            const T& value,
            const std::string& display_name = "")
                : m_name{name}
                , m_display_name{display_name.empty() ? name : display_name}
                , m_value{value}
                , m_type{get_type_id<T>()}
        {}

        [[nodiscard]] const std::string& get_name() const noexcept { return m_name; }
        [[nodiscard]] const std::string& get_display_name() const noexcept { return m_display_name; }
        [[nodiscard]] std::string get_type_string() const;

        template<typename T>
        [[nodiscard]] bool holds() const { return std::holds_alternative<T>(m_value); }

        template<typename T>
        [[nodiscard]] std::optional<T> get() const
        {
            if constexpr (std::is_same_v<T, std::string>)
            {
                if (std::holds_alternative<int>(m_value))
                    return std::to_string(std::get<int>(m_value));
                if (std::holds_alternative<size_t>(m_value))
                    return std::to_string(std::get<size_t>(m_value));
                if (std::holds_alternative<float>(m_value))
                    return std::to_string(std::get<float>(m_value));
                if (std::holds_alternative<double>(m_value))
                    return std::to_string(std::get<double>(m_value));
                if (std::holds_alternative<bool>(m_value))
                    return std::get<bool>(m_value) ? "true" : "false";
                if (std::holds_alternative<std::string>(m_value))
                    return std::get<std::string>(m_value);
                return std::nullopt;
            }
            else
            {
                if (std::holds_alternative<T>(m_value))
                    return std::get<T>(m_value);
                return std::nullopt;
            }
        }

        [[nodiscard]] std::string to_string() const;

    private:
        std::string m_name;
        std::string m_display_name;
        ValueType m_value;
        size_t m_type{0};

        template<typename T>
        static size_t get_type_id()
        {
            static size_t id = ++s_type_counter;
            return id;
        }

        static inline size_t s_type_counter = 0;
    };

} // namespace c2l::algorithms

#endif //CODE2LOGIC_ALGORITHM_VARIABLE_HPP