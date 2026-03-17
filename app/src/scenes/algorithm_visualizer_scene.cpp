#include "scenes/algorithm_visualizer_scene.hpp"
#include "algorithms/core/algorithm_types.hpp"
#include "core/utils/utils.hpp"
#include "core/utils/logger/logger.hpp"
#include "core/utils/variables.hpp"

#include <iterator>

namespace c2l::scenes
{
    AlgorithmVisualizerScene::AlgorithmVisualizerScene(
        graphics::Renderer& renderer,
        core::ThreadManager& thread_manager,
        core::JsonConfigManager& json_config_manager,
        algorithms::AlgorithmRegistry& algorithm_registry,
        core::resources::ResourceManager& resource_manager,
        c2l::ui::managers::IconManager& icon_manager)
        : BaseScene(
            renderer, 
            thread_manager, 
            json_config_manager,
            algorithm_registry,
            resource_manager, 
            icon_manager
        )
        , m_algorithm_manager{
            std::make_unique<algorithms::AlgorithmManager>(
                thread_manager, 
                algorithm_registry
            )
        }
    {
        LOG_INFO("AlgorithmVisualizerScene created");
    }

    void AlgorithmVisualizerScene::on_create()
    {
        setup_ui_components();
        setup_main_menu();
        setup_shortcuts_tooltip();

        // Loading default algorithm
        m_algorithm_manager->load_algorithm(
            algorithms::AlgorithmType::BUBBLE_SORT
        );
        m_algorithm_manager->generate_and_set_random_data();

        // Cache categorized algorithms
        m_cached_categorized_algorithms = m_algorithm_registry.get_available_categorized_algorithms();

        LOG_INFO("AlgorithmVisualizerScene initialized");
    }

    void AlgorithmVisualizerScene::on_destroy()
    {
        if (m_algorithm_manager)
        {
            m_algorithm_manager->stop_background_execution();
        }

        m_ui_manager->unregister_all_components();
    }

    void AlgorithmVisualizerScene::on_activate()
    {
        if (!is_created())
        {
            on_create();
            mark_created();
        }
    }

    void AlgorithmVisualizerScene::on_deactivate()
    {
        m_ui_manager->hide_all_panels();
        m_ui_manager->close_all_popups();

        if (m_algorithm_manager)
        {
            m_algorithm_manager->pause();
        }
    }

    void AlgorithmVisualizerScene::process_input(
        const core::InputHandler& input)
    {
        if (input.is_key_just_pressed(GLFW_KEY_SPACE))
        {
            if (m_algorithm_manager->is_playing()) {
                m_algorithm_manager->pause();
            } else {
                m_algorithm_manager->play();
            }
        }

        if (input.is_key_just_pressed(GLFW_KEY_R))
        {
            m_algorithm_manager->stop();
            m_algorithm_manager->generate_and_set_random_data();
        }

        // View toggles
        if (input.is_key_just_pressed(GLFW_KEY_F1))
            m_show_algorithm_selector_panel = !m_show_algorithm_selector_panel;
        if (input.is_key_just_pressed(GLFW_KEY_F2))
            m_show_algorithm_code_panel = !m_show_algorithm_code_panel;
        if (input.is_key_just_pressed(GLFW_KEY_F3))
            m_show_algorithm_description_panel = !m_show_algorithm_description_panel;
        if (input.is_key_just_pressed(GLFW_KEY_F4))
            m_show_algorithm_variable_inspector_panel = !m_show_algorithm_variable_inspector_panel;
        
        // Thread control shortcuts
        if (input.is_key_just_pressed(GLFW_KEY_T))
        {
            if (m_algorithm_manager->is_executing())
            {
                m_algorithm_manager->stop_background_execution();
            } else {
                m_algorithm_manager->start_background_execution();
            }
        }

        // Using callback to switch back to the main scene, 
        // rather than SceneManager dependency
        if (input.is_key_just_pressed(GLFW_KEY_ESCAPE))
        {
            request_scene_pop();  
        }
    }

