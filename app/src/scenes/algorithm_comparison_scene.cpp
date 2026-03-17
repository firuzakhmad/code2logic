#include "scenes/algorithm_comparison_scene.hpp"
#include "core/utils/logger/logger.hpp"
#include "core/utils/variables.hpp"

#include "imgui.h"

namespace c2l::scenes
{
    AlgorithmComparisonScene::AlgorithmComparisonScene(
        graphics::Renderer& renderer,
        core::ThreadManager& thread_manager,
        core::JsonConfigManager& json_config_manager,
        algorithms::AlgorithmRegistry& algorithm_registry,
        core::resources::ResourceManager& resource_manager,
        ui::managers::IconManager& icon_manager)
        : BaseScene{
            renderer,
            thread_manager,
            json_config_manager,
            algorithm_registry,
            resource_manager,
            icon_manager
        }
        , m_comparison_manager{
            std::make_unique<algorithms::ParallelComparisonManager>(
                thread_manager,
                algorithm_registry)
        }
    {
        LOG_INFO("AlgorithmComparisonScene created");
    }

    void AlgorithmComparisonScene::on_create()
    {
        setup_ui_components();
        setup_main_menu();
        update_algorithm_cache();

        // Initialize with default algorithms
        m_comparison_manager->set_algorithm_left(m_left_selected);
        m_comparison_manager->set_algorithm_right(m_right_selected);
        m_comparison_manager->generate_random_data(
            static_cast<size_t>(m_data_size),
            m_max_value
        );

        // Initializing visualizer
        sync_visualizers();

        LOG_INFO("AlgorithmComparisonScene initialized");
    }

    void AlgorithmComparisonScene::on_destroy()
    {
        m_ui_manager->unregister_all_components();
        LOG_DEBUG("AlgorithmComparisonScene destroyed");
    }

    void AlgorithmComparisonScene::on_activate()
    {
        if (!is_created())
        {
            on_create();
            mark_created();
        }

        // Refreshing cache
        update_algorithm_cache();
        sync_visualizers();
    }

    void AlgorithmComparisonScene::on_deactivate()
    {
        m_ui_manager->hide_all_panels();
        m_ui_manager->close_all_popups();

        if (m_comparison_manager)
        {
            m_comparison_manager->pause();
        }
    }

    void AlgorithmComparisonScene::process_input(const core::InputHandler& input)
    {
        // Playback controls
        if (input.is_key_just_pressed(GLFW_KEY_SPACE))
        {
            if (m_comparison_manager->is_playing())
                m_comparison_manager->pause();
            else
                m_comparison_manager->play();
        }

        if (input.is_key_just_pressed(GLFW_KEY_RIGHT))
        {
            m_comparison_manager->step_forward();
        }

        if (input.is_key_just_pressed(GLFW_KEY_LEFT))
        {
            m_comparison_manager->step_backward();
        }

        if (input.is_key_just_pressed(GLFW_KEY_R))
        {
            m_comparison_manager->reset();
        }

        if (input.is_key_just_pressed(GLFW_KEY_N))
        {
            m_comparison_manager->generate_random_data(
                static_cast<size_t>(m_data_size),
                m_max_value
            );
        }

        // Exit
        if (input.is_key_just_pressed(GLFW_KEY_ESCAPE))
        {
            request_scene_pop();
        }
    }

    void AlgorithmComparisonScene::update(double dt)
    {
        m_ui_manager->update(dt);
        m_animation_time += dt;

        if (m_comparison_manager)
        {
            m_visualizer.update(dt);
        }

        m_icon_manager.update();
    }

    void AlgorithmComparisonScene::render()
    {
        m_renderer.render();
        m_ui_manager->render();

        // Render all panels
        if (m_show_selection_panel)
            render_selection_panel();

        if (m_show_comparison_view)
            render_comparison_view_panel();

        if (m_show_control_panel)
            render_control_panel();

        if (m_show_data_control)
            render_data_control_panel();

        if (m_show_results_panel)
            render_results_panel();

        if (m_show_performance_graphs)
            render_performance_graphs();

        render_common_ui();
        m_renderer.clear();
    }

