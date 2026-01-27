#include "scenes/algorithm_visualizer_scene.hpp"
#include "algorithms/algorithm_types.hpp"
#include "core/utils/utils.hpp"
#include "core/utils/logger/logger.hpp"
#include "core/utils/variables.hpp"

namespace c2l::scenes
{
    AlgorithmVisualizerScene::AlgorithmVisualizerScene(
        graphics::Renderer& renderer,
        core::ThreadManager& thread_manager,
        core::resources::ResourceManager& resource_manager,
        c2l::ui::managers::IconManager& icon_manager)
        : BaseScene(renderer, thread_manager, resource_manager, icon_manager)
        , m_algorithm_manager{std::make_unique<algorithms::AlgorithmManager>(thread_manager)}
    {
        LOG_INFO("AlgorithmVisualizerScene created");

        on_create();
        setup_main_menu();

        // setup_icons();
    }

    void AlgorithmVisualizerScene::on_create()
    {
        setup_ui_components();

        // Loading default algorithm
        m_algorithm_manager->load_algorithm(algorithms::AlgorithmType::BUBBLE_SORT);

        m_algorithm_manager->generate_and_set_random_data();

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

    void AlgorithmVisualizerScene::process_input(const core::InputHandler& input)
    {
        if (input.is_key_just_pressed(GLFW_KEY_SPACE))
        {
            if (m_algorithm_manager->is_playing()) {
                m_algorithm_manager->pause();
            } else {
                m_algorithm_manager->play();
            }
        }
        
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

        if (input.is_key_just_pressed(GLFW_KEY_ESCAPE))
        {
            request_scene_pop();  // Using callback to switch back to the main scene, rather than SceneManager dependency
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

        if (m_algorithm_manager)
        {
            m_algorithm_manager->render();
        }

        // // Render our algorithm visualization components
        if (m_show_algorithm_selector)
        {
            render_algorithm_selector();
        }

        if (m_show_algorithm_visualizer)
        {
            render_algorithm_visualizer();
        }

        if (m_show_controls)
        {
            render_controls();
        }

        if (m_show_data_controls)
        {
            render_data_controls();
        }

        if (m_show_thread_info)
        {
            render_thread_info();
        }

        if (m_show_stats_panel)
        {
            render_stats_panel();
        }

        if (m_show_code_panel)
        {
            render_code_panel();
        }

        if (m_show_algorithm_description)
        {
            render_algorithm_description();

        }

        render_common_ui();
        m_renderer.clear();
    }

    void AlgorithmVisualizerScene::setup_main_menu()
    {
        // Common menu items for all scenes
        std::vector<ui::components::MainMenu::MenuItem> view_items =
        {
            {"ImGui Demo", []() { std::cout << "ShowDemoWindow\n"; }, &m_show_demo_window, "F1"},
            {"Algorithm Selector", []() { }, &m_show_algorithm_selector, "F2"},
            {"Algorithm Controls", []() { }, &m_show_controls, "F3"},
            {"Algorithm Visualizer", []() { }, &m_show_algorithm_visualizer, "F3"},
            {"Algorithm Stats", []() { }, &m_show_stats_panel, "F2"},
            {"Algorithm Description", []() { }, &m_show_algorithm_description, "F2"},
            {"Algorithm Code", []() { }, &m_show_code_panel, "F2"},
        };

        m_main_menu->add_menu("View", std::move(view_items));
    }

    void AlgorithmVisualizerScene::render_algorithm_selector()
    {
        ImGui::Begin("Algorithm Selector", &m_show_algorithm_selector);

        m_icon_manager.render_icon(ui::managers::IconType::ALGORITHM, {30, 30});
        ImGui::SameLine();
        core::utils::heading_colored_text("Algorithm Selector");
        ImGui::Separator();

        const auto& name_to_type_map = m_algorithm_manager->get_name_to_type_map();
        const auto& algorithm_names = m_algorithm_manager->get_algorithm_names();

        // Group by category
        std::unordered_map<algorithms::AlgorithmCategory, std::vector<std::string>> categorized_algorithms;

        for (const auto& name : algorithm_names)
        {
            auto it = name_to_type_map.find(name);
            if (it != name_to_type_map.end())
            {
                auto type = it->second;
                auto category = algorithms::get_algorithm_category(type);
                categorized_algorithms[category].push_back(name);
            }
        }

        for (const auto& [category, algorithms] : categorized_algorithms)
        {
            std::string category_name = algorithm_category_to_string(category);
            if (ImGui::TreeNode(category_name.c_str()))
            {
                for (const auto& algorithm_name : algorithms)
                {
                    bool is_selected = (algorithm_name == m_algorithm_manager->get_current_algorithm_name());
                    if (ImGui::Selectable(algorithm_name.c_str(), is_selected))
                    {
                        m_algorithm_manager->load_algorithm(algorithm_name);
                        m_algorithm_manager->on_step_changed();
                    }
                }
                ImGui::TreePop();
            }
        }

        ImGui::End();
    }

    void AlgorithmVisualizerScene::render_algorithm_visualizer()
    {
        const auto* current_algorithm = m_algorithm_manager->get_current_algorithm();
        if (!current_algorithm) return;

        ImGui::Begin("Algorithm Visualization", &m_show_algorithm_visualizer);

            // Play/Pause button
            auto play_pause_icon = m_algorithm_manager->is_playing()
                ? ui::managers::IconType::PAUSE
                : ui::managers::IconType::PLAY;

            render_icon_button(
                "play_pause_btn",
                play_pause_icon,
                [this]()
                    {
                        if (m_algorithm_manager->is_playing()) {
                            m_algorithm_manager->pause();
                        } else {
                            m_algorithm_manager->play();
                        }
                    }
            );

            ImGui::SameLine();

        // Check if step controls should be enabled
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

        // Visualization part - render below the controls
        auto* current_visualizer = m_algorithm_manager->get_current_visualizer();
        if (current_visualizer)
        {
            ImGui::Separator();
            m_icon_manager.render_icon(ui::managers::IconType::ARRAY);
            ImGui::SameLine();
            core::utils::heading_colored_text("Array Visualization", {1.0f, 1.0f, 0.0f, 1.0f});

            current_visualizer->render();
        }

        render_comparison_analysis(current_algorithm->get_current_step());
        render_performance_metrics();

        ImGui::End();
    }

    void AlgorithmVisualizerScene::render_stats_panel()
    {
        auto* current_algorithm = m_algorithm_manager->get_current_algorithm();
        if (!current_algorithm) return;

        ImGui::Begin("Algorithm Statistics", &m_show_stats_panel);

        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "📊 Statistics");
        ImGui::Separator();

        const auto current_step = current_algorithm->get_current_step();

        ImGui::Text("Algorithm: %s", m_algorithm_manager->get_current_algorithm_name().c_str());
        ImGui::Text("Category: %s", algorithm_category_to_string(current_algorithm->get_category()));
        ImGui::Text("Current Step: %zu / %zu",
                   current_algorithm->get_current_step_index(),
                   current_algorithm->get_step_count());
        ImGui::Text("Complete: %s", current_algorithm->is_complete() ? "Yes" : "No");
        ImGui::Text("Thread: %s", m_algorithm_manager->is_executing() ? "🔄 Background" : "📱 Main");

        // Algorithm-specific stats
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "📈 Performance");
        ImGui::Text("Time Complexity: %s", current_algorithm->get_time_complexity().c_str());
        ImGui::Text("Space Complexity: %s", current_algorithm->get_space_complexity().c_str());

        if (current_algorithm->get_category() == algorithms::AlgorithmCategory::SORTING) {
            ImGui::Text("Estimated Comparisons: %zu", current_step.visualization.comparisons);
            ImGui::Text("Estimated Swaps: %zu", current_step.visualization.swaps);
        }

        ImGui::End();
    }