    void AlgorithmVisualizerScene::update(double delta_time)
    {
        m_ui_manager->update(delta_time);

        if (m_algorithm_manager)
        {
            m_algorithm_manager->update(delta_time);
        }

        m_icon_manager.update();
    }

    void AlgorithmVisualizerScene::render()
    {
        m_renderer.render();
        m_ui_manager->render();

        // Rendering algorithm visualization components
        if (m_show_algorithm_selector_panel)
        {
            render_algorithm_selector_panel();
        }

        if (m_show_algorithm_visualization_panel)
        {
            render_algorithm_visualization_panel();
        }

        if (m_show_algorithm_control_panel)
        {
            render_algorithm_control_panel();
        }

        if (m_show_algorithm_data_control_panel)
        {
            render_algorithm_data_control_panel();
        }

        if (m_show_thread_info_panel)
        {
            render_thread_info_panel();
        }

        if (m_show_algorithm_stats_panel)
        {
            render_algorithm_stats_panel();
        }

        if (m_show_algorithm_code_panel)
        {
            render_algorithm_code_panel();
        }

        if (m_show_algorithm_description_panel)
        {
            render_algorithm_description_panel();

        }

        render_common_ui();
        m_renderer.clear();
    }

    void AlgorithmVisualizerScene::setup_main_menu()
    {
       std::vector<ui::components::MainMenu::MenuItem> view_items = {
            {
                "Algorithm Selector", 
                [this]() 
                { 
                    m_show_algorithm_selector_panel = !m_show_algorithm_selector_panel; 
                }, 
                nullptr, 
                "F1"
            },
            {
                "Visualization", 
                [this]() 
                { 
                    m_show_algorithm_visualization_panel = !m_show_algorithm_visualization_panel; 
                },
                nullptr, 
                "F2"
            },
            {
                "Controls", 
                [this]() 
                { 
                    m_show_algorithm_control_panel = !m_show_algorithm_control_panel; 
                }, 
                nullptr, 
                "F3"
            },
            {
                "Code Panel", 
                [this]() 
                { 
                    m_show_algorithm_code_panel = !m_show_algorithm_code_panel; 
                }, 
                nullptr, 
                "F4"
            },
            {
                "Description", 
                [this]() 
                { 
                    m_show_algorithm_description_panel = !m_show_algorithm_description_panel; 
                }, 
                nullptr, 
                "F5"
            },
            {
                "Statistics", 
                [this]() 
                { 
                    m_show_algorithm_stats_panel = !m_show_algorithm_stats_panel; 
                }, 
                nullptr, 
                "F6"
            },
            {
                "Variable Inspector", 
                [this]() 
                { 
                    m_show_algorithm_variable_inspector_panel = !m_show_algorithm_variable_inspector_panel; 
                }, 
                nullptr, 
                "F7"
            },
            {
                "Thread Info", 
                [this]() 
                { 
                    m_show_thread_info_panel = !m_show_thread_info_panel; 
                }, 
                nullptr, 
                "F8"
            },
        };

        m_main_menu->add_menu("View", std::move(view_items));
    }

