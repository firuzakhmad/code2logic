#include "array_based_visualizer.hpp"
#include "core/utils/logger/logger.hpp"

#include <cmath>
#include <algorithm>

#include "core/utils/utils.hpp"

namespace c2l::algorithms
{
    ArrayBasedVisualizer::ArrayBasedVisualizer(
        const VisualizationConfig& visualization_config)
            : m_visualization_config{visualization_config}
    {}

	void ArrayBasedVisualizer::initialize(
        const ISimpleAlgorithm* execution,
        const IAlgorithmMetadata* metadata)
	{
        m_execution = execution;
        m_metadata = metadata;
        m_has_initialized_particles = false;
        m_animation_time = 0.0f;

        if (m_metadata)
        {
            m_current_style = m_metadata->get_visualization_config().default_style;
        }

        LOG_DEBUG(
            "ArrayBasedVisualizer initialized for {}",
            m_metadata ? m_metadata->get_display_name() : "Unknown"
        );
    }

    void ArrayBasedVisualizer::update(double delta_time) 
    {
        m_animation_time += static_cast<float>(delta_time);

        // Updating particles if they are being used
        if (m_current_style == VisualizationStyle::PARTICLE_SYSTEM &&
            m_execution && 
            !m_particles.empty())
        {
            const auto step = m_execution->get_current_step();
            if (!step.data.empty())
            {
                update_particles(step, static_cast<float>(delta_time));
            }
        }

    }

    void ArrayBasedVisualizer::render()
    {
    	if (!m_execution) return;

        const auto current_step = m_execution->get_current_step();

        if (current_step.data.empty())
        {
            ImGui::TextColored(
                ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 
                "No data to visualize. Please initialize the algorithm first."
            );
            return;
        }

    	render_array_visualization(current_step);
    }

    void ArrayBasedVisualizer::render_array_visualization(
        const AlgorithmStep& step) 
    {
        // Style selector
        ImGui::SetNextItemWidth(180);
        int current_style_int = static_cast<int>(m_current_style);
        if (ImGui::Combo("Visualization Style", &current_style_int, 
            STYLE_NAMES, static_cast<int>(VisualizationStyle::COUNT)))
        {
            m_current_style = static_cast<VisualizationStyle>(current_style_int);
            
        }
        
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Different visual representations of the sorting process");
            ImGui::EndTooltip();
        }
        
        ImGui::Separator();
        
