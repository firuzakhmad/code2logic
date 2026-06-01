#ifndef CODE2LOGIC_HEURISTIC_TYPE_HPP
#define CODE2LOGIC_HEURISTIC_TYPE_HPP

#include <string>

namespace c2l::algorithms
{
    enum class HeuristicType
    {
        Euclidean,
        Manhattan,
        Chebyshev,
        Octile
    };

    inline std::string heuristic_type_to_string(HeuristicType type)
    {
        switch (type)
        {
            case HeuristicType::Euclidean: return "euclidean";
            case HeuristicType::Manhattan: return "manhattan";
            case HeuristicType::Chebyshev: return "chebyshev";
            case HeuristicType::Octile: return "octile";
            default: return "euclidean";
        }
    }

    inline HeuristicType string_to_heuristic_type(const std::string& str)
    {
        if (str == "manhattan") return HeuristicType::Manhattan;
        if (str == "chebyshev") return HeuristicType::Chebyshev;
        if (str == "octile") return HeuristicType::Octile;
        return HeuristicType::Euclidean;
    }
}

#endif // CODE2LOGIC_HEURISTIC_TYPE_HPP