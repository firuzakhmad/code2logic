#include "array_based_visualizer.hpp"
#include "core/utils/logger/logger.hpp"

#include <math.h>
#include <algorithm>

#include "core/utils/utils.hpp"

namespace c2l::algorithms
{
	void ArrayBasedVisualizer::initialize(
        const ISimpleAlgorithm* execution,
        const IAlgorithmMetadata* metadata)
	{
        m_execution = execution;
        m_metadata = metadata;

        LOG_DEBUG(
            "ArrayBasedVisualizer initialized for {}",
            m_metadata ? m_metadata->get_display_name() : "Unknown"
        );
    }

    void ArrayBasedVisualizer::render()
    {
    	if (!m_execution) return;

        const auto current_step = m_execution->get_current_step();
    	render_array_visualization(current_step);
    }

    void ArrayBasedVisualizer::update(double delta_time) 
    {}

    void ArrayBasedVisualizer::render_header() 
    {
        if (!m_metadata) return;

        ImGui::Text(
            "Time: %s | Space: %s", 
            m_metadata->get_complexity().time_average.c_str(),
            m_metadata->get_complexity().space.c_str()
        );
        
        // Progress bar
        float progress = m_execution->get_step_count() > 0 ? 
            static_cast<float>(m_execution->get_current_step_index()) / 
            (m_execution->get_step_count() - 1) : 0.0f;
        
        char progress_text[64];
        snprintf(
            progress_text, 
            sizeof(progress_text), 
            "%zu/%zu", 
            m_execution->get_current_step_index(), 
            m_execution->get_step_count()
        );
        
        ImGui::ProgressBar(progress, ImVec2(-1, 20), progress_text);
        ImGui::Separator();
    }

    void ArrayBasedVisualizer::render_array_visualization(
        const AlgorithmStep& step) 
    {
        if (m_metadata->get_category() == AlgorithmCategory::SORTING)
        {
            render_sorting_visualization(step);
        } else {
            render_searching_visualization(step);
        }
    }

    void ArrayBasedVisualizer::render_sorting_visualization(
        const AlgorithmStep& step) 
    {
	    const char* styles[] = {
	        "Enhanced Bars", "Circular", "Network", "Waveform",
            "Heat Map", "Particle System", "Tree View",
            "Molecular", "Neural Network", "Classic Bars", "Dots"
        };

	    ImGui::SetNextItemWidth(180);
	    ImGui::Combo(
            " ", 
            &m_visualization_style, 
            styles, 
            IM_COUNTOF(styles)
        );

	    switch (m_visualization_style)
	    {
	        case 0: render_enhanced_bar_visualization(step); break;
	        case 1: render_circular_visualization(step); break;
	        case 2: render_network_visualization(step); break;
	        case 3: render_waveform_visualization(step); break;
	        case 4: render_heatmap_visualization(step); break;
	        case 5: render_particle_visualization(step); break;
	        case 6: render_tree_visualization(step); break;
	        case 7: render_molecular_visualization(step); break;
	        case 8: render_neural_network_visualization(step); break;
	        case 9: render_bar_visualization(step); break;
	        case 10: render_dot_visualization(step); break;
	        default: render_enhanced_bar_visualization(step); break;
	    }
    }

    void ArrayBasedVisualizer::render_bar_visualization(
        const AlgorithmStep& step) 
    {
        ImGui::BeginChild(
            "BarVisualization", 
            ImVec2(0, 235), 
            true
        );
        
        float available_width = ImGui::GetContentRegionAvail().x;
        float bar_width = std::max(10.0f, available_width / step.data.size() - 2.0f);
        bar_width = std::min(bar_width, 60.0f);
        
        if (step.data.empty()) 
        {
            ImGui::Text("No data to visualize");
            ImGui::EndChild();
            return;
        }
        
        int max_value = *std::max_element(step.data.begin(), step.data.end());
        if (max_value == 0) max_value = 1; // Avoiding division by zero
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        
        float start_y = cursor_pos.y + 200;
        float max_bar_height = 180.0f;
        
        for (size_t i = 0; i < step.data.size(); ++i) 
        {
            float bar_height = (static_cast<float>(step.data[i]) / max_value) * max_bar_height;
            ImVec2 bar_min(cursor_pos.x + i * (bar_width + 2), start_y - bar_height);
            ImVec2 bar_max(cursor_pos.x + i * (bar_width + 2) + bar_width, start_y);
            
            // Getting color based on element state
            ImU32 color = get_element_color(step, i);
            
            // Draw the bar
            draw_list->AddRectFilled(bar_min, bar_max, color);
            draw_list->AddRect(bar_min, bar_max, ImColor(255, 255, 255, 255));
            
            // Drawing value label for reasonable-sized arrays
            if (step.data.size() <= 20) 
            {
                std::string value_str = std::to_string(step.data[i]);
                ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
                float text_x = bar_min.x + (bar_width - text_size.x) * 0.5f;
                float text_y = bar_min.y - text_size.y - 2;
                
                // Ensuring text doesn't go above the window
                if (text_y >= cursor_pos.y) 
                {
                    draw_list->AddText(
                        ImVec2(text_x, text_y), 
                        ImColor(255, 255, 255, 255),
                        value_str.c_str()
                    );
                }
            }
            
            // Drawing index below bar
            std::string index_str = std::to_string(i);
            ImVec2 index_size = ImGui::CalcTextSize(index_str.c_str());
            float index_x = bar_min.x + (bar_width - index_size.x) * 0.5f;
            draw_list->AddText(ImVec2(index_x, start_y + 5), 
                              ImColor(200, 200, 200, 255),
                              index_str.c_str());
        }
        
        ImGui::EndChild();
    }

    void ArrayBasedVisualizer::render_dot_visualization(
        const AlgorithmStep& step) 
    {
        ImGui::BeginChild(
            "DotVisualization", 
            ImVec2(0, 150), 
            true
        );
        
        if (step.data.empty()) 
        {
            ImGui::Text("No data to visualize");
            ImGui::EndChild();
            return;
        }
        
        int max_value = *std::max_element(step.data.begin(), step.data.end());
        if (max_value == 0) max_value = 1;
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 region_size = ImGui::GetContentRegionAvail();
        
        float dot_spacing = region_size.x / step.data.size();
        float max_dot_radius = 20.0f;
        
        for (size_t i = 0; i < step.data.size(); ++i) 
        {
            float dot_radius = (static_cast<float>(step.data[i]) / max_value) * max_dot_radius;
            dot_radius = std::max(dot_radius, 5.0f); // Minimum size
            
            ImVec2 dot_center(
                cursor_pos.x + i * dot_spacing + dot_spacing * 0.5f,
                cursor_pos.y + region_size.y * 0.5f
            );
            
            ImU32 color = get_element_color(step, i);
            
            // Draw the dot
            draw_list->AddCircleFilled(dot_center, dot_radius, color);
            draw_list->AddCircle(dot_center, dot_radius, ImColor(255, 255, 255, 255));
            
            // Value label
            if (dot_radius > 8) 
            {
                std::string value_str = std::to_string(step.data[i]);
                ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
                draw_list->AddText(
                    ImVec2(
                        dot_center.x - text_size.x * 0.5f, 
                        dot_center.y - text_size.y * 0.5f
                    ),
                    ImColor(255, 255, 255, 255),
                    value_str.c_str()
                );
            }
        }
        
        ImGui::EndChild();
    }

