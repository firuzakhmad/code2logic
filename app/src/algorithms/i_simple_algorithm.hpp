//
// Created by Akhmad on 11/7/25.
//

#ifndef CODE2LOGIC_I_SIMPLE_ALGORITHM_HPP
#define CODE2LOGIC_I_SIMPLE_ALGORITHM_HPP

#include <vector>
#include <string>

#include "algorithm_observer.hpp"
#include "algorithms/algorithm_types.hpp"
#include "algorithms/algorithm_step.hpp"

namespace c2l::algorithms
{
    class ISimpleAlgorithm
    {
    public:
        virtual ~ISimpleAlgorithm() = default;

        virtual void initialize(const std::vector<int>& data)               = 0;
        virtual bool step_forward()                                         = 0;
        virtual bool step_backward()                                        = 0;
        virtual void reset()                                                = 0;

        virtual void add_observer(AlgorithmObserver* observer)              = 0;
        virtual void remove_observer(AlgorithmObserver* observer)           = 0;

        virtual void populate_step_metadata(AlgorithmStep& out_step) const  = 0;

        [[nodiscard]] virtual std::vector<int> get_original_data() const    = 0;
        [[nodiscard]] virtual AlgorithmStep get_current_step() const        = 0;
        [[nodiscard]] virtual size_t get_step_count() const                 = 0;
        [[nodiscard]] virtual size_t get_current_step_index() const         = 0;
        [[nodiscard]] virtual bool is_complete() const                      = 0;
        [[nodiscard]] virtual bool is_steps_empty_or_invalid() const        = 0;

        // Algorithm metadata
        [[nodiscard]] virtual AlgorithmType get_type() const                = 0;
        [[nodiscard]] virtual std::string get_name() const                  = 0;
        [[nodiscard]] virtual AlgorithmCategory get_category() const        = 0;
        [[nodiscard]] virtual std::string get_time_complexity() const       = 0;
        [[nodiscard]] virtual std::string get_space_complexity() const      = 0;
        [[nodiscard]] virtual std::string get_description() const           = 0;
        [[nodiscard]] virtual std::vector<std::string> get_detailed_description() const             = 0;
        [[nodiscard]] virtual std::unordered_map<std::string, std::string> get_properties() const   = 0;


        // Algorithm capabilities
        [[nodiscard]] virtual bool supports_backward_steps() const      { return true; }
        [[nodiscard]] virtual bool supports_random_access() const       { return false; }
        [[nodiscard]] virtual bool requires_specialized_data() const    { return false; }

        [[nodiscard]] virtual std::vector<std::string> get_variable_names() const
        {
            return {};  // Overriding in derived classes
        }
    };
} // namespace c2l::algorithms

#endif //CODE2LOGIC_I_SIMPLE_ALGORITHM_HPP