        // Dispatching to appropriate visualization
        if (m_metadata && m_metadata->get_category() == AlgorithmCategory::SORTING)
        {
            switch (m_current_style)
            {
                case VisualizationStyle::CLASSIC_BARS:
                    render_classic_bars(step);
                    break;
                case VisualizationStyle::ENHANCED_BARS:
                    render_enhanced_bars(step);
                    break;
                case VisualizationStyle::DOTS:
                    render_dots(step);
                    break;
                case VisualizationStyle::CIRCULAR:
                    render_circular(step);
                    break;
                case VisualizationStyle::NETWORK:
                    render_network(step);
                    break;
                case VisualizationStyle::WAVEFORM:
                    render_waveform(step);
                    break;
                case VisualizationStyle::HEATMAP:
                    render_heatmap(step);
                    break;
                case VisualizationStyle::PARTICLE_SYSTEM:
                    render_particle_system(step);
                    break;
                case VisualizationStyle::TREE_VIEW:
                    render_tree_view(step);
                    break;
                case VisualizationStyle::MOLECULAR:
                    render_molecular(step);
                    break;
                case VisualizationStyle::NEURAL_NETWORK:
                    render_neural_network(step);
                    break;
                
                default:
                    render_enhanced_bars(step);
                    break;
            }
        }
    }

    void ArrayBasedVisualizer::render_classic_bars(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild(
            "ClassicBars", 
            ImVec2(0, 300), 
            true, 
            ImGuiWindowFlags_HorizontalScrollbar
        );

        const float available_width = ImGui::GetContentRegionAvail().x;
        const float bar_width = calculate_bar_width(
            step.data.size(), 
            available_width
        );
        const float max_bar_height = calculate_max_bar_height(
            ImGui::GetContentRegionAvail()
        );  

        const int max_value = *std::max_element(
            step.data.begin(), 
            step.data.end()
        );
        if (max_value == 0) 
            return;


        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        const float start_y = cursor_pos.y + max_bar_height + 20;
        
        for (size_t i = 0; i < step.data.size(); ++i)
        {
            const float bar_height = (static_cast<float>(step.data[i]) / max_value) * max_bar_height;
            const float x = cursor_pos.x + i * (bar_width + 2);
            
            const ImVec2 bar_min(x, start_y - bar_height);
            const ImVec2 bar_max(x + bar_width, start_y);
            
            // Drawing bar
            const ImU32 color = get_element_color(step, i);
            draw_list->AddRectFilled(bar_min, bar_max, color);
            draw_list->AddRect(bar_min, bar_max, ImColor(200, 200, 200, 200));
            
            // Drawing value label (only for reasonable array sizes)
            if (step.data.size() <= 30 && bar_height > 20)
            {
                const std::string value_str = std::to_string(step.data[i]);
                const ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
                const float text_x = x + (bar_width - text_size.x) * 0.5f;
                const float text_y = bar_min.y - text_size.y - 2;
                
                if (text_y >= cursor_pos.y)
                {
                    draw_list->AddText(
                        ImVec2(text_x, text_y), 
                        ImColor(255, 255, 255, 255), value_str.c_str()
                    );
                }
            }
            
            // Drawing index label
            const std::string index_str = std::to_string(i);
            const ImVec2 index_size = ImGui::CalcTextSize(index_str.c_str());
            const float index_x = x + (bar_width - index_size.x) * 0.5f;
            draw_list->AddText(
                ImVec2(index_x, start_y + 5), 
                ImColor(180, 180, 180, 200), 
                index_str.c_str()
            );
        }

        ImGui::EndChild();
    }

    void ArrayBasedVisualizer::render_enhanced_bars(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild(
            "EnhancedBars", 
            ImVec2(0, 350),
            true,
            ImGuiWindowFlags_HorizontalScrollbar
        );

        const float available_width = ImGui::GetContentRegionAvail().x;
        const float bar_width = calculate_bar_width(
            step.data.size(), 
            available_width
        );
        const float max_bar_height = calculate_max_bar_height(
            ImGui::GetContentRegionAvail()
        );
        
        const int max_value = *std::max_element(
            step.data.begin(), 
            step.data.end()
        );
        if (max_value == 0) return;
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        const float start_y = cursor_pos.y + max_bar_height + 30;
        const float spacing = 2.0f;
        
        // Drawing gradient background
        draw_list->AddRectFilledMultiColor(
            cursor_pos,
            ImVec2(
                cursor_pos.x + available_width, 
                cursor_pos.y + max_bar_height + 40
            ),
            ImColor(25, 25, 45, 255),
            ImColor(25, 25, 45, 255),
            ImColor(15, 15, 25, 255),
            ImColor(15, 15, 25, 255)
        );
        
        for (size_t i = 0; i < step.data.size(); ++i)
        {
            const float bar_height = (static_cast<float>(step.data[i]) / max_value) * max_bar_height;
            const float x = cursor_pos.x + i * (bar_width + spacing);
            
            const ImVec2 bar_min(x, start_y - bar_height);
            const ImVec2 bar_max(x + bar_width, start_y);
            
            // Getting enhanced color with gradient
            const ImU32 base_color = get_enhanced_element_color(step, i);
            const ImU32 top_color = apply_color_variation(base_color, 1.3f);
            
            // Draw bar with vertical gradient
            draw_list->AddRectFilledMultiColor(
                bar_min, bar_max,
                top_color, top_color, base_color, base_color
            );
            
            // Adding border
            draw_list->AddRect(
                bar_min, 
                bar_max, 
                ImColor(255, 255, 255, 80), 
                0, 
                0, 
                1.0f
            );
            
            // Glowing effect for active elements
            const bool is_active = (step.visualization.highlighted_index.has_value() && 
                                    i == step.visualization.highlighted_index.value()) ||
                                   (step.visualization.compared_index.has_value() && 
                                    i == step.visualization.compared_index.value());
            
            if (is_active)
            {
                const float pulse = (std::sin(m_animation_time * 8.0f) + 1.0f) * 0.5f;
                const ImVec2 glow_min(bar_min.x - 2, bar_min.y - 2);
                const ImVec2 glow_max(bar_max.x + 2, bar_max.y + 2);
                draw_list->AddRect(
                    glow_min, 
                    glow_max, 
                    ImColor(
                        255, 
                        255, 
                        255, 
                        static_cast<int>(150 + 100 * pulse)
                    ), 
                    0, 
                    0, 
                    2.0f
                );
            }
            
            // Drawing value label
            if (step.data.size() <= 40 && bar_height > 25)
            {
                const std::string value_str = std::to_string(step.data[i]);
                const ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
                const float text_x = x + (bar_width - text_size.x) * 0.5f;
                const float text_y = bar_min.y - text_size.y - 2;
                
                if (text_y >= cursor_pos.y)
                {
                    draw_list->AddText(
                        ImVec2(text_x, text_y),
                        ImColor(255, 255, 255, 255), 
                        value_str.c_str()
                    );
                }
            }
            
            // Drawing index label
            const std::string index_str = std::to_string(i);
            const ImVec2 index_size = ImGui::CalcTextSize(index_str.c_str());
            const float index_x = x + (bar_width - index_size.x) * 0.5f;
            draw_list->AddText(
                ImVec2(index_x, start_y + 8),
                ImColor(180, 180, 200, 200), 
                index_str.c_str()
            );
        }
        
        // Drawing comparison line if two elements are being compared
        if (step.visualization.highlighted_index.has_value() &&
            step.visualization.compared_index.has_value())
        {
            const size_t idx1 = step.visualization.highlighted_index.value();
            const size_t idx2 = step.visualization.compared_index.value();
            
            if (idx1 < step.data.size() && idx2 < step.data.size())
            {
                const float x1 = cursor_pos.x + idx1 * (bar_width + spacing) + bar_width * 0.5f;
                const float x2 = cursor_pos.x + idx2 * (bar_width + spacing) + bar_width * 0.5f;
                
                const float height1 = (static_cast<float>(step.data[idx1]) / max_value) * max_bar_height;
                const float height2 = (static_cast<float>(step.data[idx2]) / max_value) * max_bar_height;
                
                const ImVec2 p1(x1, start_y - height1 - 10);
                const ImVec2 p2(x2, start_y - height2 - 10);
                
                draw_list->AddBezierCubic(
                    ImVec2(p1.x, p1.y - 10),
                    ImVec2(p1.x, p1.y - 25),
                    ImVec2(p2.x, p2.y - 25),
                    ImVec2(p2.x, p2.y - 10), 
                    ImColor(255, 200, 100, 200),
                    2.0f
                );
                
                draw_list->AddCircleFilled(
                    ImVec2(p1.x, p1.y - 10), 
                    4.0f, 
                    ImColor(255, 200, 100, 255)
                );
                draw_list->AddCircleFilled(
                    ImVec2(p2.x, p2.y - 10), 
                    4.0f, 
                    ImColor(255, 200, 100, 255)
                );
            }
        }


        ImGui::EndChild();
    }

    void ArrayBasedVisualizer::render_dots(
        const AlgorithmStep& step)
    {
        const int max_value = *std::max_element(
            step.data.begin(), step.data.end()
        );
        if (max_value == 0) 
            return;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        const ImVec2 region_size = ImGui::GetContentRegionAvail();

        const float dot_spacing = region_size.x / step.data.size();
        const float max_dot_radius = std::min(
            25.0f,
            region_size.y * 0.4f
        );
        const float center_y = cursor_pos.y + region_size.y * 0.5f;

        for (size_t i = 0; i < step.data.size(); ++i)
        {
            const float radius_ratio = static_cast<float>(step.data[i]) / max_value;
            const float dot_radius = std::max(5.0f, radius_ratio * max_dot_radius);

            const ImVec2 dot_center(
                cursor_pos.x + i * dot_spacing + dot_spacing * 0.5f, 
                center_y
            );

            const ImU32 color = get_enhanced_element_color(step, i);

            draw_list->AddCircleFilled(
                dot_center, 
                dot_radius, 
                color
            );
            draw_list->AddCircle(
                dot_center, 
                dot_radius, 
                ImColor(255, 255, 255, 200)
            );

            // Adding pulse effect for active dots
            const bool is_active = (step.visualization.highlighted_index.has_value() && 
                        i == step.visualization.highlighted_index.value()) ||
                       (step.visualization.compared_index.has_value() && 
                        i == step.visualization.compared_index.value());

            if (is_active)  
            {
                const float pulse = (std::sin(m_animation_time * 10.0f) + 1.0f) * 0.5f;
                draw_list->AddCircle(
                    dot_center, 
                    dot_radius + 3 + pulse * 4,
                    ImColor(255, 255, 255, 200), 
                    0, 
                    2.0f
                );
            }

            // Drawing value label inside larger dots
            if (dot_radius > 10)
            {
                const std::string value_str = std::to_string(step.data[i]);
                const ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
                draw_list->AddText(
                    ImVec2(dot_center.x - text_size.x * 0.5f, dot_center.y - text_size.y * 0.5f),
                    ImColor(255, 255, 255, 255),
                    value_str.c_str()
                );
            }
            
            // Drawing index label
            const std::string index_str = std::to_string(i);
            const ImVec2 index_size = ImGui::CalcTextSize(index_str.c_str());
            draw_list->AddText(
                ImVec2(dot_center.x - index_size.x * 0.5f, dot_center.y + dot_radius + 5),
                ImColor(160, 160, 160, 200),
                index_str.c_str()
            );
        }
    }

    void ArrayBasedVisualizer::render_circular(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild("Circular", ImVec2(0, 400), true);
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        const ImVec2 region_size = ImGui::GetContentRegionAvail();
        
        const ImVec2 center = calculate_center(region_size);
        const ImVec2 absolute_center(
            cursor_pos.x + center.x, 
            cursor_pos.y + center.y
        );
        const float radius = std::min(region_size.x, region_size.y) * 0.35f;
        
        const int max_value = *std::max_element(
            step.data.begin(), 
            step.data.end()
        );
        if (max_value == 0) return;
        
        const float angle_step = (2.0f * 3.14159265f) / step.data.size();
        
        // Drawing background
        draw_list->AddCircleFilled(
            absolute_center, 
            radius + 15, 
            ImColor(30, 30, 45, 255)
        );
        draw_list->AddCircle(
            absolute_center, 
            radius + 15, 
            ImColor(80, 80, 100, 255), 
            0, 
            2.0f
        );
        
        for (size_t i = 0; i < step.data.size(); ++i)
        {
            const float value_ratio = static_cast<float>(step.data[i]) / max_value;
            const float bar_length = radius * 0.7f * value_ratio;
            const float angle = i * angle_step;
            
            const float cos_angle = std::cos(angle);
            const float sin_angle = std::sin(angle);
            
            const ImVec2 inner_point(
                absolute_center.x + cos_angle * (radius * 0.25f),
                absolute_center.y + sin_angle * (radius * 0.25f)
            );
            
            const ImVec2 outer_point(
                absolute_center.x + cos_angle * (radius * 0.25f + bar_length),
                absolute_center.y + sin_angle * (radius * 0.25f + bar_length)
            );
            
            const ImU32 color = get_enhanced_element_color(step, i);
            
            // Drawing radial bar
            draw_list->AddLine(inner_point, outer_point, color, 6.0f);
            
            // Drawing connection line for active elements
            const bool is_active = (step.visualization.highlighted_index.has_value() && 
                                    i == step.visualization.highlighted_index.value()) ||
                                   (step.visualization.compared_index.has_value() && 
                                    i == step.visualization.compared_index.value());
            
            if (is_active)
            {
                draw_list->AddLine(
                    absolute_center, 
                    inner_point, 
                    ImColor(255, 255, 255, 100), 
                    1.5f
                );
                draw_list->AddCircleFilled(
                    outer_point, 
                    6.0f, 
                    ImColor(255, 200, 100, 255)
                );
            }
            
            // Drawing value label
            if (step.data.size() <= 36)
            {
                const std::string value_str = std::to_string(step.data[i]);
                const ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
                const ImVec2 text_pos(
                    outer_point.x - text_size.x * 0.5f + cos_angle * 8,
                    outer_point.y - text_size.y * 0.5f + sin_angle * 8
                );
                draw_list->AddText(
                    text_pos, 
                    ImColor(255, 255, 255, 255), 
                    value_str.c_str()
                );
            }
        }
        
        // Drawing center circle
        draw_list->AddCircleFilled(
            absolute_center, 
            radius * 0.2f, 
            ImColor(40, 40, 55, 255)
        );
        draw_list->AddCircle(
            absolute_center, 
            radius * 0.2f, 
            ImColor(120, 120, 140, 255), 
            0, 
            2.0f
        );
        
        ImGui::EndChild();
    }

    void ArrayBasedVisualizer::render_network(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild("Network", ImVec2(0, 450), true);
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        const ImVec2 region_size = ImGui::GetContentRegionAvail();
        
        const std::vector<ImVec2> node_positions = calculate_network_positions(step.data, region_size);
        
        const int max_value = *std::max_element(
            step.data.begin(), 
            step.data.end()
        );
        if (max_value == 0) return;
        
        // Drawing connections between adjacent nodes
        for (size_t i = 0; i < step.data.size() - 1; ++i)
        {
            const bool is_active_comparison = (step.visualization.highlighted_index.has_value() &&
                                               i == step.visualization.highlighted_index.value() &&
                                               step.visualization.compared_index.has_value() &&
                                               i + 1 == step.visualization.compared_index.value());
            
            const ImU32 line_color = is_active_comparison ? 
                ImColor(255, 200, 100, 220) : ImColor(80, 80, 120, 120);
            const float line_thickness = is_active_comparison ? 3.0f : 1.5f;
            
            draw_list->AddLine(
                ImVec2(cursor_pos.x + node_positions[i].x, cursor_pos.y + node_positions[i].y),
                ImVec2(cursor_pos.x + node_positions[i + 1].x, cursor_pos.y + node_positions[i + 1].y),
                line_color, line_thickness
            );
        }
        
        // Drawing nodes
        for (size_t i = 0; i < step.data.size(); ++i)
        {
            const float value_ratio = static_cast<float>(step.data[i]) / max_value;
            const float node_radius = 18.0f + value_ratio * 12.0f;
            const ImU32 node_color = get_network_node_color(step, i, value_ratio);
            
            const ImVec2 screen_pos(
                cursor_pos.x + node_positions[i].x,
                cursor_pos.y + node_positions[i].y
            );
            
            // Drawing node with glow
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
            
            // Drawing value
            const std::string value_str = std::to_string(step.data[i]);
            const ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
            draw_list->AddText(
                ImVec2(screen_pos.x - text_size.x * 0.5f, screen_pos.y - text_size.y * 0.5f),
                ImColor(255, 255, 255, 255),
                value_str.c_str()
            );
            
            // Drawing index
            const std::string index_str = "[" + std::to_string(i) + "]";
            const ImVec2 index_size = ImGui::CalcTextSize(index_str.c_str());
            draw_list->AddText(
                ImVec2(screen_pos.x - index_size.x * 0.5f, screen_pos.y + node_radius + 5),
                ImColor(160, 160, 180, 200),
                index_str.c_str()
            );
            
            // Highlighting active nodes with pulse
            const bool is_active = (step.visualization.highlighted_index.has_value() && 
                                    i == step.visualization.highlighted_index.value()) ||
                                   (step.visualization.compared_index.has_value() && 
                                    i == step.visualization.compared_index.value());
            
            if (is_active)
            {
                const float pulse = (std::sin(m_animation_time * 6.0f) + 1.0f) * 0.5f;
                draw_list->AddCircle(
                    screen_pos, 
                    node_radius + 5 + pulse * 6,
                    ImColor(255, 255, 255, static_cast<int>(150 + 100 * pulse)), 
                    0, 
                    2.5f
                );
            }
        }
        
        ImGui::EndChild();
    }



    void ArrayBasedVisualizer::render_waveform(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild("Waveform", ImVec2(0, 250), true);
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        const ImVec2 region_size = ImGui::GetContentRegionAvail();
        
        const int max_value = *std::max_element(
            step.data.begin(), 
            step.data.end()
        );
        if (max_value == 0) return;
        
        const float x_step = region_size.x / (step.data.size() - 1);
        const float base_y = cursor_pos.y + region_size.y * 0.7f;
        const float amplitude_scale = region_size.y * 0.5f;
        
        // Drawing waveform lines
        for (size_t i = 0; i < step.data.size() - 1; ++i)
        {
            const float x1 = cursor_pos.x + i * x_step;
            const float y1 = base_y - (static_cast<float>(step.data[i]) / max_value) * amplitude_scale;
            
            const float x2 = cursor_pos.x + (i + 1) * x_step;
            const float y2 = base_y - (static_cast<float>(step.data[i + 1]) / max_value) * amplitude_scale;
            
            const ImU32 color = get_enhanced_element_color(step, i);
            draw_list->AddLine(
                ImVec2(x1, y1), 
                ImVec2(x2, y2), 
                color, 
                2.5f
            );
            
            // Drawing data points
            const bool is_active = (step.visualization.highlighted_index.has_value() && 
                                    i == step.visualization.highlighted_index.value()) ||
                                   (step.visualization.compared_index.has_value() && 
                                    i == step.visualization.compared_index.value());
            
            if (is_active)
            {
                draw_list->AddCircleFilled(
                    ImVec2(x1, y1), 
                    6.0f, 
                    ImColor(255, 200, 100, 255)
                );
                
                const std::string value_str = std::to_string(step.data[i]);
                const ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
                draw_list->AddText(
                    ImVec2(x1 - text_size.x * 0.5f, y1 - 20),
                    ImColor(255, 255, 255, 255), value_str.c_str()
                );
            }
        }
        
        // Drawing baseline
        draw_list->AddLine(
            ImVec2(cursor_pos.x, base_y),
            ImVec2(cursor_pos.x + region_size.x, base_y),
            ImColor(100, 100, 120, 150), 1.0f
        );
        
        ImGui::EndChild();
    }


    void ArrayBasedVisualizer::render_heatmap(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild("Heatmap", ImVec2(0, 400), true);
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        const ImVec2 region_size = ImGui::GetContentRegionAvail();
        
        const int max_value = *std::max_element(
            step.data.begin(), 
            step.data.end()
        );
        if (max_value == 0) return;
        
        const float cell_size = std::min(
            region_size.x / step.data.size(), 
             region_size.y / step.data.size()
         );
        const float grid_width = cell_size * step.data.size();
        const float start_x = cursor_pos.x + (region_size.x - grid_width) * 0.5f;
        const float start_y = cursor_pos.y + (region_size.y - grid_width) * 0.5f;
        
        for (size_t i = 0; i < step.data.size(); ++i)
        {
            for (size_t j = 0; j < step.data.size(); ++j)
            {
                const float value_ratio = static_cast<float>(step.data[i]) / max_value;
                const ImVec2 cell_min(start_x + j * cell_size, start_y + i * cell_size);
                const ImVec2 cell_max(cell_min.x + cell_size, cell_min.y + cell_size);
                
                ImU32 cell_color;
                
                // Highlight comparison cells
                if (step.visualization.highlighted_index.has_value() && 
                    step.visualization.compared_index.has_value() &&
                    i == step.visualization.highlighted_index.value() && 
                    j == step.visualization.compared_index.value())
                {
                    cell_color = ImColor(255, 200, 50, 220);
                }
                else if (i == j)
                {
                    cell_color = value_to_heatmap_color(value_ratio);
                }
                else if (step.data[i] > step.data[j])
                {
                    cell_color = ImColor(200, 60, 60, 100);
                }
                else
                {
                    cell_color = ImColor(60, 100, 200, 100);
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
                
                // Draw value on diagonal
                if (i == j && cell_size > 25)
                {
                    const std::string value_str = std::to_string(step.data[i]);
                    const ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
                    const ImVec2 text_pos(
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

    void ArrayBasedVisualizer::initialize_particles(
        const std::vector<int>& data)
    {
        m_particles.clear();
        m_particles.reserve(data.size());
        
        const size_t n = data.size();
        const float step = (n > 1) ? 1.0f / (n - 1) : 1.0f;
        
        for (size_t i = 0; i < n; ++i)
        {
            const float target_x = static_cast<float>(i) * step;
            
            Particle particle;
            particle.position = ImVec2(target_x, 0.5f);
            particle.velocity = ImVec2(0, 0);
            particle.target_position = ImVec2(target_x, 0.5f);
            particle.value = data[i];
            particle.size = 10.0f + static_cast<float>(data[i]) * 0.15f;
            particle.is_active = true;
            particle.lifetime = 1.0f;
            
            m_particles.push_back(particle);
        }
        
        m_has_initialized_particles = true;
        m_last_update_time = ImGui::GetTime();
    }

    void ArrayBasedVisualizer::update_particles(
        const AlgorithmStep& step, 
        float delta_time)
    {
        if (m_particles.size() != step.data.size())
        {
            initialize_particles(step.data);
            return;
        }
        
        const float dt = std::min(delta_time, 0.033f); // Capping at 30fps for stability
        const float acceleration = 10.0f;
        const float damping = 0.85f;
        
        for (size_t i = 0; i < m_particles.size(); ++i)
        {
            auto& particle = m_particles[i];
            
            // Updating target position based on current array order
            const float target_x = (m_particles.size() > 1) ? 
                static_cast<float>(i) / (m_particles.size() - 1) : 0.5f;
            
            // Adjusting Y position based on highlight state
            float target_y = 0.5f;
            if (step.visualization.highlighted_index.has_value() && 
                i == step.visualization.highlighted_index.value())
            {
                target_y = 0.3f;
                particle.size = 15.0f + static_cast<float>(step.data[i]) * 0.15f;
            }
            else if (step.visualization.compared_index.has_value() && 
                     i == step.visualization.compared_index.value())
            {
                target_y = 0.7f;
                particle.size = 15.0f + static_cast<float>(step.data[i]) * 0.15f;
            }
            else
            {
                particle.size = 8.0f + static_cast<float>(step.data[i]) * 0.1f;
            }
            
            particle.target_position = ImVec2(target_x, target_y);
            particle.value = step.data[i];
            
            // Applying spring force towards target
            const ImVec2 direction(
                particle.target_position.x - particle.position.x,
                particle.target_position.y - particle.position.y
            );
            
            particle.velocity.x = particle.velocity.x * damping + direction.x * acceleration * dt;
            particle.velocity.y = particle.velocity.y * damping + direction.y * acceleration * dt;
            
            particle.position.x += particle.velocity.x * dt;
            particle.position.y += particle.velocity.y * dt;
            
            // Clamp position
            particle.position.x = std::clamp(particle.position.x, 0.0f, 1.0f);
            particle.position.y = std::clamp(particle.position.y, 0.0f, 1.0f);
        }
    }

    void ArrayBasedVisualizer::draw_particle_connections(
        ImDrawList* draw_list, 
        const ImVec2& cursor_pos,
        const ImVec2& region_size, 
        const AlgorithmStep& step)
    {
        // Drawing connections between comparing particles
        if (step.visualization.highlighted_index.has_value() &&
            step.visualization.compared_index.has_value())
        {
            const size_t idx1 = step.visualization.highlighted_index.value();
            const size_t idx2 = step.visualization.compared_index.value();
            
            if (idx1 < m_particles.size() && idx2 < m_particles.size())
            {
                const auto& p1 = m_particles[idx1];
                const auto& p2 = m_particles[idx2];
                
                const ImVec2 screen_pos1(
                    cursor_pos.x + p1.position.x * region_size.x,
                    cursor_pos.y + p1.position.y * region_size.y
                );
                const ImVec2 screen_pos2(
                    cursor_pos.x + p2.position.x * region_size.x,
                    cursor_pos.y + p2.position.y * region_size.y
                );
                
                const float pulse = (std::sin(m_animation_time * 12.0f) + 1.0f) * 0.5f;
                const ImU32 line_color = ImColor(255, 200, 100, static_cast<int>(150 + 100 * pulse));
                
                draw_list->AddLine(screen_pos1, screen_pos2, line_color, 3.0f);
            }
        }
    }



    void ArrayBasedVisualizer::render_particle_system(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild("ParticleSystem", ImVec2(0, 450), true);
        
        // Initializing or resize particles if needed
        if (!m_has_initialized_particles || 
            m_particles.size() != step.data.size())
        {
            initialize_particles(step.data);
        }
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        const ImVec2 region_size = ImGui::GetContentRegionAvail();
        
        // Drawing connections
        draw_particle_connections(
            draw_list, 
            cursor_pos, 
            region_size, 
            step
        );
        
        // Drawing particles
        for (size_t i = 0; i < m_particles.size(); ++i)
        {
            const auto& particle = m_particles[i];
            const ImVec2 screen_pos(
                cursor_pos.x + particle.position.x * region_size.x,
                cursor_pos.y + particle.position.y * region_size.y
            );
            
            const ImU32 particle_color = get_particle_color(step, i);
            
            // Drawing particle
            draw_list->AddCircleFilled(
                screen_pos, 
                particle.size, 
                particle_color
            );
            
            // Outer glow
            const float pulse = (std::sin(m_animation_time * 6.0f + static_cast<float>(i) * 0.5f) + 1.0f) * 0.3f;
            draw_list->AddCircle(
                screen_pos, 
                particle.size + 3.0f + pulse * 3.0f,
                ImColor(255, 255, 255, 80), 
                0, 
                1.5f
            );
            
            // Value label
            const std::string value_str = std::to_string(particle.value);
            const ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
            draw_list->AddText(
                ImVec2(screen_pos.x - text_size.x * 0.5f, screen_pos.y - text_size.y * 0.5f),
                ImColor(255, 255, 255, 255),
                value_str.c_str()
            );
            
            // Index label
            const std::string index_str = "[" + std::to_string(i) + "]";
            const ImVec2 index_size = ImGui::CalcTextSize(index_str.c_str());
            draw_list->AddText(
                ImVec2(screen_pos.x - index_size.x * 0.5f, screen_pos.y + particle.size + 5),
                ImColor(160, 160, 160, 200),
                index_str.c_str()
            );
        }
        
        ImGui::EndChild();
    }

    void ArrayBasedVisualizer::render_tree_view(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild("TreeView", ImVec2(0, 500), true);
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        const ImVec2 region_size = ImGui::GetContentRegionAvail();
        
        const std::vector<TreeNode> tree_nodes = build_binary_tree(step.data);
        const std::vector<ImVec2> node_positions = calculate_tree_positions(
            tree_nodes.size(), 
            region_size
        );
        
        const int max_value = *std::max_element(
            step.data.begin(), 
            step.data.end()
        );
        if (max_value == 0) return;
        
        // Drawing connections first
        for (size_t i = 0; i < tree_nodes.size(); ++i)
        {
            if (tree_nodes[i].left_child != static_cast<size_t>(-1) && 
                tree_nodes[i].left_child < node_positions.size())
            {
                draw_list->AddLine(
                    ImVec2(cursor_pos.x + node_positions[i].x, cursor_pos.y + node_positions[i].y),
                    ImVec2(cursor_pos.x + node_positions[tree_nodes[i].left_child].x, 
                           cursor_pos.y + node_positions[tree_nodes[i].left_child].y),
                    ImColor(100, 100, 150, 120), 2.0f
                );
            }
            
            if (tree_nodes[i].right_child != static_cast<size_t>(-1) && 
                tree_nodes[i].right_child < node_positions.size())
            {
                draw_list->AddLine(
                    ImVec2(cursor_pos.x + node_positions[i].x, cursor_pos.y + node_positions[i].y),
                    ImVec2(cursor_pos.x + node_positions[tree_nodes[i].right_child].x, 
                           cursor_pos.y + node_positions[tree_nodes[i].right_child].y),
                    ImColor(100, 100, 150, 120), 
                    2.0f
                );
            }
        }
        
        // Drawing nodes
        for (size_t i = 0; i < tree_nodes.size() && i < node_positions.size(); ++i)
        {
            const float value_ratio = static_cast<float>(tree_nodes[i].value) / max_value;
            const ImU32 node_color = get_tree_node_color(step, i, value_ratio);
            const float node_radius = 18.0f + value_ratio * 10.0f;
            
            const ImVec2 screen_pos(
                cursor_pos.x + node_positions[i].x,
                cursor_pos.y + node_positions[i].y
            );
            
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
            
            const std::string value_str = std::to_string(tree_nodes[i].value);
            const ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
            draw_list->AddText(
                ImVec2(screen_pos.x - text_size.x * 0.5f, screen_pos.y - text_size.y * 0.5f),
                ImColor(255, 255, 255, 255),
                value_str.c_str()
            );
            
            const std::string index_str = "[" + std::to_string(i) + "]";
            const ImVec2 index_size = ImGui::CalcTextSize(index_str.c_str());
            draw_list->AddText(
                ImVec2(screen_pos.x - index_size.x * 0.5f, screen_pos.y + node_radius + 5),
                ImColor(160, 160, 160, 200),
                index_str.c_str()
            );
            
            // Highlight active nodes
            const bool is_active = (step.visualization.highlighted_index.has_value() && 
                                    i == step.visualization.highlighted_index.value()) ||
                                   (step.visualization.compared_index.has_value() && 
                                    i == step.visualization.compared_index.value());
            
            if (is_active)
            {
                const float pulse = (std::sin(m_animation_time * 8.0f) + 1.0f) * 0.5f;
                draw_list->AddCircle(
                    screen_pos, 
                    node_radius + 5.0f + pulse * 6.0f,
                    ImColor(255, 255, 255, static_cast<int>(150 + 100 * pulse)), 
                    0, 
                    2.5f
                );
            }
        }
        
        ImGui::EndChild();
    }

        void ArrayBasedVisualizer::render_molecular(
            const AlgorithmStep& step)
    {
        ImGui::BeginChild("Molecular", ImVec2(0, 400), true);
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        const ImVec2 region_size = ImGui::GetContentRegionAvail();
        
        const ImVec2 center = calculate_center(region_size);
        const ImVec2 absolute_center(
            cursor_pos.x + center.x, 
            cursor_pos.y + center.y
        );
        const float max_radius = std::min(region_size.x, region_size.y) * 0.35f;
        
        const int max_value = *std::max_element(
            step.data.begin(), 
            step.data.end()
        );
        if (max_value == 0) return;
        
        const float angle_step = (2.0f * 3.14159265f) / step.data.size();
        
        for (size_t i = 0; i < step.data.size(); ++i)
        {
            const float radius_ratio = static_cast<float>(step.data[i]) / max_value;
            const float angle = i * angle_step;
            const float radius = 30.0f + radius_ratio * max_radius;
            
            const ImVec2 position(
                absolute_center.x + std::cos(angle) * radius,
                absolute_center.y + std::sin(angle) * radius
            );
            
            const ImU32 atom_color = get_molecular_color(step, i, radius_ratio);
            const float atom_size = 10.0f + radius_ratio * 10.0f;
            
            // Drawing atom
            draw_list->AddCircleFilled(position, atom_size, atom_color);
            
            // Electron rings
            for (int ring = 1; ring <= 2; ++ring)
            {
                const float ring_radius = atom_size + ring * 8.0f;
                draw_list->AddCircle(
                    position, 
                    ring_radius, 
                    ImColor(100, 100, 200, 60), 
                    0, 
                    1.0f
                );
            }
            
            // Value label
            const std::string value_str = std::to_string(step.data[i]);
            const ImVec2 text_size = ImGui::CalcTextSize(value_str.c_str());
            draw_list->AddText(
                ImVec2(position.x - text_size.x * 0.5f, position.y - atom_size - 10),
                ImColor(255, 255, 255, 255),
                value_str.c_str()
            );
            
            // Drawing bond between comparing elements
            if (step.visualization.highlighted_index.has_value() &&
                step.visualization.compared_index.has_value() &&
                i == step.visualization.highlighted_index.value())
            {
                const size_t other_idx = step.visualization.compared_index.value();
                if (other_idx < step.data.size())
                {
                    const float other_radius_ratio = static_cast<float>(step.data[other_idx]) / max_value;
                    const float other_radius = 30.0f + other_radius_ratio * max_radius;
                    const float other_angle = other_idx * angle_step;
                    
                    const ImVec2 other_position(
                        absolute_center.x + std::cos(other_angle) * other_radius,
                        absolute_center.y + std::sin(other_angle) * other_radius
                    );
                    
                    const float pulse = (std::sin(m_animation_time * 12.0f) + 1.0f) * 0.5f;
                    const ImU32 bond_color = ImColor(255, 200, 100, static_cast<int>(150 + 100 * pulse));
                    
                    draw_list->AddLine(
                        position, 
                        other_position, 
                        bond_color, 
                        3.0f
                    );
                }
            }
        }
        
        ImGui::EndChild();
    }

    
    std::vector<ImVec2> ArrayBasedVisualizer::calculate_layer_positions(
        int node_count, 
        const ImVec2& region_size, 
        float x_ratio) const
    {
        std::vector<ImVec2> positions;
        positions.reserve(static_cast<size_t>(node_count));
        
        const float x = region_size.x * x_ratio;
        
        for (int i = 0; i < node_count; ++i)
        {
            const float y = (static_cast<float>(i) + 0.5f) / node_count * region_size.y;
            positions.emplace_back(x, y);
        }
        
        return positions;
    }

    void ArrayBasedVisualizer::draw_neural_nodes(
        ImDrawList* draw_list, 
        const std::vector<ImVec2>& nodes,
        const AlgorithmStep& step, 
        const std::string& layer_name) const
    {
        const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        const int max_value = *std::max_element(
            step.data.begin(), 
            step.data.end()
        );
        
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const ImVec2 screen_pos(
                cursor_pos.x + nodes[i].x, 
                cursor_pos.y + nodes[i].y
            );
            const float activation = max_value > 0 ? 
                static_cast<float>(step.data[i % step.data.size()]) / max_value : 0.5f;
            
            const ImU32 node_color = ImColor(
                static_cast<int>(255 * activation),
                static_cast<int>(150 * (1.0f - activation)),
                static_cast<int>(100 + 155 * activation),
                255
            );
            
            const float node_size = 8.0f + activation * 12.0f;
            
            draw_list->AddCircleFilled(
                screen_pos, 
                node_size, 
                node_color
            );
            draw_list->AddCircle(
                screen_pos, 
                node_size + 2.0f, 
                ImColor(255, 255, 255, 100), 
                0, 
                1.5f
            );
            
            // Activation glow
            if (activation > 0.7f)
            {
                const float pulse = (std::sin(m_animation_time * 8.0f) + 1.0f) * 0.5f;
                draw_list->AddCircle(
                    screen_pos, 
                    node_size + 4.0f + pulse * 3.0f,
                    ImColor(255, 200, 100, static_cast<int>(100 + 100 * pulse)), 
                    0, 
                    1.0f
                );
            }
        }
    }

    ImU32 ArrayBasedVisualizer::get_neural_connection_color(
        float weight
    ) const
    {
        if (weight > 0.7f) return ImColor(50, 200, 50, 150);   // Strong - green
        if (weight > 0.3f) return ImColor(200, 200, 50, 120);  // Medium - yellow
        return ImColor(200, 50, 50, 80);                       // Weak - red
    }

    void ArrayBasedVisualizer::render_neural_network(
        const AlgorithmStep& step)
    {
        ImGui::BeginChild("NeuralNetwork", ImVec2(0, 450), true);
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        const ImVec2 region_size = ImGui::GetContentRegionAvail();
        
        const int input_size = static_cast<int>(step.data.size());
        const int hidden_size = std::min(8, input_size);
        const int output_size = input_size;
        
        const std::vector<ImVec2> input_nodes = calculate_layer_positions(
            input_size, 
            region_size, 
            0.1f
        );
        const std::vector<ImVec2> hidden_nodes = calculate_layer_positions(
            hidden_size, 
            region_size, 
            0.5f
        );
        const std::vector<ImVec2> output_nodes = calculate_layer_positions(
            output_size, 
            region_size, 
            0.9f
        );
        
        const int max_value = *std::max_element(
            step.data.begin(), 
            step.data.end()
        );
        
        // Drawing connections: Input -> Hidden
        for (size_t i = 0; i < input_nodes.size(); ++i)
        {
            for (size_t j = 0; j < hidden_nodes.size(); ++j)
            {
                const float weight = max_value > 0 ? 
                    static_cast<float>(step.data[i]) / max_value : 0.5f;
                
                const ImU32 connection_color = get_neural_connection_color(weight);
                const float line_width = 1.0f + weight * 2.5f;
                
                draw_list->AddLine(
                    ImVec2(cursor_pos.x + input_nodes[i].x, cursor_pos.y + input_nodes[i].y),
                    ImVec2(cursor_pos.x + hidden_nodes[j].x, cursor_pos.y + hidden_nodes[j].y),
                    connection_color, line_width
                );
            }
        }
        
        // Drawing connections: Hidden -> Output
        for (size_t i = 0; i < hidden_nodes.size(); ++i)
        {
            for (size_t j = 0; j < output_nodes.size(); ++j)
            {
                const float weight = 1.0f - (static_cast<float>(i) / hidden_nodes.size());
                const ImU32 connection_color = get_neural_connection_color(weight);
                const float line_width = 1.0f + weight * 2.5f;
                
                draw_list->AddLine(
                    ImVec2(cursor_pos.x + hidden_nodes[i].x, cursor_pos.y + hidden_nodes[i].y),
                    ImVec2(cursor_pos.x + output_nodes[j].x, cursor_pos.y + output_nodes[j].y),
                    connection_color, line_width
                );
            }
        }
        
        // Drawing nodes
        draw_neural_nodes(draw_list, input_nodes, step, "Input");
        draw_neural_nodes(draw_list, hidden_nodes, step, "Hidden");
        draw_neural_nodes(draw_list, output_nodes, step, "Output");
        
        ImGui::EndChild();
    }


    // Helper functions
    ImU32 ArrayBasedVisualizer::get_molecular_color(
        const AlgorithmStep& step, 
        size_t index, 
        float radius_ratio
    ) const
    {
        if (step.visualization.highlighted_index.has_value() &&
            index == step.visualization.highlighted_index.value())
        {
            return ImColor(255, 215, 0, 255);  // Gold
        }
        
        if (step.visualization.compared_index.has_value() &&
            index == step.visualization.compared_index.value())
        {
            return ImColor(50, 205, 50, 255);  // Lime green
        }
        
        // Chemical element inspired colors based on value
        if (radius_ratio < 0.25f) return ImColor(70, 130, 180, 255);   // Steel blue
        if (radius_ratio < 0.5f) return ImColor(34, 139, 34, 255);     // Forest green
        if (radius_ratio < 0.75f) return ImColor(255, 140, 0, 255);    // Dark orange
        return ImColor(178, 34, 34, 255);  // Fire brick red
    }

    std::vector<ArrayBasedVisualizer::TreeNode> 
    ArrayBasedVisualizer::build_binary_tree(
        const std::vector<int>& data
    ) const
    {
        std::vector<TreeNode> tree;
        tree.reserve(data.size());
        
        for (size_t i = 0; i < data.size(); ++i)
        {
            TreeNode node(data[i]);
            node.left_child = (2 * i + 1 < data.size()) ? 2 * i + 1 : static_cast<size_t>(-1);
            node.right_child = (2 * i + 2 < data.size()) ? 2 * i + 2 : static_cast<size_t>(-1);
            node.parent = (i > 0) ? (i - 1) / 2 : static_cast<size_t>(-1);
            tree.push_back(node);
        }
        
        return tree;
    }

    std::vector<ImVec2> ArrayBasedVisualizer::calculate_tree_positions(
        size_t node_count, 
        const ImVec2& region_size
    ) const
    {
        std::vector<ImVec2> positions;
        if (node_count == 0) return positions;
        
        positions.reserve(node_count);
        
        // Calculate tree depth
        int depth = 0;
        size_t max_nodes_at_depth = 1;
        size_t total_nodes = 0;
        
        while (total_nodes < node_count)
        {
            depth++;
            total_nodes += max_nodes_at_depth;
            max_nodes_at_depth *= 2;
        }
        
        // Calculating positions for each level
        size_t current_index = 0;
        for (int level = 0; level < depth && current_index < node_count; ++level)
        {
            const int nodes_in_level = 1 << level;
            const float level_height = region_size.y / (depth + 1);
            const float y = (level + 1) * level_height;
            
            for (int i = 0; i < nodes_in_level && current_index < node_count; ++i)
            {
                const float x = (static_cast<float>(i) + 0.5f) / nodes_in_level * region_size.x;
                positions.emplace_back(x, y);
                current_index++;
            }
        }
        
        return positions;
    }

    ImU32 ArrayBasedVisualizer::get_tree_node_color(
        const AlgorithmStep& step, 
        size_t index, 
        float value_ratio
    ) const
    {
        if (step.visualization.highlighted_index.has_value() &&
            index == step.visualization.highlighted_index.value())
            return ImColor(255, 200, 50, 255);
            
        if (step.visualization.compared_index.has_value() &&
            index == step.visualization.compared_index.value())
            return ImColor(50, 200, 100, 255);
            
        if (!step.visualization.additional_highlights.empty() &&
            std::find(step.visualization.additional_highlights.begin(),
                     step.visualization.additional_highlights.end(), index) !=
            step.visualization.additional_highlights.end())
            return ImColor(100, 150, 255, 255);
            
        return ImColor(
            static_cast<int>(80 + 175 * value_ratio),
            static_cast<int>(120 + 135 * (1.0f - value_ratio)),
            200,
            255
        );
    }

    ImU32 ArrayBasedVisualizer::get_particle_color(
        const AlgorithmStep& step, 
        size_t index
    ) const
    {
        if (step.visualization.highlighted_index.has_value() &&
            index == step.visualization.highlighted_index.value())
        {
            return ImColor(255, 200, 50, 255);
        }
        
        if (step.visualization.compared_index.has_value() &&
            index == step.visualization.compared_index.value())
        {
            return ImColor(50, 220, 120, 255);
        }
        
        const int max_value = *std::max_element(step.data.begin(), step.data.end());
        const float value_ratio = max_value > 0 ? 
            static_cast<float>(step.data[index]) / max_value : 0.5f;
        
        return ImColor(
            static_cast<int>(100 + 155 * value_ratio),
            static_cast<int>(100 + 155 * (1.0f - value_ratio)),
            200,
            255
        );
    }

    ImU32 ArrayBasedVisualizer::value_to_heatmap_color(
        float ratio
    ) const
    {
        // Blue (cool) to Red (hot) heatmap
        int r = static_cast<int>(255 * ratio);
        int g = static_cast<int>(128 * (1.0f - std::abs(ratio - 0.5f) * 2.0f));
        int b = static_cast<int>(255 * (1.0f - ratio));
        return ImColor(r, g, b, 220);
    }

    std::vector<ImVec2> ArrayBasedVisualizer::calculate_network_positions(
        const std::vector<int>& data, 
        const ImVec2& region_size) const
    {
        std::vector<ImVec2> positions;
        positions.reserve(data.size());
        
        if (data.size() <= 1)
        {
            positions.emplace_back(region_size.x * 0.5f, region_size.y * 0.5f);
            return positions;
        }
        
        for (size_t i = 0; i < data.size(); ++i)
        {
            const float x_ratio = static_cast<float>(i) / (data.size() - 1);
            const float x = x_ratio * region_size.x;
            
            // Adding slight sine wave curve for better visibility
            const float curve = std::sin(x_ratio * 3.14159265f) * 0.15f;
            const float y = (0.5f + curve) * region_size.y;
            
            positions.emplace_back(x, y);
        }
        
        return positions;
    }

    ImU32 ArrayBasedVisualizer::get_network_node_color(
        const AlgorithmStep& step, 
        size_t index, 
        float value_ratio) const
    {
        if (step.visualization.highlighted_index.has_value() &&
            index == step.visualization.highlighted_index.value())
            return ImColor(255, 200, 50, 255);
            
        if (step.visualization.compared_index.has_value() &&
            index == step.visualization.compared_index.value())
            return ImColor(50, 200, 100, 255);
            
        return ImColor(
            static_cast<int>(100 + 155 * value_ratio),
            static_cast<int>(100 + 155 * (1.0f - value_ratio)),
            200,
            255
        );
    }

    ImVec2 ArrayBasedVisualizer::calculate_center(
        const ImVec2& region_size
    ) const
    {
        return ImVec2(region_size.x * 0.5f, region_size.y * 0.5f);
    }

    ImU32 ArrayBasedVisualizer::get_enhanced_element_color(
        const AlgorithmStep& step, 
        size_t index
    ) const
    {
        if (index >= step.data.size())
            return ImColor(100, 100, 100, 255);

        // Calculating value ratio for gradient
        const int max_value = *std::max_element(
            step.data.begin(), 
            step.data.end()
        );

        float value_ratio = max_value > 0 
            ? static_cast<float>(step.data[index]) / static_cast<float>(max_value)
            : 0.5f;

        // Priority highlights
        if (step.visualization.highlighted_index.has_value() &&
            index == step.visualization.highlighted_index.value())
        {
            return ImColor(255, 200, 50, 255);      // Golden yellow
        }

        if (step.visualization.compared_index.has_value() &&
            index == step.visualization.compared_index.value())
        {
            return ImColor(50, 220, 120, 255);      // Emerald green   
        }

        if (!step.visualization.additional_highlights.empty() &&
            std::find(step.visualization.additional_highlights.begin(),
                     step.visualization.additional_highlights.end(), 
                     index) != step.visualization.additional_highlights.end())
        {
            if (step.visualization.is_partition_step)
            {
                return ImColor(180, 80, 220, 255);    // Purple for pivot
            }
            return ImColor(80, 180, 255, 255);        // Bright blue for sorted
        }

        // Rainbow gradient based on value
        return value_to_rainbow_color(value_ratio);
    }

    ImU32 ArrayBasedVisualizer::value_to_rainbow_color(
        float ratio
    ) const
    {
        int r, g, b;
        
        if (ratio < 0.25f) 
        {
            // Blue to Cyan
            r = 0;
            g = static_cast<int>(255 * (ratio / 0.25f));
            b = 255;
        } else if (ratio < 0.5f) 
        {
            // Cyan to Green
            r = 0;
            g = 255;
            b = static_cast<int>(255 * (1.0f - (ratio - 0.25f) / 0.25f));
        } else if (ratio < 0.75f) 
        {
            // Green to Yellow
            r = static_cast<int>(255 * (ratio - 0.5f) / 0.25f);
            g = 255;
            b = 0;
        } else 
        {
            // Yellow to Red
            r = 255;
            g = static_cast<int>(255 * (1.0f - (ratio - 0.75f) / 0.25f));
            b = 0;
        }
        
        return ImColor(r, g, b, 255);
    }

    ImU32 ArrayBasedVisualizer::apply_color_variation(
        ImU32 color, 
        float factor
    ) const
    {
        int r = (color >> IM_COL32_R_SHIFT) & 0xFF;
        int g = (color >> IM_COL32_G_SHIFT) & 0xFF;
        int b = (color >> IM_COL32_B_SHIFT) & 0xFF;
        
        r = std::min(255, static_cast<int>(r * factor));
        g = std::min(255, static_cast<int>(g * factor));
        b = std::min(255, static_cast<int>(b * factor));
        
        return ImColor(r, g, b, 255);
    }

    ImU32 ArrayBasedVisualizer::get_element_color(
        const AlgorithmStep& step, 
        size_t index) const
    {
        if (index >= step.data.size()) 
            return ImColor(100, 100, 100, 255);
        
        const auto& colors = m_metadata->get_visualization_config().highlight_colors;

        // Highlighted index (current focus)
        if (step.visualization.highlighted_index.has_value() &&
            index == step.visualization.highlighted_index.value())
        {
            return colors.current;
        }
        
        // Compared index (being compared)
        if (step.visualization.compared_index.has_value() &&
            index == step.visualization.compared_index.value())
        {
            return colors.compared;
        }
        
        // Additional highlights (sorted portion, partition, etc.)
        if (!step.visualization.additional_highlights.empty())
        {
            if (std::find(step.visualization.additional_highlights.begin(),
                         step.visualization.additional_highlights.end(), 
                         index) != step.visualization.additional_highlights.end())
            {
                // Special case for partition step
                if (step.visualization.is_partition_step)
                {
                    return colors.pivot;    
                }
                return colors.sorted;      
            }
        }
        
        // Default: Gradient based on value
        const int max_value = *std::max_element(
            step.data.begin(), 
            step.data.end()
        );
        const int min_value = *std::min_element(
            step.data.begin(), 
            step.data.end()
        );
        
        float value_ratio = (max_value > min_value) ? 
            static_cast<float>(step.data[index] - min_value) / 
            static_cast<float>(max_value - min_value) : 0.5f;
        
        // Blue (low) to Red (high) gradient
        return ImColor(
            static_cast<int>(100 + 155 * value_ratio),   // R
            static_cast<int>(150 - 100 * value_ratio),   // G
            static_cast<int>(200 - 100 * value_ratio),   // B
            255
        );
    }


    float ArrayBasedVisualizer::calculate_bar_width(
        size_t data_size, 
        float available_width
    ) const 
    {
        if (data_size == 0) 
            return 10.0f;

        float width = available_width / static_cast<float>(data_size);

        constexpr float MIN_BAR_WIDTH = 4.0f;
        constexpr float MAX_BAR_WIDTH = 60.0f;

        width = std::max(MIN_BAR_WIDTH, width - 2.0f);
        width = std::min(width, MAX_BAR_WIDTH);

        return width;
    }

    float ArrayBasedVisualizer::calculate_max_bar_height(
        const ImVec2& region_size
    ) const
    {
        return std::max(50.0f, region_size.y - 80.0f);
    }



    void ArrayBasedVisualizer::set_visualization_style(
        VisualizationStyle style)
    {
        m_current_style = style;
    }

    VisualizationStyle 
    ArrayBasedVisualizer::get_visualization_style() const
    {
        return m_current_style;
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