    void ArrayBasedVisualizer::render_enhanced_bar_visualization(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild(
            "EnhancedBarVisualization", 
            ImVec2(0, 280), 
            true
        );

        float available_width = ImGui::GetContentRegionAvail().x;
        float bar_width = std::max(8.0f, available_width / step.data.size() - 1.0f);
        bar_width = std::min(bar_width, 40.0f);

        if (step.data.empty())
        {
            ImGui::Text("No data to visualize");
            ImGui::EndChild();
            return;
        }

        int max_value = *std::max_element(step.data.begin(), step.data.end());
        if (max_value == 0) max_value = 1;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

        float start_y = cursor_pos.y + 220;
        float max_bar_height = 180.0f;
        float spacing = 1.0f;

        // Drawing gradient background
        draw_list->AddRectFilledMultiColor(
            cursor_pos,
            ImVec2(cursor_pos.x + available_width, cursor_pos.y + 240),
            ImColor(20, 20, 40, 255),
            ImColor(20, 20, 40, 255),
            ImColor(10, 10, 20, 255),
            ImColor(10, 10, 20, 255)
        );

        for (size_t i = 0; i < step.data.size(); ++i)
        {
            float bar_height = (static_cast<float>(step.data[i]) / max_value) * max_bar_height;
            ImVec2 bar_min(cursor_pos.x + i * (bar_width + spacing), start_y - bar_height);
            ImVec2 bar_max(cursor_pos.x + i * (bar_width + spacing) + bar_width, start_y);

            // Getting enhanced color with gradient
            ImU32 base_color = get_enhanced_element_color(step, i);
            ImU32 top_color = apply_color_variation(base_color, 1.3f); // Lighter top

            // Drawing bar with gradient
            draw_list->AddRectFilledMultiColor(
                bar_min, bar_max,
                top_color, top_color, base_color, base_color
            );

            // 3D effect with borders
            draw_list->AddRect(bar_min, bar_max, ImColor(255, 255, 255, 80), 0, 0, 1.0f);

            // Highlight effects for active elements
            if (i == step.visualization.highlighted_index ||
                i == step.visualization.compared_index)
            {
                // Glow effect
                ImVec2 glow_min = ImVec2(bar_min.x - 2, bar_min.y - 2);
                ImVec2 glow_max = ImVec2(bar_max.x + 2, bar_max.y + 2);
                draw_list->AddRect(glow_min, glow_max, ImColor(255, 255, 255, 150), 0, 0, 2.0f);

                // Pulse animation (simple version)
                float pulse = (sin(static_cast<float>(ImGui::GetTime()) * 8.0f) + 1.0f) * 0.5f;
                ImU32 pulse_color = ImColor(255, 255, 255, static_cast<int>(100 * pulse));
                draw_list->AddRect(bar_min, bar_max, pulse_color, 0, 0, 1.5f);
            }

            // Draw value and index with better typography
            if (step.data.size() <= 25 && bar_height > 20) 
            {
                std::string value_str = std::to_string(step.data[i]);
                ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
                float text_x = bar_min.x + (bar_width - text_size.x) * 0.5f;
                float text_y = bar_min.y - text_size.y - 1;

                if (text_y >= cursor_pos.y) 
                {
                    draw_list->AddText(ImVec2(text_x, text_y),
                                      ImColor(240, 240, 240, 255),
                                      value_str.c_str());
                }
            }

            // Index with subtle styling
            std::string index_str = std::to_string(i);
            ImVec2 index_size = ImGui::CalcTextSize(index_str.c_str());
            float index_x = bar_min.x + (bar_width - index_size.x) * 0.5f;
            draw_list->AddText(ImVec2(index_x, start_y + 8),
                              ImColor(180, 180, 180, 200),
                              index_str.c_str());
        }

        // Draw comparison lines if comparing two elements
        if (step.visualization.highlighted_index != static_cast<size_t>(-1) &&
            step.visualization.compared_index != static_cast<size_t>(-1))
        {
            draw_comparison_line(draw_list, cursor_pos, step, bar_width, spacing, start_y);
        }

        ImGui::EndChild();
    }

    ImU32 ArrayBasedVisualizer::get_enhanced_element_color(
        const AlgorithmStep& step, 
        size_t index)
    {
        // Sophisticated color coding with smooth transitions
        float value_ratio = static_cast<float>(step.data[index]) /
                           *std::max_element(step.data.begin(), step.data.end());

        // Base color based on value (rainbow spectrum)
        ImU32 base_color = value_to_rainbow_color(value_ratio);

        // State-based modifications
        if (index == step.visualization.highlighted_index)
            return ImColor(255, 200, 50, 255);    // Golden yellow - active element

        if (index == step.visualization.compared_index)
            return ImColor(50, 200, 100, 255);    // Emerald green - comparison element

	    if (std::find(step.visualization.additional_highlights.begin(),
              step.visualization.additional_highlights.end(),
              index)
            != step.visualization.additional_highlights.end())
	    {
	        return ImColor(180, 80, 220, 255);
	    }

        return base_color;
    }

    ImU32 ArrayBasedVisualizer::value_to_rainbow_color(float ratio)
    {
        // Convert value ratio to rainbow color
        int r, g, b;

        if (ratio < 0.25f) {
            // Blue to cyan
            r = 0;
            g = static_cast<int>(255 * (ratio / 0.25f));
            b = 255;
        } else if (ratio < 0.5f) {
            // Cyan to green
            r = 0;
            g = 255;
            b = static_cast<int>(255 * (1.0f - (ratio - 0.25f) / 0.25f));
        } else if (ratio < 0.75f) {
            // Green to yellow
            r = static_cast<int>(255 * (ratio - 0.5f) / 0.25f);
            g = 255;
            b = 0;
        } else {
            // Yellow to red
            r = 255;
            g = static_cast<int>(255 * (1.0f - (ratio - 0.75f) / 0.25f));
            b = 0;
        }

        return ImColor(r, g, b, 255);
    }

    ImU32 ArrayBasedVisualizer::apply_color_variation(
        ImU32 color, 
        float factor)
    {
        // Lighten color for gradient effect
        int r = (color >> IM_COL32_R_SHIFT) & 0xFF;
        int g = (color >> IM_COL32_G_SHIFT) & 0xFF;
        int b = (color >> IM_COL32_B_SHIFT) & 0xFF;

        r = std::min(255, static_cast<int>(r * factor));
        g = std::min(255, static_cast<int>(g * factor));
        b = std::min(255, static_cast<int>(b * factor));

        return ImColor(r, g, b, 255);
    }

    void ArrayBasedVisualizer::draw_comparison_line(
        ImDrawList* draw_list,
        const ImVec2& cursor_pos,
        const AlgorithmStep& step,
        float bar_width,
        float spacing,
        float start_y)
	{
	    const auto& idx1 = step.visualization.highlighted_index;
	    const auto& idx2 = step.visualization.compared_index;

	    if (!idx1 || !idx2)
	        return;

	    if (*idx1 >= step.data.size() || *idx2 >= step.data.size())
	        return;

	    const float max_value =
            static_cast<float>(*std::max_element(step.data.begin(), step.data.end()));

	    const float x1 =
            cursor_pos.x + *idx1 * (bar_width + spacing) + bar_width * 0.5f;
	    const float x2 =
            cursor_pos.x + *idx2 * (bar_width + spacing) + bar_width * 0.5f;

	    const float height1 =
            (step.data[*idx1] / max_value) * 180.0f;
	    const float height2 =
            (step.data[*idx2] / max_value) * 180.0f;

	    ImVec2 p1(x1, start_y - height1 - 10);
	    ImVec2 p2(x2, start_y - height2 - 10);

	    draw_list->AddBezierCubic(
            p1,
            ImVec2(p1.x, p1.y - 20),
            ImVec2(p2.x, p2.y - 20),
            p2,
            ImColor(255, 255, 255, 180),
            2.0f
        );

	    draw_list->AddCircleFilled(p1, 3.0f, ImColor(255, 255, 255, 255));
	    draw_list->AddCircleFilled(p2, 3.0f, ImColor(255, 255, 255, 255));
	}


