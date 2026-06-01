//
// Created by Akhmad on 3/12/26.
//

#include "algorithms/visualizers/side_by_side_visualizer.hpp"

#include "graph_based_visualizer.hpp"
#include "path_finding_visualizer.hpp"
#include "core/utils/logger/logger.hpp"

namespace c2l::algorithms
{
    void SideBySideVisualizer::initialize(
        const ParallelAlgorithm& left,
        const ParallelAlgorithm& right,
        SharedGridState* shared_grid_state)
    {
        if (m_left == &left && m_right == &right)
            return;

        m_left = &left;
        m_right = &right;
        m_shared_grid_state = shared_grid_state;

        // Sync shared grid state to both visualizers
        if (m_shared_grid_state && m_left->visualizer && m_right->visualizer)
        {
            sync_shared_grid_to_visualizer(*m_left->visualizer);
            sync_shared_grid_to_visualizer(*m_right->visualizer);
        }

        LOG_DEBUG("SideBySideVisualizer initialized: left={}, right={}",
                  left.name, right.name);
    }

    void SideBySideVisualizer::update(double dt)
    {
        m_animation_time += dt;

        if (m_left && m_left->visualizer)
            m_left->visualizer->update(dt);

        if (m_right && m_right->visualizer)
            m_right->visualizer->update(dt);
    }

