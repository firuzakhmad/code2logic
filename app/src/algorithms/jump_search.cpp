#include "jump_search.hpp"
#include "core/utils/logger/logger.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace c2l::algorithms
{
    JumpSearch::JumpSearch(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase(json_config_manager, AlgorithmType::JUMP_SEARCH)
    {
        LOG_DEBUG("JumpSearch created and metadata loaded from JSON");
    }

    void JumpSearch::initialize(const std::vector<int>& data)
    {
        m_original_data.clear();
        reset();

        if (data.empty())
        {
            LOG_WARNING("JumpSearch initialized with empty data");
            return;
        }

        m_original_data = data;

        // Verify array is sorted (jump search requirement)
        if (!std::is_sorted(data.begin(), data.end()))
        {
            LOG_WARNING(
                "JumpSearch requires sorted input array. Array is being sorted."
            );

            std::sort(m_original_data.begin(), m_original_data.end());
        }

        generate_all_steps();

        LOG_INFO(
            "JumpSearch initialized with {} elements. Block size is {}",
            data.size(),
            calculate_block_size(data.size())
        );
    }

    void JumpSearch::set_search_target(const int target)
    {
        m_target = target;
        reset();
        generate_all_steps();
    }

    void JumpSearch::reset_state()
    {
        m_result_index = -1;
        m_metrics = PerformanceMetrics{};
    }

    void JumpSearch::generate_all_steps()
    {
        if (m_original_data.empty())
        {
            LOG_WARNING("Cannot generate steps: empty data");
            return;
        }

        m_steps.clear();
        m_metrics.total_comparisons = 0;
        m_metrics.jumps_performed = 0;
        m_metrics.linear_scans = 0;
        m_result_index = -1;

        // Initializing state
        State state;
        state.data = m_original_data;
        state.target = m_target;
        state.step = calculate_block_size(m_original_data.size());
        state.prev = 0;
        state.current_index = 0;
        state.block_start = 0;
        state.block_end = 0;
        state.comparisons = 0;
        state.jumps_performed = 0;
        state.linear_scans = 0;
        state.result_index = -1;
        state.found = false;
        state.phase = SearchPhase::JUMPING;
        state.iteration = 0;
        state.search_progress = 0.0f;

        // Track visualization data
        std::vector<size_t> jumped_indices;
        std::vector<size_t> scanned_indices;
        std::vector<std::pair<size_t, size_t>> block_indices;

        // Set optimal block size for metrics
        m_metrics.optimal_block_size = std::sqrt(m_original_data.size());
        m_metrics.block_size = state.step;

        // Initialization step
        push_step(
            state, 
            "init", 
            jumped_indices, 
            scanned_indices, 
            block_indices
        );

        // Performing jump search
        perform_search(
            state, 
            jumped_indices, 
            scanned_indices, 
            block_indices
        );

        // Final completion step
        push_step(
            state, 
            "completed", 
            jumped_indices, 
            scanned_indices, 
            block_indices
        );

        // Update metrics
        m_metrics.total_comparisons = state.comparisons;
        m_metrics.jumps_performed = state.jumps_performed;
        m_metrics.linear_scans = state.linear_scans;
        m_metrics.found = state.found;
        m_metrics.result_index = state.result_index;

        LOG_DEBUG(
            "Generated {} steps for jump search (target: {}, block size: {})",
            m_steps.size(),
            m_target,
            state.step
        );
    }

    void JumpSearch::perform_search(
        State& state, 
        std::vector<size_t>& jumped_indices,
        std::vector<size_t>& scanned_indices,
        std::vector<std::pair<size_t, size_t>>& block_indices)
    {
        const size_t n = state.data.size();
        
        // Jumping through blocks
        state.phase = SearchPhase::JUMPING;
        
        while (state.step <= n)
        {
            state.iteration++;
            size_t boundary_idx = std::min(state.step, n) - 1;
            
            // Record jump
            jumped_indices.push_back(boundary_idx);
            state.jumps_performed = jumped_indices.size();
            
            // Jump start step
            push_step(
                state, 
                "jump_start", 
                jumped_indices, 
                scanned_indices, 
                block_indices
            );
            
            // Jump comparison step
            state.comparisons++;
            push_step(
                state, 
                "jump_compare", 
                jumped_indices, 
                scanned_indices, 
                block_indices
            );

            if (state.data[boundary_idx] >= state.target)
            {
                // Found the block where target might be
                state.block_start = state.prev;
                state.block_end = state.step;
                block_indices.emplace_back(state.prev, state.step);
                state.phase = SearchPhase::BLOCK_FOUND;
                
                push_step(
                    state, 
                    "block_found", 
                    jumped_indices, 
                    scanned_indices, 
                    block_indices
                );
                break;
            }
            
            // Jump forward
            state.prev = state.step;
            state.step += calculate_block_size(n);
            
            // Check if we've exceeded array bounds
            if (state.prev >= n)
            {
                push_step(
                    state, 
                    "not_found", 
                    jumped_indices, 
                    scanned_indices, 
                    block_indices
                );
                return;
            }
            
            push_step(
                state, 
                "jump_forward", 
                jumped_indices, 
                scanned_indices, 
                block_indices
            );
        }
        
        // Phase 2: Linear scan in identified block
        state.phase = SearchPhase::LINEAR_SCAN;
        push_step(
            state, 
            "linear_scan_start", 
            jumped_indices, 
            scanned_indices, 
            block_indices
        );
        
        size_t block_end = std::min(state.block_end, n);
        for (size_t i = state.block_start; i < block_end && !state.found; ++i)
        {
            state.current_index = i;
            scanned_indices.push_back(i);
            state.linear_scans = scanned_indices.size();
            
            // Linear comparison step
            state.comparisons++;
            push_step(
                state, 
                "linear_compare", 
                jumped_indices, 
                scanned_indices, 
                block_indices
            );
            
            if (state.data[i] == state.target)
            {
                // Target found
                state.found = true;
                state.result_index = static_cast<int>(i);
                m_result_index = static_cast<int>(i);
                state.phase = SearchPhase::COMPLETE;
                push_step(
                    state, "found", 
                    jumped_indices, 
                    scanned_indices, 
                    block_indices
                );
                return;
            }
            
            // Continue linear scan
            if (i < block_end - 1)
            {
                push_step(
                    state, 
                    "linear_next", 
                    jumped_indices, 
                    scanned_indices, 
                    block_indices
                );
            }
        }
        
        // Target not found
        if (!state.found)
        {
            state.result_index = -1;
            m_result_index = -1;
            state.phase = SearchPhase::COMPLETE;
            push_step(
                state, 
                "not_found", 
                jumped_indices, 
                scanned_indices, 
                block_indices
            );
        }
    }

    void JumpSearch::push_step(
        const State& state,
        const std::string& operation_id,
        const std::vector<size_t>& jumped_indices,
        const std::vector<size_t>& scanned_indices,
        const std::vector<std::pair<size_t, size_t>>& block_indices)
    {
        auto step = create_step_from_state(
            state, 
            operation_id, 
            jumped_indices, 
            scanned_indices, 
            block_indices
        );
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep JumpSearch::create_step_from_state(
        const State& state,
        const std::string& operation_id,
        const std::vector<size_t>& jumped_indices,
        const std::vector<size_t>& scanned_indices,
        const std::vector<std::pair<size_t, size_t>>& block_indices
    ) const
    {
        AlgorithmStep step;
        
        // Core data
        step.data = state.data;
        step.metadata.operation_id = operation_id;
        
        // Populating metadata using JSON-driven approach
        populate_step_metadata(step, state, operation_id);
        
        // Generating description using JSON template
        step.description = format_step_description(
            operation_id, 
            step
        );
        
        // Updating visualization data
        update_visualization_data(
            step, 
            state, 
            operation_id, 
            jumped_indices, 
            scanned_indices, 
            block_indices
        );
        
        return step;
    }

    void JumpSearch::populate_step_metadata(
        AlgorithmStep& step, const State& state,
        const std::string& operation_id) const
    {
        const size_t n = state.data.size();
        
        // Core search variables
        step.metadata.set(
            "target", 
            state.target, 
            "Target value being searched for"
        );
        step.metadata.set(
            "step", 
            state.step, 
            "Current jump step size"
        );
        step.metadata.set(
            "prev", 
            state.prev, 
            "Previous block boundary"
        );
        step.metadata.set(
            "current_index", 
            state.current_index, 
            "Current index being examined"
        );
        
        // Block boundaries
        if (state.block_end > 0)
        {
            step.metadata.set(
                "block_start", 
                state.block_start, 
                "Start of current block"
            );
            step.metadata.set(
                "block_end", 
                state.block_end, 
                "End of current block"
            );
        }
        
        // Adding boundary values if available
        size_t boundary_idx = std::min(state.step, n) - 1;
        if (boundary_idx < n)
        {
            step.metadata.set(
                "boundary_value",
                state.data[boundary_idx],
                "Value at jump boundary"
            );
            step.metadata.set(
                "boundary_idx",
                boundary_idx,
                "Index of jump boundary"
            );
        }
        
        // Add current value if available
        if (state.current_index < n)
        {
            step.metadata.set(
                "current_value",
                state.data[state.current_index],
                "Value at current index"
            );
        }
        
        // Performance metrics
        step.metadata.set(
            "comparisons", 
            state.comparisons, 
            "Total comparisons performed"
        );
        step.metadata.set(
            "jumps_performed", 
            state.jumps_performed, 
            "Number of jumps made"
        );
        step.metadata.set(
            "linear_scans", 
            state.linear_scans, 
            "Number of linear comparisons"
        );
        step.metadata.set(
            "block_size", 
            calculate_block_size(n), 
            "Block size (√n)"
        );
        step.metadata.set(
            "iteration", 
            state.iteration, 
            "Current iteration number"
        );
        step.metadata.set(
            "found", 
            state.found, 
            "Whether target has been found"
        );
        step.metadata.set(
            "result_index", 
            state.result_index, 
            "Index where target was found (-1 if not found)"
        );
        step.metadata.set(
            "size", 
            n, "Total array size"
        );
        
        // Phase string for display
        std::string phase_str;
        switch (state.phase)
        {
            case SearchPhase::JUMPING: 
                phase_str = "Jumping"; 
                break;
            case SearchPhase::BLOCK_FOUND: 
                phase_str = "Block Found"; 
                break;
            case SearchPhase::LINEAR_SCAN: 
                phase_str = "Linear Scan"; 
                break;
            case SearchPhase::COMPLETE: 
                phase_str = "Complete"; 
                break;
        }
        step.metadata.set(
            "phase", 
            phase_str, 
            "Current search phase"
        );
        
        // Add context for specific operations
        if (operation_id == "jump_compare")
        {
            step.metadata.set(
                "can_jump_further",
                (boundary_idx < n && state.data[boundary_idx] < state.target),
                "Whether we need to jump further"
            );
        }
        else if (operation_id == "found")
        {
            step.metadata.set(
                "found_status", 
                "found", 
                "Search result status"
            );
        }
        else if (operation_id == "not_found")
        {
            step.metadata.set(
                "found_status", 
                "not found", 
                "Search result status"
            );
        }
        else if (operation_id == "completed")
        {
            std::string found_status = state.found ? "found" : "not found";
            step.metadata.set(
                "found_status", 
                found_status, 
                "Whether target was found"
            );
        }
        
        // Adding tags from step mapping
        auto tags = get_step_tags(operation_id);
        for (const auto& tag : tags)
        {
            step.metadata.add_tag(tag);
        }
    }

    void JumpSearch::update_visualization_data(
        AlgorithmStep& step,
        const State& state,
        const std::string& operation_id,
        const std::vector<size_t>& jumped_indices,
        const std::vector<size_t>& scanned_indices,
        const std::vector<std::pair<size_t, size_t>>& block_indices
    ) const
    {
        auto& viz = step.visualization;

        // Basic metrics
        viz.comparison_count = state.comparisons;
        viz.target_value = state.target;

        // Clear previous highlights
        viz.highlighted_index = std::nullopt;
        viz.compared_index = std::nullopt;
        viz.additional_highlights.clear();

        // Populate search-specific visualization data
        viz.search.target_value = state.target;
        viz.search.is_searching = !state.found && state.phase != SearchPhase::COMPLETE;
        viz.search.search_step = state.iteration;
        
        // Calculate search progress based on phase
        if (state.phase == SearchPhase::JUMPING)
        {
            viz.search.search_progress = static_cast<float>(state.prev) / state.data.size();
        }
        else if (state.phase == SearchPhase::LINEAR_SCAN)
        {
            viz.search.search_progress = static_cast<float>(state.current_index) / state.data.size();
        }
        else if (state.phase == SearchPhase::BLOCK_FOUND)
        {
            viz.search.search_progress = static_cast<float>(state.block_start) / state.data.size();
        }
        
        // Set boundaries for visualization
        viz.search.left_boundary = state.block_start;
        viz.search.right_boundary = state.block_end;
        
        // Set searched indices (jumped positions + scanned positions)
        viz.search.searched_indices = jumped_indices;
        viz.search.searched_indices.insert(
            viz.search.searched_indices.end(),
            scanned_indices.begin(),
            scanned_indices.end()
        );
        
        // Eliminated indices are blocks that were jumped over
        std::vector<size_t> eliminated;
        for (size_t i = 0; i < state.prev && i < state.data.size(); ++i)
        {
            // Don't eliminate if in current block or scanned
            if (i >= state.block_start && i < state.block_end)
                continue;
            
            auto it = std::find(scanned_indices.begin(), scanned_indices.end(), i);
            if (it == scanned_indices.end())
            {
                eliminated.push_back(i);
            }
        }
        viz.search.eliminated_indices = eliminated;
        
        // Set current focus based on phase
        if (state.phase == SearchPhase::JUMPING && !jumped_indices.empty())
        {
            size_t last_jump = jumped_indices.back();
            if (last_jump < state.data.size())
            {
                viz.highlighted_index = last_jump;
                viz.compared_index = last_jump;
            }
        }
        else if (state.phase == SearchPhase::LINEAR_SCAN && 
                 state.current_index < state.data.size())
        {
            viz.highlighted_index = state.current_index;
            viz.compared_index = state.current_index;
        }
        
        // Set block boundaries for additional highlights
        for (size_t i = state.block_start; i < state.block_end && i < state.data.size(); ++i)
        {
            viz.additional_highlights.push_back(i);
        }
        
        // Store block indices for visualization
        // (This would be used by custom renderers to show blocks)
        if (!block_indices.empty())
        {
            // Store the current block in metadata for visualization
            const auto& current_block = block_indices.back();
            step.metadata.set(
                "visual_block_start", 
                current_block.first, 
                "Block start for visualization"
            );
            step.metadata.set(
                "visual_block_end", 
                current_block.second, 
                "Block end for visualization"
            );
        }
        
        // Set mid point for compatibility (for linear scan phase)
        if (state.phase == SearchPhase::LINEAR_SCAN && 
            state.current_index < state.data.size())
        {
            viz.search.mid_point = state.current_index;
        }
        else
        {
            viz.search.mid_point = std::nullopt;
        }
        
        // Set found index if applicable
        if (state.found && state.result_index >= 0)
        {
            viz.search.found_index = static_cast<size_t>(state.result_index);
            viz.is_found = true;
            
            // Also add to additional highlights for compatibility
            viz.additional_highlights.push_back(
                static_cast<size_t>(state.result_index)
            );
        }
        else
        {
            viz.search.found_index = std::nullopt;
            viz.is_found = false;
        }
        
        // For backward compatibility
        viz.eliminated_regions = eliminated;
        
        // Store jump count in visualization
        step.metadata.set(
            "jumps", 
            state.jumps_performed, 
            "Jump count"
        );
        step.metadata.set(
            "phase_state", 
            static_cast<int>(state.phase), 
            "Phase state"
        );
    }

} // namespace c2l::algorithms