    void ArrayBasedVisualizer::render_circular_visualization(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild(
            "CircularVisualization", 
            ImVec2(0, 300), 
            true
        );

        if (step.data.empty())
        {
            ImGui::Text("No data to visualize");
            ImGui::EndChild();
            return;
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 region_size = ImGui::GetContentRegionAvail();

        ImVec2 center = ImVec2(cursor_pos.x + region_size.x * 0.5f,
                              cursor_pos.y + region_size.y * 0.5f);
        float radius = std::min(region_size.x, region_size.y) * 0.4f;

        // Draw circular background
        draw_list->AddCircleFilled(center, radius + 10, ImColor(30, 30, 40, 255));
        draw_list->AddCircle(center, radius + 10, ImColor(80, 80, 100, 255), 0, 2.0f);

        int max_value = *std::max_element(step.data.begin(), step.data.end());
        if (max_value == 0) max_value = 1;

        float angle_step = (2.0f * 3.14159f) / step.data.size();

        for (size_t i = 0; i < step.data.size(); ++i)
        {
            float value_ratio = static_cast<float>(step.data[i]) / max_value;
            float bar_length = radius * 0.7f * value_ratio;
            float angle = i * angle_step;

            ImVec2 inner_point = ImVec2(
                center.x + cos(angle) * (radius * 0.3f),
                center.y + sin(angle) * (radius * 0.3f)
            );

            ImVec2 outer_point = ImVec2(
                center.x + cos(angle) * (radius * 0.3f + bar_length),
                center.y + sin(angle) * (radius * 0.3f + bar_length)
            );

            // Get color with state awareness
            ImU32 color = get_enhanced_element_color(step, i);

            // Draw radial bar
            draw_list->AddLine(inner_point, outer_point, color, 8.0f);

            // Draw connection to center for selected elements
            if (i == step.visualization.highlighted_index ||
                i == step.visualization.compared_index)
            {
                draw_list->AddLine(center, inner_point, ImColor(255, 255, 255, 100), 1.0f);

                // Highlight circle
                draw_list->AddCircleFilled(outer_point, 6.0f, ImColor(255, 255, 255, 255));
            }

            // Value label at outer point
            if (step.data.size() <= 30) 
            {
                std::string value_str = std::to_string(step.data[i]);
                ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
                ImVec2 text_pos = ImVec2(
                    outer_point.x - text_size.x * 0.5f + cos(angle) * 5,
                    outer_point.y - text_size.y * 0.5f + sin(angle) * 5
                );
                draw_list->AddText(text_pos, ImColor(240, 240, 240, 255), value_str.c_str());
            }
        }

        // Draw center information
        draw_list->AddCircleFilled(center, radius * 0.2f, ImColor(40, 40, 50, 255));
        draw_list->AddCircle(center, radius * 0.2f, ImColor(100, 100, 120, 255), 0, 2.0f);

        std::string center_text = m_metadata->get_display_name();
        ImVec2 text_size = ImGui::CalcTextSize(center_text.c_str());
        draw_list->AddText(ImVec2(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f),
                          ImColor(200, 200, 220, 255), center_text.c_str());

        ImGui::EndChild();
    }

    void ArrayBasedVisualizer::render_network_visualization(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild(
            "NetworkVisualization", 
            ImVec2(0, 400), 
            true
        );

        if (step.data.empty())
        {
            ImGui::Text("No data to visualize");
            ImGui::EndChild();
            return;
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        // ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 region_size = ImGui::GetContentRegionAvail();

        // Calculate node positions based on current sorted order
        std::vector<ImVec2> node_positions = calculate_network_positions(step.data, region_size);

        int max_value = *std::max_element(step.data.begin(), step.data.end());
        if (max_value == 0) max_value = 1;

        // Draw connections between adjacent elements (showing the comparison sequence)
        for (size_t i = 0; i < step.data.size() - 1; ++i)
        {
            bool is_active_comparison = (i == step.visualization.highlighted_index &&
                                        i + 1 == step.visualization.compared_index);

            ImU32 line_color = is_active_comparison ?
                ImColor(255, 255, 0, 200) : ImColor(80, 80, 120, 100);

            float line_thickness = is_active_comparison ? 3.0f : 1.5f;

            draw_list->AddLine(node_positions[i], node_positions[i + 1],
                              line_color, line_thickness);

            // Draw comparison indicators for active comparisons
            if (is_active_comparison) 
            {
                ImVec2 mid_point(
                    (node_positions[i].x + node_positions[i + 1].x) * 0.5f,
                    (node_positions[i].y + node_positions[i + 1].y) * 0.5f
                );

                // Draw animated comparison indicator
                float pulse = (sin(static_cast<float>(ImGui::GetTime()) * 8.0f) + 1.0f) * 0.5f;
                draw_list->AddCircleFilled(mid_point, 8.0f + pulse * 4.0f, ImColor(255, 255, 0, 150));

                // Show comparison result
                std::string comparison = step.data[i] > step.data[i + 1] ? "SWAP" : "KEEP";
                ImVec2 text_size = ImGui::CalcTextSize(comparison.c_str());
                draw_list->AddText(
                    ImVec2(mid_point.x - text_size.x * 0.5f, mid_point.y - 20),
                    ImColor(255, 255, 255, 255),
                    comparison.c_str()
                );
            }
        }

        // Draw nodes with proper value mapping
        for (size_t i = 0; i < step.data.size(); ++i)
        {
            float value_ratio = static_cast<float>(step.data[i]) / max_value;
            float node_radius = 20.0f + value_ratio * 10.0f;

            ImU32 node_color = get_network_node_color(step, i, value_ratio);

            // Draw node with border
            draw_list->AddCircleFilled(node_positions[i], node_radius, node_color);
            draw_list->AddCircle(node_positions[i], node_radius, ImColor(255, 255, 255, 200), 0, 2.0f);

            // Draw value
            std::string value_str = std::to_string(step.data[i]);
            ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
            draw_list->AddText(
                ImVec2(node_positions[i].x - text_size.x * 0.5f,
                      node_positions[i].y - text_size.y * 0.5f),
                ImColor(255, 255, 255, 255),
                value_str.c_str()
            );

            // Draw index below
            std::string index_str = "[" + std::to_string(i) + "]";
            ImVec2 index_size = ImGui::CalcTextSize(index_str.c_str());
            draw_list->AddText(
                ImVec2(node_positions[i].x - index_size.x * 0.5f,
                      node_positions[i].y + node_radius + 5),
                ImColor(180, 180, 180, 200),
                index_str.c_str()
            );

            // Highlight active nodes
            if (i == step.visualization.highlighted_index ||
                i == step.visualization.compared_index)
            {
                float pulse = (sin(static_cast<float>(ImGui::GetTime()) * 6.0f) + 1.0f) * 0.5f;
                draw_list->AddCircle(
                    node_positions[i],
                    node_radius + 8 + pulse * 6,
                    ImColor(255, 255, 255, static_cast<int>(150 + 100 * pulse)),
                    0, 3.0f
                );
            }
        }

        ImGui::EndChild();
    }

    void ArrayBasedVisualizer::render_waveform_visualization(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild(
            "WaveformVisualization", 
            ImVec2(0, 250), 
            true
        );

        if (step.data.empty())
        {
            ImGui::Text("No data to visualize");
            ImGui::EndChild();
            return;
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 region_size = ImGui::GetContentRegionAvail();

        int max_value = *std::max_element(step.data.begin(), step.data.end());
        if (max_value == 0) max_value = 1;

        float x_step = region_size.x / (step.data.size() - 1);
        float base_y = cursor_pos.y + region_size.y * 0.7f;
        float amplitude_scale = region_size.y * 0.6f;

        // Drawing waveform
        for (size_t i = 0; i < step.data.size() - 1; ++i)
        {
            float x1 = cursor_pos.x + i * x_step;
            float y1 = base_y - (static_cast<float>(step.data[i]) / max_value) * amplitude_scale;

            float x2 = cursor_pos.x + (i + 1) * x_step;
            float y2 = base_y - (static_cast<float>(step.data[i + 1]) / max_value) * amplitude_scale;

            ImU32 color1 = get_enhanced_element_color(step, i);
            // ImU32 color2 = get_enhanced_element_color(step, i + 1);

            // Drawing gradient line
            draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color1, 3.0f);

            // Drawing data points
            if (i == step.visualization.highlighted_index ||
                i == step.visualization.compared_index)
            {
                draw_list->AddCircleFilled(
                    ImVec2(x1, y1), 
                    6.0f, 
                    ImColor(255, 255, 255, 255)
                );

                // Value label
                std::string value_str = std::to_string(step.data[i]);
                ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
                draw_list->AddText(ImVec2(x1 - text_size.x * 0.5f, y1 - 20),
                                  ImColor(255, 255, 255, 255), value_str.c_str());
            }
        }

        // Drawing baseline
        draw_list->AddLine(
            ImVec2(cursor_pos.x, base_y),
            ImVec2(cursor_pos.x + region_size.x, base_y),
            ImColor(100, 100, 100, 150), 1.0f
        );

        ImGui::EndChild();
    }

    void ArrayBasedVisualizer::render_heatmap_visualization(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild(
            "HeatmapVisualization", 
            ImVec2(0, 300), 
            true
        );

        if (step.data.empty())
        {
            ImGui::Text("No data to visualize");
            ImGui::EndChild();
            return;
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 region_size = ImGui::GetContentRegionAvail();

        int max_value = *std::max_element(step.data.begin(), step.data.end());
        if (max_value == 0) max_value = 1;

        float cell_size = std::min(region_size.x / step.data.size(), 40.0f);
        float grid_height = cell_size * step.data.size();
        float start_y = cursor_pos.y + (region_size.y - grid_height) * 0.5f;

        // Drawing heatmap grid
        for (size_t i = 0; i < step.data.size(); ++i)
        {
            for (size_t j = 0; j < step.data.size(); ++j)
            {
                float value_ratio = static_cast<float>(step.data[i]) / max_value;
                ImVec2 cell_min(cursor_pos.x + j * cell_size, start_y + i * cell_size);
                ImVec2 cell_max(cell_min.x + cell_size, cell_min.y + cell_size);

                // Coloring based on comparison relationship
                ImU32 cell_color;
                if (i == step.visualization.highlighted_index && 
                    j == step.visualization.compared_index) 
                {
                    cell_color = ImColor(255, 255, 0, 200); // Yellow for active comparison
                } 
                else if (i == j) 
                {
                    cell_color = value_to_heatmap_color(value_ratio); // Diagonal - value heat
                } 
                else if (step.data[i] > step.data[j]) 
                {
                    cell_color = ImColor(255, 50, 50, 80); // Red for greater than
                } 
                else 
                {
                    cell_color = ImColor(50, 150, 255, 60); // Blue for less than
                }

                draw_list->AddRectFilled(
                    cell_min, 
                    cell_max, 
                    cell_color
                );
                draw_list->AddRect(
                    cell_min, 
                    cell_max, 
                    ImColor(255, 255, 255, 30)
                );

                // Value on diagonal
                if (i == j && cell_size > 25) 
                {
                    std::string value_str = std::to_string(step.data[i]);
                    ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
                    ImVec2 text_pos(
                        cell_min.x + (cell_size - text_size.x) * 0.5f,
                        cell_min.y + (cell_size - text_size.y) * 0.5f
                    );
                    draw_list->AddText(
                        text_pos, 
                        ImColor(255, 255, 255, 255), 
                        value_str.c_str()
                    );
                }
            }
        }

        ImGui::EndChild();
    }

    ImU32 ArrayBasedVisualizer::value_to_heatmap_color(
        float ratio)
    {
        // Blue (cool) to Red (hot) heatmap
        int r = static_cast<int>(255 * ratio);
        int g = static_cast<int>(128 * (1.0f - std::abs(ratio - 0.5f) * 2.0f));
        int b = static_cast<int>(255 * (1.0f - ratio));
        return ImColor(r, g, b, 200);
    }

    void ArrayBasedVisualizer::render_particle_visualization(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild(
            "ParticleVisualization", 
            ImVec2(0, 400), 
            true
        );

        if (step.data.empty())
        {
            ImGui::Text("No data to visualize");
            ImGui::EndChild();
            return;
        }

        // Ensuring particles are synchronized with current data
        if (m_particles.size() != step.data.size()) 
        {
            initialize_particles(step.data);
        }

        // Update particle values from current step data
        for (size_t i = 0; i < m_particles.size(); ++i) 
        {
            m_particles[i].value = step.data[i];
        }

        update_particles(step);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 region_size = ImGui::GetContentRegionAvail();

        // Draw particle trails and connections first
        draw_particle_connections(draw_list, cursor_pos, region_size, step);

        // Draw particles on top
        for (size_t i = 0; i < m_particles.size(); ++i)
        {
            const auto& particle = m_particles[i];
            ImVec2 screen_pos(
                cursor_pos.x + particle.position.x * region_size.x,
                cursor_pos.y + particle.position.y * region_size.y
            );

            ImU32 particle_color = get_particle_color(step, i);

            // Draw particle with glow
            draw_list->AddCircleFilled(screen_pos, particle.size, particle_color);

            // Outer glow
            float pulse = (sin(static_cast<float>(ImGui::GetTime()) * 6.0f + i * 0.5f) + 1.0f) * 0.3f;
            draw_list->AddCircle(
                screen_pos, 
                particle.size + 3 + pulse * 2,
                ImColor(255, 255, 255, 80), 0, 2.0f
            );

            // Value label
            std::string value_str = std::to_string(particle.value);
            ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
            draw_list->AddText(
                ImVec2(screen_pos.x - text_size.x * 0.5f, screen_pos.y - text_size.y * 0.5f),
                ImColor(255, 255, 255, 255),
                value_str.c_str()
            );

            // Index label below
            std::string index_str = "[" + std::to_string(i) + "]";
            ImVec2 index_size = ImGui::CalcTextSize(index_str.c_str());
            draw_list->AddText(
                ImVec2(screen_pos.x - index_size.x * 0.5f, screen_pos.y + particle.size + 5),
                ImColor(180, 180, 180, 200),
                index_str.c_str()
            );
        }

        ImGui::EndChild();
    }

    void ArrayBasedVisualizer::initialize_particles(
        const std::vector<int>& data)
    {
	    m_particles.resize(data.size());
	    for (size_t i = 0; i < data.size(); ++i)
	    {
	        // Target x position based on sorted order (initially in input order)
	        float target_x = static_cast<float>(i) / static_cast<float>(data.size() - 1);
	        m_particles[i] = {
	            ImVec2(target_x, 0.5f),           // Start at target position
                ImVec2(0, 0),                     // Zero initial velocity
                ImVec2(target_x, 0.5f),           // Target position (will be updated)
                data[i],                          // value
                12.0f + static_cast<float>(data[i]) * 0.1f,          // size based on value
                true                              // is_active
            };
	    }
    }

    void ArrayBasedVisualizer::update_particles(
        const AlgorithmStep& step)
    {
        double current_time = ImGui::GetTime();
        double delta_time = current_time - m_last_update_time;
        m_last_update_time = current_time;

        // Updating target positions based on current array order
        for (size_t i = 0; i < m_particles.size(); ++i)
        {
            auto& particle = m_particles[i];

            // Finding what position this value should be in based on current array order
            // This ensures particles move to their correct sorted positions
            float target_x = static_cast<float>(i) / static_cast<float>(m_particles.size() - 1);
            particle.target_position.x = target_x;

            // Special vertical positioning for active comparisons
            if (i == step.visualization.highlighted_index) 
            {
                particle.target_position = ImVec2(target_x, 0.3f);
            } 
            else if (i == step.visualization.compared_index) 
            {
                particle.target_position = ImVec2(target_x, 0.7f);
            } 
            else 
            {
                particle.target_position = ImVec2(target_x, 0.5f);
            }

            // Smooth movement with physics
            ImVec2 direction = ImVec2(
                particle.target_position.x - particle.position.x,
                particle.target_position.y - particle.position.y
            );

            // Apply smooth acceleration
            float acceleration = 8.0f;
            particle.velocity = ImVec2(
                particle.velocity.x * 0.85f + direction.x * acceleration * static_cast<float>(delta_time),
                particle.velocity.y * 0.85f + direction.y * acceleration * static_cast<float>(delta_time)
            );

            // Update position
            particle.position = ImVec2(
                particle.position.x + particle.velocity.x * static_cast<float>(delta_time),
                particle.position.y + particle.velocity.y * static_cast<float>(delta_time)
            );

            // Add slight bouncing effect when reaching target
            float distance_to_target = sqrtf(direction.x * direction.x + direction.y * direction.y);
            if (distance_to_target < 0.02f)
            {
                particle.velocity = ImVec2(particle.velocity.x * 0.5f, particle.velocity.y * 0.5f);
            }
        }
    }

    void ArrayBasedVisualizer::draw_particle_connections(
        ImDrawList* draw_list, 
        const ImVec2& cursor_pos,
        const ImVec2& region_size, 
        const AlgorithmStep& step)
    {
        // Draw connections between comparing particles
        if (step.visualization.highlighted_index != static_cast<size_t>(-1) &&
            step.visualization.compared_index < m_particles.size())
        {
            const auto& p1 = m_particles[*step.visualization.highlighted_index];
            const auto& p2 = m_particles[*step.visualization.compared_index];

            ImVec2 screen_pos1(
                cursor_pos.x + p1.position.x * region_size.x,
                cursor_pos.y + p1.position.y * region_size.y
            );
            ImVec2 screen_pos2(
                cursor_pos.x + p2.position.x * region_size.x,
                cursor_pos.y + p2.position.y * region_size.y
            );

            // Animated connection line
            float pulse = (sin(static_cast<float>(ImGui::GetTime()) * 10.0f) + 1.0f) * 0.5f;
            ImU32 line_color = ImColor(255, 255, 255, static_cast<int>(150 + 100 * pulse));

            draw_list->AddLine(screen_pos1, screen_pos2, line_color, 3.0f);

            // Comparison indicator
            ImVec2 mid_point(
                (screen_pos1.x + screen_pos2.x) * 0.5f,
                (screen_pos1.y + screen_pos2.y) * 0.5f
            );

            std::string op = step.data[*step.visualization.highlighted_index] >
                           step.data[*step.visualization.compared_index] ? ">" : "<";
            ImVec2 op_size = ImGui::CalcTextSize(op.c_str());
            draw_list->AddText(
                ImVec2(mid_point.x - op_size.x * 0.5f, mid_point.y - op_size.y * 0.5f),
                ImColor(255, 255, 0, 255),
                op.c_str()
            );
        }

        // Drawing subtle trails between adjacent particles
        for (size_t i = 0; i < m_particles.size() - 1; ++i)
        {
            const auto& p1 = m_particles[i];
            const auto& p2 = m_particles[i + 1];

            ImVec2 screen_pos1(
                cursor_pos.x + p1.position.x * region_size.x,
                cursor_pos.y + p1.position.y * region_size.y
            );
            ImVec2 screen_pos2(
                cursor_pos.x + p2.position.x * region_size.x,
                cursor_pos.y + p2.position.y * region_size.y
            );

            draw_list->AddLine(
                screen_pos1, 
                screen_pos2, 
                ImColor(100, 100, 150, 40), 
                1.0f
            );
        }
    }

    ImU32 ArrayBasedVisualizer::get_particle_color(
        const AlgorithmStep& step, 
        size_t index)
    {
        float value_ratio = static_cast<float>(step.data[index]) /
                           *std::max_element(step.data.begin(), step.data.end());

        if (index == step.visualization.highlighted_index) 
        {
            return ImColor(255, 200, 50, 255); // Gold - active
        } 
        else if (index == step.visualization.compared_index) 
        {
            return ImColor(50, 220, 120, 255); // Green - comparison
        } 
        else 
        {
            // Gradient from blue to red based on value
            return ImColor(
                static_cast<int>(255 * value_ratio),
                static_cast<int>(150 * (1.0f - std::abs(value_ratio - 0.5f) * 2.0f)),
                static_cast<int>(255 * (1.0f - value_ratio)),
                255
            );
        }
    }

    void ArrayBasedVisualizer::render_tree_visualization(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild(
            "TreeVisualization", 
            ImVec2(0, 500), 
            true
        );

        if (step.data.empty())
        {
            ImGui::Text("No data to visualize");
            ImGui::EndChild();
            return;
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 region_size = ImGui::GetContentRegionAvail();

        // Build a binary tree representation for visualization
        std::vector<TreeNode> tree_nodes = build_binary_tree(step.data);
        std::vector<ImVec2> node_positions = calculate_tree_positions(tree_nodes.size(), region_size);

        if (node_positions.empty()) 
        {
            ImGui::Text("Error calculating tree layout");
            ImGui::EndChild();
            return;
        }

        int max_value = *std::max_element(step.data.begin(), step.data.end());
        if (max_value == 0) max_value = 1;

        // Draw tree connections first (so they appear behind nodes)
        for (size_t i = 0; i < tree_nodes.size(); ++i)
        {
            const auto& node = tree_nodes[i];

            // Draw connection to left child
            if (node.left_child < tree_nodes.size() && 
                node.left_child < node_positions.size()) 
            {
                draw_list->AddLine(
                    node_positions[i],
                    node_positions[node.left_child],
                    ImColor(100, 100, 150, 120),
                    2.0f
                );
            }

            // Draw connection to right child
            if (node.right_child < tree_nodes.size() && 
                node.right_child < node_positions.size()) 
            {
                draw_list->AddLine(
                    node_positions[i],
                    node_positions[node.right_child],
                    ImColor(100, 100, 150, 120),
                    2.0f
                );
            }
        }

        // Draw nodes on top of connections
        for (size_t i = 0; i < tree_nodes.size() && 
            i < node_positions.size(); 
            ++i)
        {
            const auto& node = tree_nodes[i];
            float value_ratio = static_cast<float>(node.value) / max_value;
            ImU32 node_color = get_tree_node_color(step, i, value_ratio);
            float node_radius = 20.0f + value_ratio * 10.0f;

            // Convert to screen coordinates
            ImVec2 screen_pos(
                cursor_pos.x + node_positions[i].x, 
                cursor_pos.y + node_positions[i].y
            );

            // Draw node with gradient effect
            draw_list->AddCircleFilled(
                screen_pos, 
                node_radius, 
                node_color
            );
            draw_list->AddCircle(
                screen_pos, 
                node_radius, 
                ImColor(255, 255, 255, 200), 
                0, 
                2.0f
            );

            // Draw value
            std::string value_str = std::to_string(node.value);
            ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
            draw_list->AddText(
                ImVec2(screen_pos.x - text_size.x * 0.5f,
                      screen_pos.y - text_size.y * 0.5f),
                ImColor(255, 255, 255, 255),
                value_str.c_str()
            );

            // Draw array index
            std::string index_str = "[" + std::to_string(i) + "]";
            ImVec2 index_size = ImGui::CalcTextSize(index_str.c_str());
            draw_list->AddText(
                ImVec2(
                    screen_pos.x - index_size.x * 0.5f,
                    screen_pos.y + node_radius + 8
                ),
                ImColor(180, 180, 180, 200),
                index_str.c_str()
            );

            // Highlight active nodes with animation
            if (i == step.visualization.highlighted_index ||
                i == step.visualization.compared_index)
            {
                float pulse = (sin(static_cast<float>(ImGui::GetTime()) * 8.0f) + 1.0f) * 0.5f;
                ImU32 glow_color = ImColor(255, 255, 255, static_cast<int>(150 + 100 * pulse));

                draw_list->AddCircle(
                    screen_pos,
                    node_radius + 8 + pulse * 6,
                    glow_color,
                    0, 3.0f
                );
            }
        }

        // Draw comparison line if two nodes are being compared
        if (step.visualization.highlighted_index != static_cast<size_t>(-1) &&
            step.visualization.compared_index != static_cast<size_t>(-1) &&
            step.visualization.highlighted_index < node_positions.size() &&
            step.visualization.compared_index < node_positions.size())
        {
            ImVec2 pos1(
                cursor_pos.x + node_positions[*step.visualization.highlighted_index].x,
                cursor_pos.y + node_positions[*step.visualization.highlighted_index].y
            );
            ImVec2 pos2(
                cursor_pos.x + node_positions[*step.visualization.compared_index].x,
                cursor_pos.y + node_positions[*step.visualization.compared_index].y
            );

            // Drawing animated comparison line
            float pulse = (sin(static_cast<float>(ImGui::GetTime()) * 10.0f) + 1.0f) * 0.5f;
            draw_list->AddLine(
                pos1, 
                pos2, 
                ImColor(255, 255, 0, static_cast<int>(150 + 100 * pulse)), 
                3.0f
            );

            // Drawing comparison operator
            ImVec2 mid_point((pos1.x + pos2.x) * 0.5f, (pos1.y + pos2.y) * 0.5f);
            std::string comparison = step.data[*step.visualization.highlighted_index] >
                                   step.data[*step.visualization.compared_index] ? ">" : "<";
            ImVec2 text_size = ImGui::CalcTextSize(comparison.c_str());
            draw_list->AddText(
                ImVec2(mid_point.x - text_size.x * 0.5f, mid_point.y - text_size.y * 0.5f),
                ImColor(255, 255, 0, 255),
                comparison.c_str()
            );
        }

        // Drawing tree structure explanation at the bottom
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
        ImGui::TextColored(
            ImVec4(0.7f, 0.7f, 1.0f, 1.0f),
            "Binary Tree: Parent = i, Left = 2i+1, Right = 2i+2"
        );

        ImGui::EndChild();
    }

    std::vector<algorithms::ArrayBasedVisualizer::TreeNode> 
    ArrayBasedVisualizer::build_binary_tree(
        const std::vector<int>& data)
	{
	    std::vector<TreeNode> tree;
	    tree.reserve(data.size());

	    // Create tree nodes from array data
	    for (size_t i = 0; i < data.size(); ++i) {
	        TreeNode node(data[i]);

	        // Calculate child indices (binary heap layout)
	        node.left_child = 2 * i + 1;
	        node.right_child = 2 * i + 2;
	        node.parent = i > 0 ? (i - 1) / 2 : static_cast<size_t>(-1);

	        tree.push_back(node);
	    }

	    return tree;
	}


    std::vector<ImVec2> ArrayBasedVisualizer::calculate_tree_layout(
        size_t node_count, 
        const ImVec2& region_size)
    {
        std::vector<ImVec2> positions;
        positions.reserve(node_count);

        // Calculate tree depth
        int depth = static_cast<int>(std::log2(node_count)) + 1;

        for (size_t i = 0; i < node_count; ++i)
        {
            int level = static_cast<int>(std::log2(i + 1));
            int max_nodes_at_level = 1 << level;
            int position_in_level = static_cast<int>(i) + 1 - (1 << level);

            float x = static_cast<float>(position_in_level + 1) / (max_nodes_at_level + 1) * region_size.x;
            float y = static_cast<float>(level + 1) / (depth + 1) * region_size.y;

            positions.emplace_back(x, y);
        }

        return positions;
    }

    void ArrayBasedVisualizer::render_molecular_visualization(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild(
            "MolecularVisualization", 
            ImVec2(0, 350), 
            true
        );

        if (step.data.empty())
        {
            ImGui::Text("No data to visualize");
            ImGui::EndChild();
            return;
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 region_size = ImGui::GetContentRegionAvail();

        ImVec2 center(cursor_pos.x + region_size.x * 0.5f, cursor_pos.y + region_size.y * 0.5f);
        float max_radius = std::min(region_size.x, region_size.y) * 0.4f;

        int max_value = *std::max_element(step.data.begin(), step.data.end());
        if (max_value == 0) max_value = 1;

        // Draw molecular structure
        for (size_t i = 0; i < step.data.size(); ++i)
        {
            float angle = (static_cast<float>(i) / step.data.size()) * 2.0f * 3.14159f;
            float radius_ratio = static_cast<float>(step.data[i]) / max_value;
            float radius = 20.0f + radius_ratio * max_radius;

            ImVec2 position(
                center.x + cos(angle) * radius,
                center.y + sin(angle) * radius
            );

            // Atom color and size
            ImU32 atom_color = get_molecular_color(step, i, radius_ratio);
            float atom_size = 12.0f + radius_ratio * 8.0f;

            // Draw atom with electron rings
            draw_list->AddCircleFilled(
                position, 
                atom_size, 
                atom_color
            );

            // Electron rings
            for (int ring = 1; ring <= 3; ++ring)
            {
                float ring_radius = atom_size + ring * 8.0f;
                draw_list->AddCircle(position, ring_radius, ImColor(100, 100, 200, 50), 0, 1.0f);

                // Draw electrons on rings
                for (int electron = 0; electron < 4; ++electron)
                {
                    float electron_angle = angle + electron * 3.14159f / 2.0f;
                    ImVec2 electron_pos(
                        position.x + cos(electron_angle) * ring_radius,
                        position.y + sin(electron_angle) * ring_radius
                    );
                    draw_list->AddCircleFilled(
                        electron_pos, 
                        2.0f, 
                        ImColor(200, 200, 255, 200)
                    );
                }
            }

            // Value label
            std::string value_str = std::to_string(step.data[i]);
            ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
            draw_list->AddText(
                ImVec2(position.x - text_size.x * 0.5f, position.y - atom_size - 15),
                ImColor(255, 255, 255, 255),
                value_str.c_str()
            );

            // Drawing bonds between comparing elements
            const auto& highlighted_index = step.visualization.highlighted_index;
            const auto& compared_index = step.visualization.compared_index;

            if (!compared_index || !highlighted_index) return;

            if (i == *highlighted_index &&
                *compared_index < step.data.size())
            {
                float other_angle = (static_cast<float>(*compared_index) / step.data.size()) * 2.0f * 3.14159f;
                float other_radius_ratio = static_cast<float>(step.data[*compared_index]) / max_value;
                float other_radius = 20.0f + other_radius_ratio * max_radius;

                ImVec2 other_position(
                    center.x + cos(other_angle) * other_radius,
                    center.y + sin(other_angle) * other_radius
                );

                // Animated bond line
                const float pulse = (sin(static_cast<float>(ImGui::GetTime()) * 12.0f) + 1.0f) * 0.5f;
                const ImU32 bond_color = ImColor(255, 255, 0, static_cast<int>(150 + 100 * pulse));

                draw_list->AddLine(
                    position, 
                    other_position, 
                    bond_color, 
                    3.0f
                );
            }
        }

        ImGui::EndChild();
    }

    ImU32 ArrayBasedVisualizer::get_molecular_color(
        const AlgorithmStep& step, 
        size_t index, 
        float radius_ratio)
    {
        // Periodic table inspired colors
        if (index == step.visualization.highlighted_index) 
        {
            return ImColor(255, 215, 0, 255); // Gold
        } 
        else if (index == step.visualization.compared_index) 
        {
            return ImColor(50, 205, 50, 255); // Lime green
        }

        // Color based on value with chemical element inspiration
        if (radius_ratio < 0.25f) return ImColor(70, 130, 180, 255);   // Steel blue
        if (radius_ratio < 0.5f) return ImColor(34, 139, 34, 255);     // Forest green
        if (radius_ratio < 0.75f) return ImColor(255, 140, 0, 255);    // Dark orange
        return ImColor(178, 34, 34, 255); // Fire brick red
    }

    void ArrayBasedVisualizer::render_neural_network_visualization(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild(
            "NeuralNetworkVisualization", 
            ImVec2(0, 400), 
            true
        );

        if (step.data.empty())
        {
            ImGui::Text("No data to visualize");
            ImGui::EndChild();
            return;
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        // ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 region_size = ImGui::GetContentRegionAvail();

        // Create neural network layers
        int input_size = static_cast<int>(step.data.size());
        int hidden_size = std::min(8, input_size);
        int output_size = input_size;

        // Calculate node positions for each layer
        auto input_nodes = calculate_layer_positions(input_size, region_size, 0.1f);
        auto hidden_nodes = calculate_layer_positions(hidden_size, region_size, 0.5f);
        auto output_nodes = calculate_layer_positions(output_size, region_size, 0.9f);

        // Draw connections with weights based on comparisons
        for (size_t i = 0; i < input_nodes.size(); ++i)
        {
            for (size_t j = 0; j < hidden_nodes.size(); ++j)
            {
                float weight = static_cast<float>(step.data[i]) /
                              *std::max_element(step.data.begin(), step.data.end());

                ImU32 connection_color = get_neural_connection_color(weight);
                float line_width = 1.0f + weight * 3.0f;

                draw_list->AddLine(input_nodes[i], hidden_nodes[j], connection_color, line_width);
            }
        }

        for (size_t i = 0; i < hidden_nodes.size(); ++i)
        {
            for (size_t j = 0; j < output_nodes.size(); ++j)
            {
                float weight = 1.0f - (static_cast<float>(i) / hidden_nodes.size());
                ImU32 connection_color = get_neural_connection_color(weight);
                float line_width = 1.0f + weight * 3.0f;

                draw_list->AddLine(hidden_nodes[i], output_nodes[j], connection_color, line_width);
            }
        }

        // Draw nodes
        draw_neural_nodes(draw_list, input_nodes, step, "Input");
        draw_neural_nodes(draw_list, hidden_nodes, step, "Hidden");
        draw_neural_nodes(draw_list, output_nodes, step, "Output");

        ImGui::EndChild();
    }

    std::vector<ImVec2> ArrayBasedVisualizer::calculate_layer_positions(
        int node_count,
        const ImVec2& region_size, 
        float x_ratio)
    {
        std::vector<ImVec2> positions;
        positions.reserve(static_cast<size_t>(node_count));

        for (int i = 0; i < node_count; ++i)
        {
            float x = region_size.x * x_ratio;
            float y = (static_cast<float>(i) + 0.5f) / node_count * region_size.y;
            positions.emplace_back(x, y);
        }

        return positions;
    }

    void ArrayBasedVisualizer::draw_neural_nodes(
        ImDrawList* draw_list, 
        const std::vector<ImVec2>& nodes,
        const AlgorithmStep& step, 
        const std::string& layer_name)
    {
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

        for (size_t i = 0; i < nodes.size(); ++i)
        {
            ImVec2 screen_pos(cursor_pos.x + nodes[i].x, cursor_pos.y + nodes[i].y);
            float activation = static_cast<float>(step.data[i % step.data.size()]) /
                              *std::max_element(step.data.begin(), step.data.end());

            // Node color based on activation
            ImU32 node_color = ImColor(
                static_cast<int>(255 * activation),
                static_cast<int>(150 * (1.0f - activation)),
                static_cast<int>(100 + 155 * activation),
                255
            );

            float node_size = 8.0f + activation * 12.0f;

            // Draw node with glow
            draw_list->AddCircleFilled(
                screen_pos, 
                node_size, 
                node_color
            );
            draw_list->AddCircle(
                screen_pos, 
                node_size + 2, 
                ImColor(255, 255, 255, 100), 
                0, 
                1.5f
            );

            // Activation indicator
            if (activation > 0.7f) 
            {
                draw_list->AddCircle(
                    screen_pos, 
                    node_size + 4, 
                    ImColor(255, 255, 0, 150), 
                    0, 
                    1.0f
                );
            }
        }
    }

    ImU32 ArrayBasedVisualizer::get_neural_connection_color(
        float weight)
    {
        if (weight > 0.7f) return ImColor(50, 200, 50, 150);   // Strong - green
        if (weight > 0.3f) return ImColor(200, 200, 50, 120);  // Medium - yellow
        return ImColor(200, 50, 50, 80);                      // Weak - red
    }



    std::vector<ImVec2> ArrayBasedVisualizer::calculate_node_positions(
        size_t count, 
        const ImVec2& region_size)
    {
        std::vector<ImVec2> positions;
        positions.reserve(count);

        // Simple grid layout
        int cols = static_cast<int>(std::ceil(std::sqrt(count)));
        int rows = static_cast<int>(std::ceil(static_cast<float>(count) / cols));

        float cell_width = region_size.x / cols;
        float cell_height = region_size.y / rows;

        for (size_t i = 0; i < count; ++i)
        {
            int row = static_cast<int>(i) / cols;
            int col = static_cast<int>(i) % cols;

            float x = col * cell_width + cell_width * 0.5f;
            float y = row * cell_height + cell_height * 0.5f;

            positions.emplace_back(x, y);
        }

        return positions;
    }

    void ArrayBasedVisualizer::render_searching_visualization(
        const AlgorithmStep& step) 
    {
        ImGui::BeginChild(
            "SearchVisualization", 
            ImVec2(0, 120), 
            true
        );
        
        float available_width = ImGui::GetContentRegionAvail().x;
        float element_width = std::max(40.0f, available_width / step.data.size() - 4.0f);
        element_width = std::min(element_width, 80.0f);
        
        for (size_t i = 0; i < step.data.size(); ++i)
        {
            if (i > 0) ImGui::SameLine(0, 4.0f);
            
            ImVec4 color = ImVec4(0.3f, 0.3f, 0.3f, 1.0f); // Default gray
            std::string state = "Unexamined";
            
            if (i == step.visualization.highlighted_index) 
            {
                color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red - current search position
                state = "Currently Examining";
            } 
            else if (i == step.visualization.compared_index) 
            {
                color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green - found target
                state = "Target Found!";
            } 
            else if (i < step.visualization.highlighted_index) 
            {
                color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray - already examined
                state = "Already Examined";
            }
            
            ImGui::PushStyleColor(ImGuiCol_Button, color);
            ImGui::PushStyleColor(
                ImGuiCol_ButtonHovered, 
                ImVec4(
                    color.x * 1.2f, 
                    color.y * 1.2f,
                    color.z * 1.2f, 
                    1.0f
                )
            );
            
            ImGui::Button(
                std::to_string(step.data[i]).c_str(), 
                ImVec2(element_width, 60)
            );
            
            // Index below
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() - element_width);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[%zu]", i);
            
            ImGui::PopStyleColor(2);
            
            // Enhanced tooltip
            if (ImGui::IsItemHovered()) 
            {
                ImGui::BeginTooltip();
                ImGui::Text("Index: %zu", i);
                ImGui::Text("Value: %d", step.data[i]);
                ImGui::TextColored(
                    ImVec4(1.0f, 1.0f, 0.0f, 1.0f), 
                    "State: %s", 
                    state.c_str()
                );
                ImGui::EndTooltip();
            }
        }
        
        ImGui::EndChild();
    }

    ImU32 ArrayBasedVisualizer::get_element_color(
        const AlgorithmStep& step, 
        size_t index)
    {
        // Sophisticated color coding based on algorithm state
        if (index == step.visualization.highlighted_index) 
            return ImColor(255, 165, 0, 255);    // Orange - pivot/key element
            
        if (index == step.visualization.compared_index) 
            return ImColor(0, 255, 0, 255);      // Green - being compared
            
        if (!step.visualization.additional_highlights.empty() &&
            std::find(step.visualization.additional_highlights.begin(),
                     step.visualization.additional_highlights.end(), index) != 
            step.visualization.additional_highlights.end())
            return ImColor(128, 0, 128, 255);    // Purple - partition/swap area
            
        // Default color with gradient based on value
        if (!step.data.empty()) 
        {
            float value_ratio = static_cast<float>(step.data[index]) / 
                               *std::max_element(step.data.begin(), step.data.end());

            return ImColor(
                    static_cast<int>(30  + value_ratio * 100.0f),
                    static_cast<int>(100 + value_ratio * 100.0f),
                    200,
                    255);
        }
        
        return ImColor(100, 100, 100, 255);      // Gray fallback
    }

    ImU32 ArrayBasedVisualizer::get_tree_node_color(
        const AlgorithmStep& step, 
        size_t index, 
        float value_ratio)
	{
	    if (index == step.visualization.highlighted_index)
	        return ImColor(255, 200, 50, 255);    // Gold - active element

	    if (index == step.visualization.compared_index)
	        return ImColor(50, 200, 100, 255);    // Green - comparison element

	    // Check if this node is in sorted section
	    if (!step.visualization.additional_highlights.empty() &&
            std::find(step.visualization.additional_highlights.begin(),
                     step.visualization.additional_highlights.end(), index) !=
            step.visualization.additional_highlights.end())
	        return ImColor(180, 80, 220, 255);    // Purple - sorted section

	    // Default color based on value (blue to red gradient)
	    return ImColor(
            static_cast<int>(80 + 175 * value_ratio),    // Red component
            static_cast<int>(120 + 135 * (1.0f - value_ratio)), // Green component
            200,                                         // Blue component
            255                                          // Alpha
        );
	}

    std::vector<ImVec2> ArrayBasedVisualizer::calculate_tree_positions(
        size_t node_count, 
        const ImVec2& region_size)
	{
	    std::vector<ImVec2> positions;
	    if (node_count == 0) return positions;

	    positions.reserve(node_count);

	    // Calculate the depth of the tree
	    int depth = 0;
	    size_t max_nodes_at_depth = 1;
	    size_t total_nodes = 0;

	    while (total_nodes < node_count) 
        {
	        depth++;
	        total_nodes += max_nodes_at_depth;
	        max_nodes_at_depth *= 2;
	    }

	    // Calculate positions for each level
	    size_t current_index = 0;
	    for (int level = 0; level < depth && current_index < node_count; ++level) 
        {
	        int nodes_in_level = 1 << level; // 2^level
	        float level_height = region_size.y / (depth + 1);
	        float y = (level + 1) * level_height;

	        for (int i = 0; i < nodes_in_level && current_index < node_count; ++i) 
            {
	            float x = (static_cast<float>(i) + 0.5f) / nodes_in_level * region_size.x;
	            positions.emplace_back(x, y);
	            current_index++;
	        }
	    }

	    return positions;
	}

    std::vector<ImVec2> ArrayBasedVisualizer::calculate_network_positions(
        const std::vector<int>& data, 
        const ImVec2& region_size)
	{
	    std::vector<ImVec2> positions;
	    positions.reserve(data.size());

	    // Create positions that reflect the current order in the array
	    for (size_t i = 0; i < data.size(); ++i)
	    {
	        // Position nodes in a line that curves slightly for better visualization
	        float x_ratio = static_cast<float>(i) / (data.size() - 1);
	        float x = x_ratio * region_size.x * 0.8f + region_size.x * 0.1f;

	        // Add slight curve to make the network more visible
	        float curve = sin(x_ratio * 3.14159f) * 0.2f;
	        float y = (0.5f + curve) * region_size.y;

	        positions.emplace_back(x, y);
	    }

	    return positions;
	}

    ImU32 ArrayBasedVisualizer::get_network_node_color(
        const AlgorithmStep& step, 
        size_t index, 
        float value_ratio)
	{
	    if (index == step.visualization.highlighted_index)
	        return ImColor(255, 200, 50, 255);    // Gold - active element

	    if (index == step.visualization.compared_index)
	        return ImColor(50, 200, 100, 255);    // Green - comparison element

	    // Color based on value with smooth gradient
	    return ImColor(
            static_cast<int>(100 + 155 * value_ratio),
            static_cast<int>(100 + 155 * (1.0f - value_ratio)),
            200,
            255
        );
	}

    VisualizationType ArrayBasedVisualizer::get_visualization_type() const
    {
	    return VisualizationType::ARRAY_BASED;
	}

    bool ArrayBasedVisualizer::supports_algorithm(
        const AlgorithmType& type
    ) const
    {
	    const auto category = algorithm_category(type);
	    return category == AlgorithmCategory::SORTING ||
               category == AlgorithmCategory::SEARCHING;
	}

}