        void SideBySideVisualizer::render()
    {
        if (!m_left || !m_right)
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "SideBySideVisualizer not initialized");
            return;
        }

        ImVec2 available = ImGui::GetContentRegionAvail();

        if (available.x <= 0 || available.y <= 0)
            return;

        float left_width = available.x * m_split_position - 10.0f;
        float right_width = available.x * (1.0f - m_split_position) - 10.0f;
        float height = available.y;

        // Left algorithm panel
        ImGui::BeginChild("LeftAlgorithmPanel", ImVec2(left_width, height), true);
        render_algorithm_side("Left", *m_left, ImVec2(left_width, height));
        ImGui::EndChild();

        ImGui::SameLine();

        // Splitter
        render_splitter(height);
        ImGui::SameLine();

        // Right algorithm panel
        ImGui::BeginChild("RightAlgorithmPanel", ImVec2(right_width, height), true);
        render_algorithm_side("Right", *m_right, ImVec2(right_width, height));
        ImGui::EndChild();
    }

    void SideBySideVisualizer::render_algorithm_side(
        const char* side_name,
        const ParallelAlgorithm& algo,
        const ImVec2& size)
    {
        if (!algo.is_valid())
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "No algorithm for %s", side_name);
            return;
        }

        // Header with algorithm name and visualization type
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f),
                          "%s: %s", side_name, algo.name.c_str());

        // Visualization type badge
        // ImGui::SameLine();
        // ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        // std::string viz_badge =
        //     algo.metadata->get_visualization_type();
        // ImGui::SmallButton(viz_badge.c_str());
        // ImGui::PopStyleColor();

        // Progress info
        auto current_step = algo.algorithm->get_current_step();
        ImGui::Text("Step %zu / %zu",
                    algo.algorithm->get_current_step_index(),
                    algo.algorithm->get_step_count() - 1);

        // float progress = algo.algorithm->get_step_count() > 0 ?
        //     static_cast<float>(algo.algorithm->get_current_step_index()) /
        //     static_cast<float>(algo.algorithm->get_step_count() - 1) : 0.0f;
        // ImGui::ProgressBar(progress, ImVec2(-1, 6));

        ImGui::Separator();

        // Visualization area
        float viz_height = size.y - (m_show_metrics ? 180.0f : 100.0f);
        ImGui::BeginChild(("Viz_" + std::string(side_name)).c_str(),
                         ImVec2(0, viz_height), true);

        if (algo.visualizer)
        {
            algo.visualizer->render();
        }
        else
        {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Visualizer not available");
        }

        ImGui::EndChild();

        // Metrics panel
        if (m_show_metrics)
        {
            render_metrics_panel(algo.algorithm.get(), algo.metadata);
        }
    }

    void SideBySideVisualizer::render_metrics_panel(
        ISimpleAlgorithm* algorithm,
        const IAlgorithmMetadata* metadata)
    {
        if (!algorithm || !metadata) return;

        auto current_step = algorithm->get_current_step();

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Performance Metrics");
        ImGui::Separator();

        ImGui::Columns(2, nullptr, false);

        ImGui::Text("Comparisons: %zu", current_step.visualization.comparison_count);
        ImGui::NextColumn();
        ImGui::Text("Swaps: %zu", current_step.visualization.swap_count);

        ImGui::Columns(1);

        const auto& complexity = metadata->get_complexity();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
            "Best: %s | Avg: %s | Worst: %s",
            complexity.time_best.c_str(),
            complexity.time_average.c_str(),
            complexity.time_worst.c_str());

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
            "Memory: %.2f KB | Time: %.2f ms",
            static_cast<float>(algorithm->get_peak_memory_bytes()) / 1024.0f,
            static_cast<float>(algorithm->get_algorithm_time_us()) / 1000.0f);
    }

    void SideBySideVisualizer::render_splitter(float height)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

        ImGui::Button("||", ImVec2(8, height));

        if (ImGui::IsItemActive())
        {
            ImGuiIO& io = ImGui::GetIO();
            m_split_position += io.MouseDelta.x / ImGui::GetContentRegionAvail().x;
            m_split_position = std::clamp(m_split_position, 0.2f, 0.8f);
        }

        ImGui::PopStyleColor(3);
    }

    void SideBySideVisualizer::sync_shared_grid_to_visualizer(IAlgorithmVisualizer& visualizer)
    {
        if (!m_shared_grid_state) return;

        if (auto* path_viz = dynamic_cast<PathFindingVisualizer*>(&visualizer))
        {
            // Sync grid data to visualizer's internal state
            path_viz->set_grid_size(m_shared_grid_state->rows, m_shared_grid_state->cols);
            path_viz->clear_grid();

            for (int row = 0; row < m_shared_grid_state->rows; ++row)
            {
                for (int col = 0; col < m_shared_grid_state->cols; ++col)
                {
                    auto cell_type = m_shared_grid_state->grid[row][col];

                    if (cell_type == GridCellType::START)
                    {
                        path_viz->set_start(row, col);
                    }
                    else if (cell_type == GridCellType::TARGET)
                    {
                        path_viz->set_target(row, col);
                    }
                    else if (cell_type == GridCellType::WALL)
                    {
                        path_viz->set_wall(row, col, true);
                    }
                    else if (cell_type >= GridCellType::WEIGHT_1 &&
                             cell_type <= GridCellType::WEIGHT_5)
                    {
                        int weight = static_cast<int>(cell_type) -
                                    static_cast<int>(GridCellType::WEIGHT_1) + 1;
                        path_viz->set_weight(row, col, weight);
                    }
                    else
                    {
                        path_viz->set_wall(row, col, false);
                    }
                }
            }

            // Force a sync from visualizer to algorithm
            path_viz->sync_algorithm_with_grid();

            // Sync display options
            path_viz->set_show_grid_lines(m_show_grid_lines);
            path_viz->set_show_weights(m_show_weights);
            path_viz->set_show_coordinates(m_show_coordinates);

        }
    }

    void SideBySideVisualizer::sync_shared_graph_to_visualizer(IAlgorithmVisualizer& visualizer)
    {
        if (!m_shared_graph_state) return;

        if (auto* graph_viz = dynamic_cast<algorithms::GraphBasedVisualizer*>(&visualizer))
        {
            graph_viz->set_show_edge_weights(m_show_edge_weights);
            graph_viz->set_show_node_labels(m_show_node_labels);
            graph_viz->set_show_node_ids(m_show_node_ids);
            graph_viz->set_show_distances(m_show_distances);
            graph_viz->sync_from_shared_state(*m_shared_graph_state);
        }
    }

} // namespace c2l::algorithms