    void AlgorithmVisualizerScene::render_algorithm_selector_panel()
    {
        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        ImGui::Begin(
            "Algorithm Selector", 
            &m_show_algorithm_selector_panel, 
            ImGuiWindowFlags_NoCollapse
        );

        // Header with icon
        render_algorithm_header(nullptr);

        // Search box
        static char search_buffer[128] = "";
        ImGui::InputTextWithHint(
            "##Search", 
            "Search algorithms...", 
            search_buffer, 
            std::size(search_buffer)
        );
        ImGui::Separator();

        // Algorithm categories
        for (const auto& [category_name, algorithms] : m_cached_categorized_algorithms)
        {
            std::string category_id = std::string(category_name) + "_category";
            
            if (ImGui::TreeNodeEx(
                    category_id.c_str(), 
                    ImGuiTreeNodeFlags_DefaultOpen, 
                    "%s (%zu)", 
                    std::string(category_name).c_str(), 
                    algorithms.size())
                )
            {
                for (const auto* algorithm : algorithms)
                {
                    // Applying search filter
                    if (search_buffer[0] != '\0')
                    {
                        std::string name = std::string(algorithm->display_name);
                        std::string lower_name = name;
                        std::string lower_search = search_buffer;
                        
                        std::transform(
                            lower_name.begin(), 
                            lower_name.end(), 
                            lower_name.begin(), 
                            ::tolower
                        );
                        std::transform(
                            lower_search.begin(), 
                            lower_search.end(), 
                            lower_search.begin(), 
                            ::tolower
                        );
                                     
                        if (lower_name.find(lower_search) == std::string::npos)
                            continue;
                    }

                    bool is_current = m_algorithm_manager->get_current_algorithm_type() 
                                    == algorithm->type;

                    // Rendering algorithm item with icon
                    ImGui::PushID(algorithm->id.data());
                    
                    if (is_current)
                    {
                        ImGui::PushStyleColor(
                            ImGuiCol_Text, 
                            ImVec4(0.2f, 0.8f, 0.2f, 1.0f)
                        );
                    }
                        
                    if (ImGui::Selectable(
                            std::string(algorithm->display_name).c_str(), 
                            is_current)
                        )
                    {
                        m_algorithm_manager->load_algorithm(algorithm->type);
                    }

                    if (is_current)
                        ImGui::PopStyleColor();

                    // Tooltip with brief description
                    // TODO: Hover algorithm info (no only current algorithm info)
                    // if (ImGui::IsItemHovered())
                    // {
                    //     auto metadata = m_algorithm_manager->get_current_metadata();
                            
                    //     if (metadata)
                    //     {
                    //         ImGui::BeginTooltip();
                    //         ImGui::TextColored(
                    //             ImVec4(1,1,0,1), "%s", 
                    //             metadata->get_display_name().c_str()
                    //         );
                    //         ImGui::Separator();
                    //         ImGui::TextWrapped(
                    //             "%s", metadata->get_description().brief.c_str()
                    //         );
                    //         ImGui::EndTooltip();
                    //     }
                    // }

                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
        }

        ImGui::End();
    }

    void AlgorithmVisualizerScene::render_algorithm_code_panel()
    {
        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        ImGui::Begin(
            "Pseudocode", 
            &m_show_algorithm_code_panel, 
            ImGuiWindowFlags_HorizontalScrollbar
        );

        auto display = m_algorithm_manager->get_current_pseudocode_with_highlights();
        auto* metadata = m_algorithm_manager->get_current_metadata();
        
        if (metadata)
        {
            // Header with algorithm name and complexity
            ImGui::TextColored(
                ImVec4(0.6f, 0.8f, 1.0f, 1.0f), 
                "%s", 
                metadata->get_display_name().c_str()
            );
            ImGui::SameLine();
            
            ImGui::Separator();
        }

        // Code view with line numbers
        ImVec2 available = ImGui::GetContentRegionAvail();
        ImGui::BeginChild(
            "CodeView", 
            ImVec2(available.x, available.y * 0.7f), 
            true
        );

        if (ImGui::BeginTable("CodeTable", 2,
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn(
                "#", ImGuiTableColumnFlags_WidthFixed, 
                40.0f
            );
            ImGui::TableSetupColumn(
                "Code", 
                ImGuiTableColumnFlags_WidthStretch
            );

            for (size_t i = 0; i < display.lines.size(); ++i)
            {
                size_t line_number = i + 1;
                bool highlighted = std::find(
                    display.highlighted_lines.begin(),
                    display.highlighted_lines.end(),
                    line_number) != display.highlighted_lines.end();

                auto it = display.line_index_values.find(line_number);
                const auto* vars = (it != display.line_index_values.end()) 
                                 ? &it->second : nullptr;

                render_code_line(line_number, display.lines[i], highlighted, vars);
            }

            ImGui::EndTable();
        }

        ImGui::EndChild();

        // Current operation details
        auto highlights = m_algorithm_manager->get_current_code_highlights();
        if (!highlights.empty())
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(
                ImVec4(1.0f, 0.8f, 0.2f, 1.0f), 
                "Current Operation"
            );
            ImGui::Spacing();

            ImGui::BeginChild(
                "OperationDetails", 
                ImVec2(0, 0), 
                true
            );

            for (const auto& h : highlights)
            {
                if (!h.is_active) continue;

                ImGui::TextWrapped("%s", h.description.c_str());
                ImGui::Spacing();

                // Variable values
                if (!h.index_variables.empty())
                {
                    ImGui::TextColored(
                        ImVec4(0.3f, 0.9f, 0.3f, 1.0f), 
                        "Indices:"
                    );
                    ImGui::Indent();
                    for (const auto& [k, v] : h.index_variables)
                        ImGui::Text("%s = %s", k.c_str(), v.c_str());
                    ImGui::Unindent();
                }

                if (!h.variable_values.empty())
                {
                    ImGui::TextColored(
                        ImVec4(0.3f, 0.9f, 0.3f, 1.0f), 
                        "Values:"
                    );
                    ImGui::Indent();
                    for (const auto& [k, v] : h.variable_values)
                        ImGui::Text("%s = %s", k.c_str(), v.c_str());
                    ImGui::Unindent();
                }

                // Code line
                ImGui::Spacing();
                ImGui::TextColored(
                    ImVec4(0.6f, 0.8f, 1.0f, 1.0f), 
                    "Code:"
                );
                ImGui::Indent();
                ImGui::Text("%s", h.code_line.c_str());
                ImGui::Unindent();
            }

            ImGui::EndChild();
        }

        ImGui::End();
    }

    void AlgorithmVisualizerScene::render_algorithm_description_panel()
    {
        auto* metadata = m_algorithm_manager->get_current_metadata();
        if (!metadata) return;

        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );
        ImGui::Begin(
            "Algorithm Description", 
            &m_show_algorithm_description_panel
        );

        render_algorithm_header(metadata);
        ImGui::Separator();

        // Brief description
        ImGui::TextWrapped("%s", metadata->get_description().brief.c_str());
        ImGui::Spacing();

        // Complexity badges
        render_complexity_badges(metadata->get_complexity());

        // Properties table
        render_properties_table(metadata->get_properties());

        // Tab bar for detailed sections
        if (ImGui::BeginTabBar("DescriptionTabs"))
        {
            // Detailed Analysis
            if (!metadata->get_description().detailed.empty())
            {
                if (ImGui::BeginTabItem("Detailed Analysis"))
                {
                    ImGui::BeginChild(
                        "DetailedScroll", 
                        ImVec2(0, 300), 
                        true
                    );
                    
                    for (const auto& line : metadata->get_description().detailed)
                    {
                        if (line.empty())
                        {
                            ImGui::Dummy(ImVec2(0, 2));
                        }
                        else if (line.find("──────────────────") != std::string::npos)
                        {
                            ImGui::Separator();
                        }
                        else if (line.find("•") != std::string::npos)
                        {
                            ImGui::Bullet();
                            ImGui::SameLine();
                            ImGui::TextWrapped("%s", line.substr(2).c_str());
                        }
                        else if (line.find(":") != std::string::npos && 
                                 line.length() < 30)
                        {
                            ImGui::TextColored(
                                ImVec4(1,1,0,1), 
                                "%s", 
                                line.c_str()
                            );
                        }
                        else
                        {
                            ImGui::TextWrapped("%s", line.c_str());
                        }
                    }
                    
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
            }

            // Optimizations
            if (!metadata->get_description().optimizations.empty())
            {
                if (ImGui::BeginTabItem("Optimizations"))
                {
                    for (const auto& opt : metadata->get_description().optimizations)
                    {
                        ImGui::Bullet();
                        ImGui::SameLine();
                        ImGui::TextWrapped("%s", opt.c_str());
                    }
                    ImGui::EndTabItem();
                }
            }

            // Use Cases
            if (!metadata->get_description().use_cases.empty())
            {
                if (ImGui::BeginTabItem("Use Cases"))
                {
                    for (const auto& use : metadata->get_description().use_cases)
                    {
                        ImGui::Bullet();
                        ImGui::SameLine();
                        ImGui::TextWrapped("%s", use.c_str());
                    }
                    ImGui::EndTabItem();
                }
            }

            // Disadvantages
            if (!metadata->get_description().disadvantages.empty())
            {
                if (ImGui::BeginTabItem("Limitations"))
                {
                    for (const auto& dis : metadata->get_description().disadvantages)
                    {
                        ImGui::Bullet();
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1,0.5f,0.5f,1), "%s", dis.c_str());
                    }
                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    void AlgorithmVisualizerScene::render_algorithm_variable_inspector_panel()
    {
        auto* algorithm = m_algorithm_manager->get_current_algorithm();
        auto* metadata = m_algorithm_manager->get_current_metadata();
        
        if (!algorithm || !metadata) return;

        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        ImGui::Begin(
            "Variable Inspector", 
            &m_show_algorithm_variable_inspector_panel
        );

        auto current_step = algorithm->get_current_step();
        auto variables = metadata->get_variable_info();

        for (const auto& var_info : variables)
        {
            // Get current value
            std::string value_str;
            
            if (auto val_string = current_step.metadata.get<std::string>(var_info.name))
                value_str = *val_string;
            else if (auto val_int = current_step.metadata.get<int>(var_info.name))
                value_str = std::to_string(*val_int);
            else if (auto val_size_t = current_step.metadata.get<size_t>(var_info.name))
                value_str = std::to_string(*val_size_t);
            else if (auto val_bool = current_step.metadata.get<bool>(var_info.name))
                value_str = *val_bool ? "true" : "false";
            else
                value_str = "N/A";

            // Render variable row
            ImGui::PushID(var_info.name.c_str());
            
            // Color indicator
            if (!var_info.color.empty())
            {
                ImU32 color = ImColor(
                    std::stoi(var_info.color.substr(1, 2), nullptr, 16),
                    std::stoi(var_info.color.substr(3, 2), nullptr, 16),
                    std::stoi(var_info.color.substr(5, 2), nullptr, 16),
                    255
                );
                    
                ImGui::ColorButton(
                    "##color", 
                    ImColor(color), 
                    ImGuiColorEditFlags_NoTooltip | 
                    ImGuiColorEditFlags_NoBorder, 
                    ImVec2(16, 16)
                );
                ImGui::SameLine();
            }

            ImGui::TextColored(
                ImVec4(0.8f, 0.8f, 0.8f, 1.0f), 
                "%s:", 
                var_info.display_name.c_str()
            );
            ImGui::SameLine();
            ImGui::TextColored(
                ImVec4(0.2f, 1.0f, 0.2f, 1.0f), 
                "%s", 
                value_str.c_str()
            );

            // Tooltip with description
            if (ImGui::IsItemHovered() && !var_info.description.empty())
            {
                ImGui::BeginTooltip();
                ImGui::TextColored(
                    ImVec4(1,1,0,1), 
                    "%s", 
                    var_info.display_name.c_str()
                );
                ImGui::Separator();
                ImGui::TextWrapped(
                    "%s", 
                    var_info.description.c_str()
                );
                ImGui::Text(
                    "Type: %s", 
                    var_info.type.c_str()
                );
                ImGui::EndTooltip();
            }

            ImGui::PopID();
        }

        ImGui::End();
    }

    // Rendering Helpers
    void AlgorithmVisualizerScene::render_algorithm_header(
        const algorithms::IAlgorithmMetadata* metadata)
    {
        m_icon_manager.render_icon(
            ui::managers::IconType::ALGORITHM, 
            {24, 24}
        );
        ImGui::SameLine();
        
        if (metadata)
        {
            core::utils::heading_colored_text(
                metadata->get_display_name().c_str()
            );
                
            ImGui::SameLine();
            ImGui::TextDisabled(
                "(%s)", 
                metadata->get_display_category().c_str()
            );
        }
        else
        {
            core::utils::heading_colored_text(
                "Algorithm Selector"
            );
        }
    }

    void AlgorithmVisualizerScene::render_complexity_badges(
        const algorithms::AlgorithmComplexityInfo& complexity)
    {
        ImGui::Text("Complexity");
        ImGui::Separator();

        if (ImGui::BeginTable(
                "ComplexityBadges", 
                2, 
                ImGuiTableFlags_SizingStretchSame)
            )
        {
            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(0,1,0,1), "Best:");
            ImGui::SameLine();
            ImGui::Text("%s", complexity.time_best.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(1,1,0,1), "Average:");
            ImGui::SameLine();
            ImGui::Text("%s", complexity.time_average.c_str());

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(1,0,0,1), "Worst:");
            ImGui::SameLine();
            ImGui::Text("%s", complexity.time_worst.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(ImVec4(0,1,1,1), "Space:");
            ImGui::SameLine();
            ImGui::Text("%s", complexity.space.c_str());

            ImGui::EndTable();
        }

        if (!complexity.explanation.empty())
        {
            ImGui::Spacing();
            ImGui::TextWrapped("%s", complexity.explanation.c_str());
        }
    }

    void AlgorithmVisualizerScene::render_properties_table(
        const algorithms::AlgorithmPropertiesInfo& properties)
    {
        ImGui::Spacing();
        ImGui::Text("Properties");
        ImGui::Separator();

        const auto& props = properties.get_as_key_value();
        
        if (ImGui::BeginTable(
                "PropertiesTable", 2, 
                ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)
            )
        {
            for (const auto& [name, value] : props)
            {
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", name.c_str());

                ImGui::TableSetColumnIndex(1);
                if (value)
                    ImGui::TextColored(ImVec4(0,1,0,1), "True");
                else
                    ImGui::TextColored(ImVec4(1,0,0,1), "False");
            }
            
            ImGui::EndTable();
        }
    }

    void AlgorithmVisualizerScene::render_code_line(
        size_t line_number,
        const std::string& line,
        bool highlighted,
        const std::unordered_map<std::string, std::string>* vars)
    {
        ImGui::TableNextRow();

        // Line number
        ImGui::TableNextColumn();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%zu", line_number);

        // Code
        ImGui::TableNextColumn();

        if (highlighted)
        {
            ImGui::PushStyleColor(
                ImGuiCol_Text, 
                ImVec4(1.0f, 1.0f, 0.0f, 1.0f)
            );
            ImGui::PushStyleColor(
                ImGuiCol_ChildBg, 
                ImVec4(0.2f, 0.2f, 0.0f, 0.3f)
            );
            
            ImGui::BeginChild(
                ImGui::GetID(line.c_str()), 
                ImVec2(0, ImGui::GetTextLineHeightWithSpacing()), 
                false, 
                ImGuiWindowFlags_NoScrollbar
            );
        }

        ImGui::Text("%s", line.c_str());

        if (vars && !vars->empty())
        {
            ImGui::SameLine();
            ImGui::Spacing();
            
            for (const auto& [k, v] : *vars)
            {
                ImGui::SameLine();
                ImGui::PushStyleColor(
                    ImGuiCol_Text, 
                    ImVec4(0.3f, 0.9f, 0.3f, 1.0f)
                );
                ImGui::Text("  %s=%s", k.c_str(), v.c_str());
                ImGui::PopStyleColor();
            }
        }

        if (highlighted)
        {
            ImGui::EndChild();
            ImGui::PopStyleColor(2);
        }
    }

    void AlgorithmVisualizerScene::render_algorithm_visualization_panel()
    {
        const auto* current_algorithm = 
            m_algorithm_manager->get_current_algorithm();
        if (!current_algorithm) return;

        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        ImGui::Begin(
            "Algorithm Visualization", 
            &m_show_algorithm_visualization_panel
        );

        // Visualization part - render below the controls
        auto* current_visualizer = 
            m_algorithm_manager->get_current_visualizer();
        if (current_visualizer)
        {
            ImGui::Separator();
            m_icon_manager.render_icon(
                ui::managers::IconType::ARRAY, 
                {25, 25}
            );
            ImGui::SameLine();
            core::utils::heading_colored_text(
                "Array Visualization"
            );

            current_visualizer->render();
        }

        ImGui::End();
    }

    void AlgorithmVisualizerScene::render_algorithm_stats_panel()
    {
        auto* algorithm = m_algorithm_manager->get_current_algorithm();
        auto* metadata  = m_algorithm_manager->get_current_metadata();

        if (!algorithm || !metadata) return;

        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        ImGui::Begin(
            "Algorithm Statistics", 
            &m_show_algorithm_stats_panel);

        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Statistics");
        ImGui::Separator();

        const auto current_step = algorithm->get_current_step();

        ImGui::Text("Algorithm: %s", metadata->get_display_name().c_str());
        ImGui::Text(
            "Category: %s",
            std::string(algorithms::algorithm_display_category(
                metadata->get_type())).c_str()
        );

        ImGui::Text("Current Step: %zu / %zu",
            algorithm->get_current_step_index(),
            algorithm->get_step_count());

        ImGui::Text("Complete: %s",
            algorithm->is_complete() ? "Yes" : "No");

        ImGui::Separator();
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Performance");

        const auto& complexity = metadata->get_complexity();

        ImGui::Text("Time Complexity (Best): %s",
            complexity.time_best.c_str());

        ImGui::Text("Space Complexity: %s",
            complexity.space.c_str());

        if (metadata->get_category() == algorithms::AlgorithmCategory::SORTING) 
        {
            ImGui::Text(
                "Estimated Comparisons: %zu",
                current_step.visualization.comparisons
            );
            ImGui::Text(
                "Estimated Swaps: %zu",
                current_step.visualization.swaps
            );
        }

        ImGui::End();
    }

    void AlgorithmVisualizerScene::setup_shortcuts_tooltip()
    {
        // Todo
    }

    void AlgorithmVisualizerScene::render_algorithm_control_panel()
    {
        auto* current_algorithm = m_algorithm_manager->get_current_algorithm();
        if (!current_algorithm) return;
        
        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        ImGui::Begin(
            "Playback Controls", 
            &m_show_algorithm_control_panel
        );

        // Play/Pause button
        auto play_pause_icon = m_algorithm_manager->is_playing()
            ? ui::managers::IconType::PAUSE
            : ui::managers::IconType::PLAY;

        render_icon_button(
            "play_pause_btn",
            play_pause_icon,
            [this]()
            {
                if (m_algorithm_manager->is_playing()) 
                {
                    m_algorithm_manager->pause();
                } 
                else {
                    m_algorithm_manager->play();
                }
            }
        );

        ImGui::SameLine();

        // Checking if step controls should be enabled
        bool step_controls_enabled = (!m_algorithm_manager->is_executing() ||
                                        m_algorithm_manager->is_paused());

        // Step backward button
        render_icon_button(
            "step_backward_btn",
            ui::managers::IconType::STEP_BACKWARD,
            [this]() { m_algorithm_manager->step_backward(); },
            ImVec2(0, 0), step_controls_enabled, "Step backward"
        );

        ImGui::SameLine();

        // Progress bar
        const float progress = current_algorithm->get_step_count() > 0 ?
            static_cast<float>(current_algorithm->get_current_step_index()) /
            static_cast<float>(current_algorithm->get_step_count() - 1) : 0.0f;

        ImGui::ProgressBar(progress, ImVec2(200, PROGRESS_BAR_HEIGHT + 5));
        ImGui::SameLine();

        // Step forward button
        render_icon_button(
            "step_forward_btn",
            ui::managers::IconType::STEP_FORWARD,
            [this]() { m_algorithm_manager->step_forward(); },
            ImVec2(0, 0), step_controls_enabled, "Step forward"
        );

        ImGui::SameLine();

        // Reset button
        render_icon_button(
            "reset_btn",
            ui::managers::IconType::RESET,
            [this]() { m_algorithm_manager->stop(); },
            ImVec2(0, 0), true, "Reset"
        );

        ImGui::SameLine();

        // Speed control
        ImGui::Text("Speed:");
        ImGui::SameLine();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 13.0f);

        ImGui::SetNextItemWidth(SLIDER_WIDTH);
        float current_speed = m_algorithm_manager->get_speed();
        if (ImGui::SliderFloat("##Speed", &current_speed, 0.1f, 5.0f, "%.1fx"))
        {
            m_algorithm_manager->set_speed(current_speed);
        }

        ImGui::PopStyleVar(2);
        
        ImGui::End();
    }

    void AlgorithmVisualizerScene::render_algorithm_data_control_panel()
    {
        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        ImGui::Begin(
            "Data Controls", 
            &m_show_algorithm_data_control_panel
        );
        
        if (ImGui::Button("Generate Random Data")) {
            m_algorithm_manager->generate_and_set_random_data();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Small Data (5)")) {
            std::vector<int> small_data;
            for (int i = 0; i < 5; ++i) {
                small_data.push_back(rand() % 50 + 1);
            }
            m_algorithm_manager->set_data(small_data);
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Large Data (15)")) {
            std::vector<int> large_data;
            for (int i = 0; i < 15; ++i) {
                large_data.push_back(rand() % 100 + 1);
            }
            m_algorithm_manager->set_data(large_data);
        }
        
        // TODO:
        // Preset data sets
        ImGui::Separator();
        ImGui::Text("Preset Data Sets:");
        
        if (ImGui::Button("Sorted Data")) {
            std::vector<int> sorted_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            m_algorithm_manager->set_data(sorted_data);
        }
        
        ImGui::SameLine();
        
        // TODO:
        if (ImGui::Button("Reverse Sorted")) {
            std::vector<int> reverse_data = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
            m_algorithm_manager->set_data(reverse_data);
        }
        
        ImGui::SameLine();
        
        // TODO:
        if (ImGui::Button("All Equal")) {
            std::vector<int> equal_data = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
            m_algorithm_manager->set_data(equal_data);
        }
        
        ImGui::End();
    }

    void AlgorithmVisualizerScene::render_thread_info_panel()
    {
        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );
        
        ImGui::Begin(
            "Thread Information", 
            &m_show_thread_info_panel
        );

        ImGui::Text("Threading System:");
        ImGui::Separator();

        // Thread manager status
        if (m_thread_manager.is_running()) 
        {
            ImGui::TextColored(
                ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 
                "Thread Manager: ACTIVE"
            );
        } else 
        {
            ImGui::TextColored(
                ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                "Thread Manager: INACTIVE"
            );
        }

        // Algorithm execution status
        if (m_algorithm_manager) 
        {
            ImGui::Text(
                "Background Execution: %s",
                m_algorithm_manager->is_executing() ? "ACTIVE" : "INACTIVE"
            );

            ImGui::Text(
                "Playback: %s",
                m_algorithm_manager->is_playing() ? "PLAYING" : "PAUSED"
            );

            ImGui::Text(
                "Speed: %.1fx",  
                static_cast<double>(m_algorithm_manager->get_speed())
            );

            // Performance info
            ImGui::Separator();
            ImGui::Text("Performance:");
            ImGui::Text("UI Thread: Main");
            ImGui::Text(
                "Algorithm Thread: %s",
                m_algorithm_manager->is_executing() ? "Background" : "Main"
            );
        }

        // Controls
        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::Text("Space: Play/Pause");
        ImGui::Text("Arrow Keys: Step Forward/Back");
        ImGui::Text("R: Reset");
        ImGui::Text("N: New Data");
        ImGui::Text("T: Toggle Threading");

        ImGui::End();
    }

} // namespace c2l::scenes