    void AlgorithmComparisonScene::render_selection_panel()
    {
        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        ImGui::Begin(
            "Algorithm Selection",
            &m_show_selection_panel,
            ImGuiWindowFlags_NoCollapse
        );

        // Left algorithm selection
        ImGui::TextColored(
            ImVec4(0.2f, 0.8f, 1.0f, 1.0f), 
            "Left Algorithm"
        );
        ImGui::Separator();

        if (ImGui::BeginCombo("##LeftAlgorithm",
            algorithm_display_name(m_left_selected).data()))
        {
            for (const auto& algo : m_algorithm_cache)
            {
                bool is_selected = (m_left_selected == algo.type);
                if (ImGui::Selectable(algo.name.c_str(), is_selected))
                {
                    m_left_selected = algo.type;
                    m_comparison_manager->set_algorithm_left(algo.type);
                    sync_visualizers();
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // Right algorithm selection
        ImGui::TextColored(
            ImVec4(0.2f, 0.8f, 1.0f, 1.0f), 
            "Right Algorithm"
        );
        ImGui::Separator();

        if (ImGui::BeginCombo(
            "##RightAlgorithm",
            algorithms::algorithm_display_name(m_right_selected).data())
        )
        {
            for (const auto& algo : m_algorithm_cache)
            {
                bool is_selected = (m_right_selected == algo.type);
                if (ImGui::Selectable(algo.name.c_str(), is_selected))
                {
                    m_right_selected = algo.type;
                    m_comparison_manager->set_algorithm_right(algo.type);
                    sync_visualizers();
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Swap button
        ImGui::Spacing();
        if (ImGui::Button("Swap Algorithms", ImVec2(-1, 0)))
        {
            std::swap(m_left_selected, m_right_selected);
            m_comparison_manager->set_algorithm_left(m_left_selected);
            m_comparison_manager->set_algorithm_right(m_right_selected);
            sync_visualizers();
        }

        ImGui::End();
    }

    void AlgorithmComparisonScene::render_comparison_view_panel()
    {
        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        ImGui::Begin("Comparison View", &m_show_comparison_view,
                    ImGuiWindowFlags_NoScrollbar);

        // Get algorithms from manager
        const auto& left = m_comparison_manager->get_left_algorithm();
        const auto& right = m_comparison_manager->get_right_algorithm();

        // Update visualizer if needed
        if (left.algorithm.get() != m_visualizer.m_left_algorithm ||
            right.algorithm.get() != m_visualizer.m_right_algorithm)
        {
            m_visualizer.initialize(
                left.algorithm.get(), left.metadata,
                right.algorithm.get(), right.metadata
            );
        }

        // Render side-by-side visualization
        m_visualizer.render();

        ImGui::End();
    }

    void AlgorithmComparisonScene::render_control_panel()
    {
        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        ImGui::Begin("Comparison Controls", &m_show_control_panel);

        // Playback controls
        if (ImGui::Button(
            m_comparison_manager->is_playing() ? "Pause" : "Play",
            ImVec2(BUTTON_WIDTH, 0)))
        {
            if (m_comparison_manager->is_playing())
                m_comparison_manager->pause();
            else
                m_comparison_manager->play();
        }

        ImGui::SameLine();

        if (ImGui::Button("Stop", ImVec2(BUTTON_WIDTH, 0)))
        {
            m_comparison_manager->stop();
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset", ImVec2(BUTTON_WIDTH, 0)))
        {
            m_comparison_manager->reset();
        }

        // Step controls
        ImGui::Spacing();

        if (ImGui::Button("Step Back", ImVec2(BUTTON_WIDTH, 0)))
        {
            m_comparison_manager->step_backward();
        }

        ImGui::SameLine();

        if (ImGui::Button("Step Forward", ImVec2(BUTTON_WIDTH, 0)))
        {
            m_comparison_manager->step_forward();
        }

        // Speed control
        ImGui::Spacing();
        float speed = m_comparison_manager->get_speed();
        if (ImGui::SliderFloat(
            "Speed",
            &speed,
            ALGORITHM_COMPUTATION_MIN_STEEP,
            ALGORITHM_COMPUTATION_MAX_STEEP,
            "%.1fx"))
        {
            m_comparison_manager->set_speed(speed);
        }

        // Progress bars
        ImGui::Spacing();
        const auto& left = m_comparison_manager->get_left_algorithm();
        const auto& right = m_comparison_manager->get_right_algorithm();

        if (left.is_valid())
        {
            ImGui::ProgressBar(left.metrics.progress,
                              ImVec2(-1, PROGRESS_BAR_HEIGHT),
                              left.name.c_str());
        }

        if (right.is_valid())
        {
            ImGui::ProgressBar(right.metrics.progress,
                              ImVec2(-1, PROGRESS_BAR_HEIGHT),
                              right.name.c_str());
        }

        ImGui::End();
    }

    void AlgorithmComparisonScene::render_data_control_panel()
    {
        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        ImGui::Begin("Data Controls", &m_show_data_control);

        // Data size slider
        ImGui::SliderInt("Data Size", &m_data_size, 5, BUTTON_WIDTH);
        ImGui::SliderInt("Max Value", &m_max_value, 10, 50);

        if (ImGui::Button("Generate Random Data", ImVec2(-1, 0)))
        {
            m_comparison_manager->generate_random_data(
                static_cast<size_t>(m_data_size), 
                m_max_value
            );
        }

        // Preset data sets
        ImGui::Spacing();
        ImGui::Text("Presets:");
        ImGui::Separator();

        if (ImGui::Button("Sorted", ImVec2(BUTTON_WIDTH, 0)))
        {
            std::vector<int> data;
            for (int i = 1; i <= m_data_size; ++i)
                data.push_back(i);
            m_comparison_manager->set_shared_data(data);
        }
        ImGui::SameLine();

        if (ImGui::Button("Reverse", ImVec2(BUTTON_WIDTH, 0)))
        {
            std::vector<int> data;
            for (int i = m_data_size; i >= 1; --i)
                data.push_back(i);
            m_comparison_manager->set_shared_data(data);
        }
        ImGui::SameLine();

        if (ImGui::Button("Constant", ImVec2(BUTTON_WIDTH, 0)))
        {
            std::vector<int> data(
                static_cast<size_t>(m_data_size), 
                42
            );
            m_comparison_manager->set_shared_data(data);
        }

        ImGui::End();
    }

    void AlgorithmComparisonScene::render_results_panel()
    {
        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        ImGui::Begin(
            "Parallel Comparison Results",
            &m_show_results_panel
        );

        const auto& left = m_comparison_manager->get_left_algorithm();
        const auto& right = m_comparison_manager->get_right_algorithm();
        auto result = m_comparison_manager->get_parallel_comparison_result();

        // Parallel execution stats
        ImGui::TextColored(
            ImVec4(0,1,1,1), 
            "Parallel Execution Efficiency: %.2f%%", 
            result.parallel_efficiency * 100.0
        );
        ImGui::Separator();

        // Create a table for side-by-side comparison
        if (ImGui::BeginTable("ResultsTable", 3, 
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            // Header
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn(left.name.c_str());
            ImGui::TableSetupColumn(right.name.c_str());
            ImGui::TableHeadersRow();

            // Steps
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Total Steps");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", left.metrics.total_steps.load());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%zu", right.metrics.total_steps.load());

            // Comparisons
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Comparisons");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", left.metrics.total_comparisons.load());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%zu", right.metrics.total_comparisons.load());

            // Swaps
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Swaps");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", left.metrics.total_swaps.load());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%zu", right.metrics.total_swaps.load());

            // Average step time
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Avg Step Time (ms)");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", result.left.avg_step_time_ms);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.3f", result.right.avg_step_time_ms);

            // Memory
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Peak Memory (bytes)");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", left.metrics.peak_memory.load());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%zu", right.metrics.peak_memory.load());

            ImGui::EndTable();
        }

        // Winner announcement
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (result.has_winner())
        {
            const char* winner_name = result.winner == "left" ? 
                left.name.c_str() : right.name.c_str();
            
            ImGui::TextColored(ImVec4(0,1,0,1), "Winner: %s", winner_name);
            ImGui::Text("Performance Ratio: %.2fx", result.performance_ratio);
            ImGui::Text("Parallel Speedup: %.2fx", result.parallel_efficiency);
        }
        else if (m_comparison_manager->is_complete())
        {
            ImGui::TextColored(ImVec4(1,1,0,1), "Tie!");
        }

        ImGui::End();
    }

    void AlgorithmComparisonScene::render_performance_graphs()
    {
        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        ImGui::Begin(
            "Performance Graphs", 
            &m_show_performance_graphs
        );

        // Simple line graph placeholder
        // In a real implementation, you'd use ImPlot or custom rendering
        ImGui::Text("Step-by-step performance comparison");
        ImGui::PlotLines(
            "##Left",
            [](void* data, int idx)
            {
                return static_cast<float>(idx) * 0.1f;
            },
            nullptr,
            100,
            0,
            nullptr,
            0.0f,
            10.0f,
            ImVec2(0, 100)
        );

        ImGui::SameLine();

        ImGui::PlotLines(
            "##Right",
            [](void* data, int idx)
            {
                return static_cast<float>(idx) * 0.15f;
            },
            nullptr,
            100,
            0,
            nullptr,
            0.0f,
            10.0f,
            ImVec2(0, 100)
        );

        ImGui::End();
    }

    void AlgorithmComparisonScene::update_algorithm_cache()
    {
        m_algorithm_cache.clear();

        // Get all available algorithms
        auto types = m_algorithm_registry.get_available_algorithm_types();

        for (auto type : types)
        {
            AlgorithmInfo info;
            info.name = algorithms::algorithm_display_name(type);
            info.type = type;
            info.category = algorithms::algorithm_display_category(type);

            m_algorithm_cache.push_back(info);
        }

        LOG_DEBUG(
            "Updated algorithm cache with {} algorithms", 
            m_algorithm_cache.size()
        );
    }

    void AlgorithmComparisonScene::sync_visualizers()
    {
        const auto& left = m_comparison_manager->get_left_algorithm();
        const auto& right = m_comparison_manager->get_right_algorithm();

        m_visualizer.initialize(
            left.algorithm.get(), left.metadata,
            right.algorithm.get(), right.metadata
        );
    }

    void AlgorithmComparisonScene::setup_main_menu()
    {
        std::vector<ui::components::MainMenu::MenuItem> view_items = {
            {"Selection Panel", [this]() {
                m_show_selection_panel = !m_show_selection_panel;
            }, nullptr, "F1"},
            {"Comparison View", [this]() {
                m_show_comparison_view = !m_show_comparison_view;
            }, nullptr, "F2"},
            {"Controls", [this]() {
                m_show_control_panel = !m_show_control_panel;
            }, nullptr, "F3"},
            {"Data Controls", [this]() {
                m_show_data_control = !m_show_data_control;
            }, nullptr, "F4"},
            {"Results", [this]() {
                m_show_results_panel = !m_show_results_panel;
            }, nullptr, "F5"},
            {"Performance Graphs", [this]() {
                m_show_performance_graphs = !m_show_performance_graphs;
            }, nullptr, "F6"},
        };

        m_main_menu->add_menu("View", std::move(view_items));
    }

} // namespace c2l::scenes