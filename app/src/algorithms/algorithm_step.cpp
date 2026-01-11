//
// Created by Akhmad on 1/2/26.
//

#include "algorithms/algorithm_step.hpp"

namespace c2l::algorithms
{
    std::string AlgorithmStep::MetaData::to_debug_string() const
    {
        std::string result;
        result += "Operation: " + operation_type + "\n";
        result += "Variables:\n";

        for (const auto& [key, var] : variables) {
            result += "  " + key + " (" + var.get_display_name() + "): "
                     + var.to_string() + " [" + var.get_type_string() + "]\n";
        }

        if (!notes.empty()) {
            result += "Notes:\n";
            for (const auto& note : notes) {
                result += "  • " + note + "\n";
            }
        }

        if (!tags.empty()) {
            result += "Tags: ";
            for (const auto& [key, tag] : tags) {
                result += tag + " ";
            }
            result += "\n";
        }

        return result;
    }
} // namespace c2l::algorithms