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


        PASS_START,

        // Merge Sort
        MERGE_BASE_CASE_HIT,
        MERGE_CHECK_BASE_CASE,
        MERGE_DIVIDE,
        MERGE_RECURSE_LEFT,
        MERGE_RECURSE_RIGHT,
        MERGE_RECURSION_RETURN,
        MERGE_START,
        MERGE_SETUP,
        MERGE_COMPARE,
        MERGE_TAKE_LEFT,
        MERGE_TAKE_RIGHT,
        MERGE_COPY_RIGHT_REMAINING,
        MERGE_COPY_LEFT_REMAINING,
        MERGE_WRITE_BACK,
        MERGE_COMPLETE,


        PASS_COMPLETE,
        COMPLETED
    };
}
#endif //CODE2LOGIC_ALGORITHM_STEP_OPERATION_HPP