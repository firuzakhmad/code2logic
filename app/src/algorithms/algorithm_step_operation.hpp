//
// Created by Akhmad on 1/3/26.
//

#ifndef CODE2LOGIC_ALGORITHM_STEP_OPERATION_HPP
#define CODE2LOGIC_ALGORITHM_STEP_OPERATION_HPP

#include <cstdint>

namespace c2l::algorithms
{
    enum class AlgorithmStepOperation : uint8_t
    {
        NONE,

        INIT,
        LOOP_OUTER,
        LOOP_INNER,

        COMPARE,
        SWAP,

        PARTITION,
        MERGE,
        PIVOT_PLACED,
        RECURSIVE_CALL,

        PASS_COMPLETE,
        FINISHED
    };
}
#endif //CODE2LOGIC_ALGORITHM_STEP_OPERATION_HPP