    void AlgorithmVisualizerScene::render_code_panel()
{
    ImGui::Begin("Algorithm Code", &m_show_code_panel);

    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "💻 Pseudocode");
    ImGui::Separator();

    auto display = m_algorithm_manager->get_current_pseudocode_with_highlights();

    // --- CODE VIEW ---
    ImVec2 available = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("CodeView", ImVec2(available.x, available.y * 0.6f), true);

    if (ImGui::BeginTable("CodeTable", 2,
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 40.f);
        ImGui::TableSetupColumn("Code", ImGuiTableColumnFlags_WidthStretch);

        for (size_t i = 0; i < display.lines.size(); ++i)
        {
            size_t line_number = i + 1;
            bool highlighted =
                std::find(display.highlighted_lines.begin(),
                          display.highlighted_lines.end(),
                          line_number) != display.highlighted_lines.end();

            auto it = display.line_index_values.find(line_number);
            const auto* vars = (it != display.line_index_values.end()) ? &it->second : nullptr;

            draw_code_line(line_number, display.lines[i], highlighted, vars);
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();

    // --- CURRENT OPERATION ---
    auto highlights = m_algorithm_manager->get_current_code_highlights();
    if (!highlights.empty())
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.f, 0.6f, 0.2f, 1.f), "🎯 Current Operation");

