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

        PARTITION_START,
        PIVOT_SELECTED,
        PARTITION_SCAN,
        PARTITION_SWAP,
        PARTITION_COMPLETE,
        RECURSIVE_CALL,
        RECURSIVE_CALL_RIGHT,


        MERGE,

        PASS_COMPLETE,
        COMPLETED
    };
}
#endif //CODE2LOGIC_ALGORITHM_STEP_OPERATION_HPP