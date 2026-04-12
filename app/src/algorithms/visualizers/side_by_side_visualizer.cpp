//
// Created by Akhmad on 3/12/26.
//

#include "algorithms/visualizers/side_by_side_visualizer.hpp"

#include "core/utils/logger/logger.hpp"

namespace c2l::algorithms
{
    void SideBySideVisualizer::initialize(
        ISimpleAlgorithm* left_algo,
        const IAlgorithmMetadata* left_meta,
        ISimpleAlgorithm* right_algo,
        const IAlgorithmMetadata* right_meta)
    {
        m_left_algorithm = left_algo;
        m_left_metadata = left_meta;
        m_right_algorithm = right_algo;
        m_right_metadata = right_meta;

        if (left_algo && left_meta)
        {
            m_left_visualizer.initialize(left_algo, left_meta);
        }

        if (right_algo && right_meta)
        {
            m_right_visualizer.initialize(right_algo, right_meta);
        }

        LOG_DEBUG("SideBySideVisualizer initialized");
    }

    void SideBySideVisualizer::update(double dt)
    {
        m_animation_time += dt;

        if (m_left_algorithm)
            m_left_visualizer.update(dt);

        if (m_right_algorithm)
            m_right_visualizer.update(dt);
    }

    void SideBySideVisualizer::render()
    {
        ImVec2 available = ImGui::GetContentRegionAvail();

        // Calculate split sizes
        float left_width = available.x * m_split_position - 5.0f;
        float right_width = available.x * (1.0f - m_split_position) - 5.0f;
        float height = available.y;

        // Left algorithm
        {
            ImGui::BeginChild(
                "LeftAlgorithm", 
                ImVec2(left_width, height), 
                true
            );
            render_algorithm_side(
                "Left", 
                m_left_algorithm, 
                m_left_metadata,
                ImVec2(left_width, height), 
                true
            );
            ImGui::EndChild();
        }

        ImGui::SameLine();

        // Splitter
        ImGui::PushStyleColor(
            ImGuiCol_Button, 
            ImVec4(0.3f, 0.3f, 0.3f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered, 
            ImVec4(0.4f, 0.4f, 0.4f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive, 
            ImVec4(0.5f, 0.5f, 0.5f, 1.0f)
        );

        ImGui::Button("||", ImVec2(10, height));
        if (ImGui::IsItemActive())
        {
            m_split_position += ImGui::GetIO().MouseDelta.x / available.x;
            m_split_position = std::clamp(m_split_position, 0.2f, 0.8f);
        }

        ImGui::PopStyleColor(3);
        ImGui::SameLine();

        // Right algorithm
        {
            ImGui::BeginChild(
                "RightAlgorithm", 
                ImVec2(right_width, height), 
                true
            );
            render_algorithm_side(
                "Right", 
                m_right_algorithm, 
                m_right_metadata,
                ImVec2(right_width, height), 
                false
            );
            ImGui::EndChild();
        }
    }

    void SideBySideVisualizer::render_algorithm_side(
        const char* side_name,
        ISimpleAlgorithm* algorithm,
        const IAlgorithmMetadata* metadata,
        const ImVec2& size,
        bool is_left)
    {
        if (!algorithm || !metadata)
        {
            ImGui::TextColored(
                ImVec4(1,0,0,1), 
                "No algorithm selected"
            );
            return;
        }

        // Header
        ImGui::TextColored(
            ImVec4(0.2f, 0.8f, 1.0f, 1.0f), 
            "%s: %s",
            side_name, 
            metadata->get_display_name().c_str()
        );

        // Current step info
        auto current_step = algorithm->get_current_step();
        ImGui::Text(
            "Step %zu/%zu", 
            algorithm->get_current_step_index(),
            algorithm->get_step_count() - 1
        );

        ImGui::Separator();

        // Visualization area
        float viz_height = size.y - (m_show_metrics ? 150.0f : 80.0f);
        ImGui::BeginChild(
            "Visualization", 
            ImVec2(0, viz_height), 
            false
        );

        if (is_left)
            m_left_visualizer.render();
        else
            m_right_visualizer.render();

        ImGui::EndChild();

        // Metrics
        if (m_show_metrics)
        {
            ImGui::Separator();
            ImGui::Text("Performance Metrics");
            ImGui::Separator();

            ImGui::Columns(2, nullptr, false);

            ImGui::Text(
                "Comparisons: %zu", 
                current_step.visualization.comparisons
            );
            ImGui::NextColumn();
            ImGui::Text(
                "Swaps: %zu", 
                current_step.visualization.swaps
            );

            ImGui::Columns(1);

            // Complexity info
            const auto& complexity = metadata->get_complexity();
            ImGui::TextColored(
                ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                "Best: %s | Avg: %s | Worst: %s",
                complexity.time_best.c_str(),
                complexity.time_average.c_str(),
                complexity.time_worst.c_str()
            );
        }
    }

} // namespace c2l::algorithms