        ImGui::BeginChild("OperationPanel", ImVec2(0, 0), true);

        for (const auto& h : highlights)
        {
            if (!h.is_active) continue;

            ImGui::TextColored(ImVec4(1.f, 1.f, 0.4f, 1.f), "%s", h.description.c_str());

            if (!h.index_variables.empty())
            {
                ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.f), "Indices:");
                ImGui::Indent();
                for (auto& [k, v] : h.index_variables)
                    ImGui::Text("%s = %s", k.c_str(), v.c_str());
                ImGui::Unindent();
            }

            if (!h.variable_values.empty())
            {
                ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.f), "Values:");
                ImGui::Indent();
                for (auto& [k, v] : h.variable_values)
                    ImGui::Text("%s = %s", k.c_str(), v.c_str());
                ImGui::Unindent();
            }

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.f), "Code:");
            ImGui::Indent();
            ImGui::Text("%s", h.code_line.c_str());
            ImGui::Unindent();

        }

        ImGui::EndChild();
    }

    ImGui::End();
}


    void AlgorithmVisualizerScene::render_algorithm_description()
    {
        auto* current_algorithm = m_algorithm_manager->get_current_algorithm();
        if (!current_algorithm) return;

        ImGui::Begin("Algorithm Description", &m_show_algorithm_description);

        ImGui::Separator();

        // Header with algorithm icon and name
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 1.0f, 1.0f));
        ImGui::Text("📋 Algorithm Description");
        ImGui::PopStyleColor();

        ImGui::BeginChild("AlgorithmDescription", ImVec2(0, 0), true,
                         ImGuiWindowFlags_AlwaysVerticalScrollbar);

        // Get detailed description
        auto description_lines = current_algorithm->get_detailed_description();
        auto properties = current_algorithm->get_properties();

        // Render description lines with proper formatting
        for (const auto& line : description_lines)
        {
            if (line.empty())
            {
                ImGui::Dummy(ImVec2(0, 2)); // Add spacing for empty lines
                continue;
            }

            // Detect and format different line types
            if (line.find("──────────────────") != std::string::npos)
            {
                ImGui::Separator();
            }
            else if (line.find(":") != std::string::npos &&
                     line.find("•") == std::string::npos &&
                     line.find("Algorithm Analysis") == std::string::npos)
            {
                // Property lines (Key: Value)
                size_t colon_pos = line.find(":");
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);

                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s:", key.c_str());
                ImGui::SameLine();
                ImGui::TextWrapped("%s", value.c_str());
            }
            else if (line.find("•") != std::string::npos)
            {
                // Bullet points
                ImGui::Bullet();
                ImGui::TextWrapped("%s", line.substr(2).c_str()); // Skip "• "
            }
            else
            {
                // Regular text
                ImGui::TextWrapped("%s", line.c_str());
            }
        }

        // Render properties table if available
        if (!properties.empty())
        {
            ImGui::Dummy(ImVec2(0, 8));
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Algorithm Properties:");

            if (ImGui::BeginTable("Properties", 2,
                ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit))
            {
                for (const auto& [key, value] : properties)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "%s", key.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", value.c_str());
                }
                ImGui::EndTable();
            }
        }

        ImGui::EndChild();

        ImGui::End();
    }

    void AlgorithmVisualizerScene::render_comparison_analysis(const c2l::algorithms::AlgorithmStep& step)
    {
        ImGui::Separator();
        // m_icon_manager.render_icon(ui::managers::IconType::STEPS, {30, 30});
        // ImGui::SameLine();
        core::utils::heading_colored_text("Step Analysis");

        // Adding some vertical space before and after the text
        ImGui::Dummy({0, 5});
        ImGui::TextWrapped("%s", step.description.c_str());
        ImGui::Dummy({0, 5});

        // Detailed comparison information
        // if (step.visualization.highlighted_index != static_cast<size_t>(-1) &&
        //     step.visualization.compared_index != static_cast<size_t>(-1))
        // {
        //     ImGui::Text("Comparing: array[%zu] = %d vs array[%zu] = %d",
        //                step.visualization.highlighted_index,
        //                step.data[step.visualization.highlighted_index],
        //                step.visualization.compared_index,
        //                step.data[step.visualization.compared_index]);
        // }
    }

    void AlgorithmVisualizerScene::render_performance_metrics()
    {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "📈 Performance Metrics");

        // Use a responsive table that adapts to content
        const float available_width = ImGui::GetContentRegionAvail().x;
        const bool use_single_column = available_width < 400; // Switching to single column on narrow screens

        const int columns = use_single_column ? 1 : 2;

        if (ImGui::BeginTable("Metrics", columns,
            ImGuiTableFlags_SizingStretchSame |
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_BordersInnerV))
        {
            // First row: Complexity metrics
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            render_metric_card("Time Complexity",
                              m_algorithm_manager->get_current_algorithm()->get_time_complexity(),
                              ImVec4(1.0f, 1.0f, 0.0f, 1.0f));

            if (!use_single_column) {
                ImGui::TableSetColumnIndex(1);
                render_metric_card("Space Complexity",
                                  m_algorithm_manager->get_current_algorithm()->get_space_complexity(),
                                  ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            }

            // Second row: Step metrics (or continue in single column)
            if (use_single_column) {
                ImGui::TableNextRow();
            } else {
                ImGui::TableNextRow();
            }

            ImGui::TableSetColumnIndex(use_single_column ? 0 : 0);
            render_metric_card("Current Step",
                              std::to_string(m_algorithm_manager->get_current_algorithm()->get_current_step_index()),
                              ImVec4(0.0f, 1.0f, 1.0f, 1.0f));

            if (!use_single_column) {
                ImGui::TableSetColumnIndex(1);
                render_metric_card("Total Steps",
                                  std::to_string(m_algorithm_manager->get_current_algorithm()->get_step_count()),
                                  ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
            }

            // Description of the algorithm goes here
            // Todo

            ImGui::EndTable();
        }
    }

    void AlgorithmVisualizerScene::render_metric_card(const std::string& title,
                                                  const std::string& value,
                                                  const ImVec4& color)
    {
        const float available_width = ImGui::GetContentRegionAvail().x;
        const float card_width = available_width * 0.95f; // 95% of available space
        const float card_height = 60.0f;

        ImGui::BeginChild(title.c_str(), ImVec2(card_width, card_height), true,
                         ImGuiWindowFlags_NoScrollbar);

        // Title with ellipsis if too long
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + card_width - 10.0f);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", title.c_str());
        ImGui::PopTextWrapPos();

        // Value with automatic sizing and ellipsis
        const ImVec2 value_size = ImGui::CalcTextSize(value.c_str());
        const float max_value_width = card_width - 20.0f;

        if (value_size.x > max_value_width) {
            // Use text wrapping or ellipsis for long values
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + max_value_width);
            ImGui::TextColored(color, "%s", value.c_str());
            ImGui::PopTextWrapPos();
        } else {
            // Center the value if it fits
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (card_width - value_size.x) * 0.5f);
            ImGui::TextColored(color, "%s", value.c_str());
        }

        ImGui::EndChild();
    }


    void AlgorithmVisualizerScene::render_controls()
    {
        auto* algorithm = m_algorithm_manager->get_current_algorithm();
        if (!algorithm) return;
        
        ImGui::Begin("Playback Controls", &m_show_controls);

        // Threading toggle
        ImGui::Text("Execution Mode:");
        if (ImGui::Button(m_algorithm_manager->is_executing() ? "Background Thread" : "Main Thread")) {
            if (m_algorithm_manager->is_executing()) {
                m_algorithm_manager->stop_background_execution();
            } else {
                m_algorithm_manager->start_background_execution();
            }
        }

        ImGui::Separator();

        if (ImGui::Button(m_algorithm_manager->is_playing() ? "Pause" : "Play")) {
            if (m_algorithm_manager->is_playing()) {
                m_algorithm_manager->pause();
            } else {
                m_algorithm_manager->play();
            }
        }
        
        ImGui::SameLine();
        
        // Step controls (only relevant when not using background execution)
        if (!m_algorithm_manager->is_executing() || m_algorithm_manager->is_paused()) {
            if (ImGui::Button("Step Forward") && !algorithm->is_complete()) {
                m_algorithm_manager->step_forward();
            }

            ImGui::SameLine();

            if (ImGui::Button("Step Back") && algorithm->get_current_step_index() > 0) {
                m_algorithm_manager->step_backward();
            }
        } else {
            // Disabled step controls when background thread is active
            ImGui::BeginDisabled();
            ImGui::Button("Step Forward");
            ImGui::SameLine();
            ImGui::Button("Step Back");
            ImGui::EndDisabled();
        }
        
        ImGui::SameLine();
        
        // Reset button
        if (ImGui::Button("Reset")) {
            m_algorithm_manager->stop();
            m_algorithm_manager->generate_and_set_random_data();
        }

        // Speed control
        float speed = 1.0f; // You might want to add speed getter/setter to algorithm manager
        if (ImGui::SliderFloat("Speed", &speed, 0.1f, 5.0f)) {
            m_algorithm_manager->set_speed(speed);
        }
        
        // Progress bar
        float progress = algorithm->get_step_count() > 0 ? 
            static_cast<float>(algorithm->get_current_step_index()) / (algorithm->get_step_count() - 1) : 0.0f;
        ImGui::ProgressBar(progress, ImVec2(-1, 20));
        
        // Algorithm info
        ImGui::Separator();
        ImGui::Text("Algorithm: %s", m_algorithm_manager->get_current_algorithm_name().c_str());
        ImGui::Text("Steps: %zu / %zu", algorithm->get_current_step_index(), algorithm->get_step_count());
        ImGui::Text("Complete: %s", algorithm->is_complete() ? "Yes" : "No");
        ImGui::Text("Thread: %s", m_algorithm_manager->is_executing() ? "Background" : "Main");
        
        ImGui::End();
    }

    void AlgorithmVisualizerScene::render_data_controls()
    {
        ImGui::Begin("Data Controls", &m_show_data_controls);
        
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
        
        // Preset data sets
        ImGui::Separator();
        ImGui::Text("Preset Data Sets:");
        
        if (ImGui::Button("Sorted Data")) {
            std::vector<int> sorted_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            m_algorithm_manager->set_data(sorted_data);
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Reverse Sorted")) {
            std::vector<int> reverse_data = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
            m_algorithm_manager->set_data(reverse_data);
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("All Equal")) {
            std::vector<int> equal_data = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
            m_algorithm_manager->set_data(equal_data);
        }
        
        ImGui::End();
    }

    void AlgorithmVisualizerScene::render_thread_info()
    {
        ImGui::Begin("Thread Information", &m_show_thread_info);

        ImGui::Text("Threading System:");
        ImGui::Separator();

        // Thread manager status
        if (m_thread_manager.is_running()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Thread Manager: ACTIVE");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Thread Manager: INACTIVE");
        }

        // Algorithm execution status
        if (m_algorithm_manager) {
            ImGui::Text("Background Execution: %s",
                m_algorithm_manager->is_executing() ? "ACTIVE" : "INACTIVE");

            ImGui::Text("Playback: %s",
                m_algorithm_manager->is_playing() ? "PLAYING" : "PAUSED");

            ImGui::Text(
                "Speed: %.1fx", 
                m_algorithm_manager->is_playing() 
                    ? static_cast<double>(m_algorithm_manager->get_speed())
                    : 1.0);

            // Performance info
            ImGui::Separator();
            ImGui::Text("Performance:");
            ImGui::Text("UI Thread: Main");
            ImGui::Text("Algorithm Thread: %s",
                m_algorithm_manager->is_executing() ? "Background" : "Main");
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

    void AlgorithmVisualizerScene::draw_code_line(
        size_t line_number,
        const std::string& line,
        bool highlighted,
        const std::unordered_map<std::string, std::string>* vars)
    {
        ImGui::TableNextRow();

        // Line number
        ImGui::TableNextColumn();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f), "%zu", line_number);

        // Code
        ImGui::TableNextColumn();

        if (highlighted)
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.25f, 0.25f, 0.0f, 0.6f));
            // ImGui::BeginChild(ImGui::GetID(line.c_str()), ImVec2(0, 0), false);
            ImGui::TextColored(ImVec4(1.f, 1.f, 0.6f, 1.f), "%s", line.c_str());
        }
        else
        {
            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.f), "%s", line.c_str());
        }

        // Inline variables
        if (vars && !vars->empty())
        {
            ImGui::SameLine();
            ImGui::Spacing();
            for (const auto& [k, v] : *vars)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.f), "%s=%s", k.c_str(), v.c_str());
            }
        }

        if (highlighted)
        {
            // ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    }





} // namespace c2l::scenes