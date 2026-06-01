#include "algorithms/visualizers/graph_based_visualizer.hpp"
#include "core/utils/logger/logger.hpp"
#include <cmath>
#include <algorithm>
#include <queue>
#include <random>

#include "algorithms/bfs.hpp"
#include "algorithms/dfs.hpp"
#include "algorithms/dijkstra.hpp"
#include "algorithms/a_star.hpp"
#include "algorithms/bellman_ford.hpp"
#include "algorithms/topological_sort.hpp"



namespace c2l::algorithms
{
    GraphBasedVisualizer::GraphBasedVisualizer(
        ui::managers::IconManager& icon_manager,
        const VisualizationConfig& config)
        : m_icon_manager{icon_manager}
        , m_config{config}
    {
        // Create demo graph
        for (int i = 0; i < 6; ++i)
            m_graph.add_node("Node " + std::to_string(i), (i + 1) * 10);
        m_graph.add_edge(0, 1); 
        m_graph.add_edge(0, 2); 
        m_graph.add_edge(0, 3);
        m_graph.add_edge(1, 4); 
        m_graph.add_edge(2, 4); 
        m_graph.add_edge(3, 5);
        m_graph.add_edge(4, 5);

        ImVec2 bounds(800, 600);
        m_layout_engine.compute_layout(m_graph, m_current_layout, bounds);
    }

    void GraphBasedVisualizer::initialize(
        ISimpleAlgorithm* execution, 
        const IAlgorithmMetadata* metadata,
        const bool show_sidebar_controller)
    {
        m_execution = execution;
        m_metadata = metadata;
        m_animation_time = 0.0f;

        m_show_sidebar_controller = show_sidebar_controller;

        // Setting callback
        m_on_graph_changed = [this]()
        {
            sync_algorithm_with_graph();
        };

        sync_algorithm_with_graph();

        LOG_DEBUG(
            "GraphBasedVisualizer initialized for {}", 
            m_metadata ? m_metadata->get_display_name() : "Unknown"
        );
    }

    void GraphBasedVisualizer::update(double delta_time)
    {
        m_animation_time += static_cast<float>(delta_time);
        m_layout_update_timer += static_cast<float>(delta_time);

        if (m_current_layout == GraphLayoutEngine::LayoutType::FORCE_DIRECTED && 
            m_layout_update_timer >= 0.033f)
        {
            GraphLayoutEngine::Params params;
            m_layout_engine.update_force_directed(
                m_graph, m_layout_update_timer, 
                params
            );
            m_layout_update_timer = 0;
        }

        if (m_execution)
        {
            const auto step = m_execution->get_current_step();
            // Update node states from algorithm
            for (auto& [id, node] : m_graph.nodes)
            {
                node.visited = false;
                node.in_queue = false;
                node.distance = -1;
            }
            for (size_t v : step.visualization.graph_state.visited_nodes)
                if (m_graph.nodes.find(v) != m_graph.nodes.end()) 
                    m_graph.nodes[v].visited = true;
            for (size_t f : step.visualization.graph_state.frontier_nodes)
                if (m_graph.nodes.find(f) != m_graph.nodes.end()) 
                    m_graph.nodes[f].in_queue = true;
            for (const auto& [id, dist] : step.visualization.graph_state.node_distances)
                if (m_graph.nodes.find(id) != m_graph.nodes.end()) 
                    m_graph.nodes[id].distance = dist;
        }
    }

    void GraphBasedVisualizer::render()
    {
        ImGui::BeginChild("GraphMain", ImVec2(0, 0), false);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float sidebar_width = std::clamp(
            avail.x * 0.28f, 
            280.0f, 
            400.0f
        );

        if (!m_show_sidebar_controller)
            sidebar_width = 0.0f;

        // Graph View
        ImGui::BeginChild(
            "GraphView", 
            ImVec2(avail.x - sidebar_width, avail.y), 
            true, 
            ImGuiWindowFlags_NoScrollbar
        );

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 viewport_pos = ImGui::GetCursorScreenPos();
        ImVec2 viewport_size = ImGui::GetContentRegionAvail();

        // Background
        draw_list->AddRectFilled(
            viewport_pos, 
            ImVec2(
                viewport_pos.x + viewport_size.x, 
                viewport_pos.y + viewport_size.y),
            ImColor(15, 20, 30, 255)
        );

        // Draw grid
        float grid_spacing = 50.0f * m_zoom;
        float off_x = fmodf(m_view_offset.x, grid_spacing);
        float off_y = fmodf(m_view_offset.y, grid_spacing);
        for (float x = viewport_pos.x + off_x; 
             x < viewport_pos.x + viewport_size.x; 
             x += grid_spacing)
        {
            draw_list->AddLine(
                ImVec2(x, viewport_pos.y), 
                ImVec2(x, viewport_pos.y + viewport_size.y), 
                ImColor(255,255,255,20)
            );
        }
        for (float y = viewport_pos.y + off_y; 
             y < viewport_pos.y + viewport_size.y; 
             y += grid_spacing)
        {
            draw_list->AddLine(
                ImVec2(viewport_pos.x, y), 
                ImVec2(viewport_pos.x + viewport_size.x, y), 
                ImColor(255,255,255,20)
            );
        }

        // Drawing graph
        draw_graph();

        handle_mouse_interaction();
        handle_keyboard_shortcuts();

        ImGui::EndChild();

        if (m_show_sidebar_controller)
        {
            // Sidebar
            ImGui::SameLine();
            ImGui::BeginChild(
                "Sidebar",
                ImVec2(sidebar_width, avail.y),
                true
            );

            render_controls_toolbar();
            ImGui::Separator();
            ImGui::Dummy({0.0f, 15.0f});


            if (ImGui::CollapsingHeader("Graph Editor", ImGuiTreeNodeFlags_DefaultOpen))
                render_graph_editor_panel();

            if (ImGui::CollapsingHeader("Node Editor", ImGuiTreeNodeFlags_DefaultOpen))
                render_node_edit_panel();

            if (ImGui::CollapsingHeader("Edge Editor"))
                render_edge_edit_panel();

            if (ImGui::CollapsingHeader("Properties"))
                render_properties_panel();

            if (ImGui::CollapsingHeader("Algorithm Config"))
                render_algorithm_config_panel();

            ImGui::EndChild();
        }

        ImGui::EndChild();
    }

    void GraphBasedVisualizer::render_controls_toolbar()
    {
        ImGui::TextColored(ImColor(100, 200, 255), "Graph Controls");

        ImGui::Dummy({0.0f, 10.0f});

        // Layout selection
        ImGui::SetNextItemWidth(-1);
        ImGui::TextUnformatted("Layout");
        int layout_idx = static_cast<int>(m_current_layout);
        if (ImGui::Combo(
            "##Layout", 
            &layout_idx, 
            LAYOUT_NAMES, 
            IM_ARRAYSIZE(LAYOUT_NAMES))
        )
        {
            m_current_layout = static_cast<GraphLayoutEngine::LayoutType>(layout_idx);
            ImVec2 bounds(800, 600);
            m_layout_engine.compute_layout(m_graph, m_current_layout, bounds);

            sync_algorithm_with_graph();
        }

        // Display options
        ImGui::Checkbox("Show Edge Weights", &m_show_edge_weights);
        ImGui::Checkbox("Show Node Labels", &m_show_node_labels);
        ImGui::Checkbox("Show Node Ids", &m_show_node_ids);
        ImGui::Checkbox("Show Distances", &m_show_distances);

        // View controls
        ImGui::Text("Zoom: %.1f%%", m_zoom * 100.0f);
        if (ImGui::Button("Reset View"))
        {
            m_zoom = 1.0f;
            m_view_offset = ImVec2(0, 0);
        }
    }

    void GraphBasedVisualizer::render_graph_editor_panel()
    {
        // Add Node
        ImGui::Text("Add Node");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint(
            "##NodeLabel",
            "Label",
            m_new_node_label,
            sizeof(m_new_node_label)
        );

        static char node_val[128] = "";

        ImGui::SetNextItemWidth(-1);

        ImGui::InputTextWithHint(
            "##NodeValue",
            "Value (e.g., 4)",
            node_val,
            sizeof(node_val),
            ImGuiInputTextFlags_CharsDecimal
        );

        char* endptr = nullptr;

        long value = strtol(node_val, &endptr, 10);

        const bool valid =
            endptr != node_val &&
            *endptr == '\0';

        if (valid)
        {
            m_new_node_value = static_cast<int>(value);
        }

        if (ImGui::Button("Add Node", ImVec2(-1, 0)))
        {
            std::string label = m_new_node_label;
            if (label.empty()) label = "Node " + std::to_string(m_graph.next_node_id);
            m_graph.add_node(label, m_new_node_value);

            sync_algorithm_with_graph();

            m_new_node_label[0] = '\0';
            node_val[0] = '\0';
            m_new_node_value = 0;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Dummy({0.0f, 5.0f});

        // Add Edge
        ImGui::Text("Add Edge");

        // From
        static char node_from[128] = "";

        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint(
            "##NodeFrom",
            "From node ID (e.g., 0)",
            node_from,
            sizeof(node_from),
            ImGuiInputTextFlags_CharsDecimal
        );

        char* endptr_node_from = nullptr;
        long value_endptr_node_from = strtol(node_from, &endptr_node_from, 10);

        const bool valid_endptr_node_from =
            endptr_node_from != node_from &&
            *endptr_node_from == '\0';
        if (valid_endptr_node_from)
        {
            m_new_edge_from = static_cast<int>(value_endptr_node_from);
        }

        // To
        static char node_to[128] = "";

        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint(
            "##NodeTo",
            "To node ID (e.g., 1)",
            node_to,
            sizeof(node_to),
            ImGuiInputTextFlags_CharsDecimal
        );

        char* endptr_node_to = nullptr;
        long value_endptr_node_to = strtol(node_to, &endptr_node_to, 10);

        const bool valid_endptr_node_to =
            endptr_node_to != node_to &&
            *endptr_node_to == '\0';
        if (valid_endptr_node_to)
        {
            m_new_edge_to = static_cast<int>(value_endptr_node_to);
        }

        // weight
        static char node_weight[128] = "";

        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint(
            "##NodeWeight",
            "Weight (e.g., 5)",
            node_weight,
            sizeof(node_weight),
            ImGuiInputTextFlags_CharsDecimal
        );

        char* endptr_node_weight = nullptr;
        long value_endptr_node_weight = strtol(node_weight, &endptr_node_weight, 10);

        const bool valid_endptr_node_weight =
            endptr_node_weight != node_weight &&
            *endptr_node_weight == '\0';
        if (valid_endptr_node_weight)
        {
            m_new_edge_weight = static_cast<int>(value_endptr_node_weight);
        }

        if (ImGui::Button("Add Edge", ImVec2(-1, 0)))
        {
            if (m_graph.add_edge(m_new_edge_from, m_new_edge_to, m_new_edge_weight))
            {
                ImVec2 bounds(800, 600);
                m_layout_engine.compute_layout(m_graph, m_current_layout, bounds);
                sync_algorithm_with_graph();

                m_new_edge_from = 0;
                node_from[0] = '\0';
                m_new_edge_to = 0;
                node_to[0] = '\0';
                m_new_edge_weight = 0;
                node_weight[0] = '\0';
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Quick connect mode
        ImGui::Text("Quick Connect Mode");
        if (m_node_to_connect == static_cast<size_t>(-1))
        {
            if (ImGui::Button("Enter Connect Mode"))
                m_node_to_connect = 0;
        }
        else
        {
            ImGui::Text("Select target node to connect from Node %zu", m_node_to_connect);
            ImGui::NewLine();
            if (ImGui::Button("Cancel"))
            {
                m_node_to_connect = static_cast<size_t>(-1);
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Dummy({0.0f, 5.0f});


        // Graph presets
        ImGui::Text("Presets");
        if (ImGui::Button("Path (5 nodes)", ImVec2(-1, 0)))
        {
            m_graph.clear();
            for (size_t i = 0; i < 5; ++i) 
                m_graph.add_node("N" + std::to_string(i), i);
            for (size_t i = 0; i < 4; ++i) 
                m_graph.add_edge(i, i + 1);
            const ImVec2 bounds(800, 600);
            m_layout_engine.compute_layout(
                m_graph, 
                m_current_layout, 
                bounds
            );
            sync_algorithm_with_graph();
        }
        if (ImGui::Button("Cycle (6 nodes)", ImVec2(-1, 0)))
        {
            m_graph.clear();
            for (int i = 0; i < 6; ++i) 
                m_graph.add_node("N" + std::to_string(i), i);
            for (int i = 0; i < 6; ++i) 
                m_graph.add_edge(i, (i + 1) % 6);
            ImVec2 bounds(800, 600);
            m_layout_engine.compute_layout(
                m_graph, 
                m_current_layout, 
                bounds
            );
            sync_algorithm_with_graph();
        }
        if (ImGui::Button("Complete (5 nodes)", ImVec2(-1, 0)))
        {
            m_graph.clear();
            for (int i = 0; i < 5; ++i) 
                m_graph.add_node("N" + std::to_string(i), i);
            for (int i = 0; i < 5; ++i)
                for (int j = i + 1; j < 5; ++j)
                    m_graph.add_edge(i, j);
            ImVec2 bounds(800, 600);
            m_layout_engine.compute_layout(
                m_graph, 
                m_current_layout, 
                bounds
            );
            sync_algorithm_with_graph();
        }
        if (ImGui::Button("Star (6 nodes)", ImVec2(-1, 0)))
        {
            m_graph.clear();
            for (int i = 0; i < 6; ++i) 
                m_graph.add_node("N" + std::to_string(i), i);
            for (int i = 1; i < 6; ++i) 
                m_graph.add_edge(0, i);
            ImVec2 bounds(800, 600);
            m_layout_engine.compute_layout(
                m_graph, 
                m_current_layout, 
                bounds
            );
            sync_algorithm_with_graph();
        }
        if (ImGui::Button("5-ary Tree (30 nodes)", ImVec2(-1, 0)))
        {
            m_graph.clear();

            constexpr int NODE_COUNT = 30;
            constexpr int CHILDREN_PER_NODE = 2;

            // Create nodes
            for (int i = 0; i < NODE_COUNT; ++i)
            {
                m_graph.add_node("N" + std::to_string(i), i);
            }
            
            // Connect each node to 5 children
            for (size_t i = 0; i < NODE_COUNT; ++i)
            {
                for (size_t k = 1; k <= CHILDREN_PER_NODE; ++k)
                {
                    size_t child = CHILDREN_PER_NODE * i + k;

                    if (child < NODE_COUNT)
                    {
                        m_graph.add_edge(i, child);
                    }
                }
            }

            ImVec2 bounds(800, 600);
            m_layout_engine.compute_layout(
                m_graph, 
                m_current_layout, 
                bounds
            );

            sync_algorithm_with_graph();
        }
        if (ImGui::Button("Binary Tree (15 nodes)", ImVec2(-1, 0)))
        {
            m_graph.clear();

            constexpr int NODE_COUNT = 15;
            constexpr int CHILDREN_PER_NODE = 2;

            // Create nodes
            for (int i = 0; i < NODE_COUNT; ++i)
            {
                m_graph.add_node("N" + std::to_string(i), i);
            }

            // Connect each node to children with random weights
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> weight_dist(1, 10);

            for (size_t i = 0; i < NODE_COUNT; ++i)
            {
                for (size_t k = 1; k <= CHILDREN_PER_NODE; ++k)
                {
                    size_t child = CHILDREN_PER_NODE * i + k;

                    if (child < NODE_COUNT)
                    {
                        int weight = weight_dist(gen);
                        m_graph.add_edge(i, child, weight);
                    }
                }
            }

            ImVec2 bounds(800, 600);
            m_layout_engine.compute_layout(
                m_graph, 
                m_current_layout, 
                bounds
            );
            sync_algorithm_with_graph();
        }

        if (ImGui::Button("Weighted Binary Tree (31 nodes)", ImVec2(-1, 0)))
        {
            m_graph.clear();

            constexpr int NODE_COUNT = 31;
            constexpr int CHILDREN_PER_NODE = 2;

            // Create nodes
            for (int i = 0; i < NODE_COUNT; ++i)
            {
                m_graph.add_node("N" + std::to_string(i), i);
            }

            // Connect each node to children with increasing weights (deeper = higher weight)
            for (size_t i = 0; i < NODE_COUNT; ++i)
            {
                for (size_t k = 1; k <= CHILDREN_PER_NODE; ++k)
                {
                    size_t child = CHILDREN_PER_NODE * i + k;

                    if (child < NODE_COUNT)
                    {
                        // Weight increases with depth: base weight 1 + depth/2
                        int depth = 0;
                        size_t temp = child;
                        while (temp > 0) {
                            temp = (temp - 1) / CHILDREN_PER_NODE;
                            depth++;
                        }
                        int weight = std::max(1, depth);
                        m_graph.add_edge(i, child, weight);
                    }
                }
            }

            ImVec2 bounds(800, 600);
            m_layout_engine.compute_layout(
                m_graph, 
                m_current_layout, 
                bounds
            );
            sync_algorithm_with_graph();
        }

        if (ImGui::Button("Weighted Grid (5x5)", ImVec2(-1, 0)))
        {
            m_graph.clear();

            constexpr int GRID_SIZE = 5;
            constexpr int NODE_COUNT = GRID_SIZE * GRID_SIZE;

            // Create nodes
            for (int i = 0; i < NODE_COUNT; ++i)
            {
                int row = i / GRID_SIZE;
                int col = i % GRID_SIZE;
                m_graph.add_node("(" + std::to_string(row) + "," + std::to_string(col) + ")", i);
            }

            // Connecting grid with weighted edges
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> weight_dist(1, 5);

            for (int row = 0; row < GRID_SIZE; ++row)
            {
                for (int col = 0; col < GRID_SIZE; ++col)
                {
                    size_t id = row * GRID_SIZE + col;

                    // Connect right neighbor
                    if (col + 1 < GRID_SIZE)
                    {
                        size_t right = row * GRID_SIZE + (col + 1);
                        int weight = weight_dist(gen);
                        m_graph.add_edge(id, right, weight);
                    }

                    // Connect down neighbor
                    if (row + 1 < GRID_SIZE)
                    {
                        size_t down = (row + 1) * GRID_SIZE + col;
                        int weight = weight_dist(gen);
                        m_graph.add_edge(id, down, weight);
                    }
                }
            }

            ImVec2 bounds(800, 600);
            m_layout_engine.compute_layout(
                m_graph, 
                m_current_layout, 
                bounds
            );
            sync_algorithm_with_graph();
        }

        if (ImGui::Button("Weighted Complete Graph (6 nodes)", ImVec2(-1, 0)))
        {
            m_graph.clear();

            constexpr int NODE_COUNT = 6;

            // Create nodes
            for (int i = 0; i < NODE_COUNT; ++i)
            {
                m_graph.add_node("N" + std::to_string(i), i);
            }

            // Connect all nodes with random weights
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> weight_dist(1, 15);

            for (int i = 0; i < NODE_COUNT; ++i)
            {
                for (int j = i + 1; j < NODE_COUNT; ++j)
                {
                    int weight = weight_dist(gen);
                    m_graph.add_edge(i, j, weight);
                }
            }

            ImVec2 bounds(800, 600);
            m_layout_engine.compute_layout(
                m_graph, 
                m_current_layout, 
                bounds
            );
            sync_algorithm_with_graph();
        }

        if (ImGui::Button("Weighted Cycle (8 nodes)", ImVec2(-1, 0)))
        {
            m_graph.clear();

            constexpr int NODE_COUNT = 8;

            // Create nodes
            for (int i = 0; i < NODE_COUNT; ++i)
            {
                m_graph.add_node("N" + std::to_string(i), i);
            }

            // Create cycle with alternating weights
            for (int i = 0; i < NODE_COUNT; ++i)
            {
                int next = (i + 1) % NODE_COUNT;
                int weight = (i % 2 == 0) ? 2 : 5;  // Alternating weights: 2, 5, 2, 5...
                m_graph.add_edge(i, next, weight);
            }

            ImVec2 bounds(800, 600);
            m_layout_engine.compute_layout(
                m_graph, 
                m_current_layout, 
                bounds
            );
            sync_algorithm_with_graph();
        }

        if (ImGui::Button("Custom Weighted Binary Tree", ImVec2(-1, 0)))
        {
            m_graph.clear();

            constexpr int NODE_COUNT = 15;
            constexpr int CHILDREN_PER_NODE = 2;

            // Create nodes
            for (int i = 0; i < NODE_COUNT; ++i)
            {
                m_graph.add_node("N" + std::to_string(i), i);
            }

            // Manual weight configuration for specific use cases
            // Example: Creating a graph where left path is cheaper than right path
            for (size_t i = 0; i < NODE_COUNT; ++i)
            {
                size_t left_child = CHILDREN_PER_NODE * i + 1;
                size_t right_child = CHILDREN_PER_NODE * i + 2;

                if (left_child < NODE_COUNT)
                {
                    // Left edge is cheaper
                    int weight = (i % 2 == 0) ? 1 : 3;
                    m_graph.add_edge(i, left_child, weight);
                }

                if (right_child < NODE_COUNT)
                {
                    // Right edge is more expensive
                    int weight = (i % 2 == 0) ? 10 : 15;
                    m_graph.add_edge(i, right_child, weight);
                }
            }

            ImVec2 bounds(800, 600);
            m_layout_engine.compute_layout(
                m_graph, 
                m_current_layout, 
                bounds
            );
            sync_algorithm_with_graph();
        }

        if (ImGui::Button("Negative Weight Graph (test)", ImVec2(-1, 0)))
        {
            m_graph.clear();

            // Create a small graph with negative weights for Bellman-Ford testing
            for (int i = 0; i < 5; ++i)
            {
                m_graph.add_node("N" + std::to_string(i), i);
            }

            // Create edges with some negative weights
            m_graph.add_edge(0, 1, 4);   // Positive
            m_graph.add_edge(0, 2, 2);   // Positive
            m_graph.add_edge(1, 3, -3);  // Negative edge
            m_graph.add_edge(2, 3, 1);   // Positive
            m_graph.add_edge(3, 4, 2);   // Positive
            m_graph.add_edge(1, 4, 5);   // Positive

            ImVec2 bounds(800, 600);
            m_layout_engine.compute_layout(
                m_graph, 
                m_current_layout, 
                bounds
            );
            sync_algorithm_with_graph();
        }
        if (ImGui::Button("Clear All", ImVec2(-1, 0)))
        {
            m_graph.clear();
            sync_algorithm_with_graph();
        }
    }

    void GraphBasedVisualizer::render_node_edit_panel()
    {
        ImGui::Text("Nodes (%zu)", m_graph.node_count());

        ImGui::BeginChild("NodeList", ImVec2(-1, 200), true);

        size_t node_to_delete = static_cast<size_t>(-1); // Track if we need to delete

        for (auto& [id, node] : m_graph.nodes)
        {
            ImGui::PushID(static_cast<int>(id));

            bool selected = (m_selected_node == id);

            // Reserve space for buttons on the right
            float button_width = 60.0f;
            float avail_width = ImGui::GetContentRegionAvail().x;

            // Render selectable with reduced width
            ImGui::SetNextItemWidth(avail_width - button_width);

            ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

            // Create selectable area manually
            if (ImGui::Selectable(
                    (node.label + " (ID:" + std::to_string(id) + ")").c_str(),
                    selected,
                    0,
                    ImVec2(avail_width - button_width, 0)))
            {
                m_selected_node = id;
            }

            // Move cursor to right side of the row
            ImGui::SameLine(avail_width - button_width + 20);

            // Linking button
            m_icon_manager.render_icon_button(
                "link_button_" + std::to_string(id),
                ui::managers::IconType::LINK,
                [&, id]()
                {
                    if (m_node_to_connect == static_cast<size_t>(-1))
                    {
                        m_node_to_connect = id;
                    }
                    else
                    {
                        m_graph.add_edge(m_node_to_connect, id, 1);
                        m_node_to_connect = static_cast<size_t>(-1);

                        ImVec2 bounds(800.0f, 600.0f);
                        m_layout_engine.compute_layout(
                            m_graph,
                            m_current_layout,
                            bounds);
                    }
                },
                ImVec2(11, 11),
                true,
                "Link"
            );

            ImGui::SameLine();

            // Deleting button
            m_icon_manager.render_icon_button(
                "close_button_" + std::to_string(id),
                ui::managers::IconType::CLOSE,
                [&node_to_delete, id]()
                {
                    node_to_delete = id;
                },
                ImVec2(11, 11),
                true,
                "Remove"
            );

            // Selected node editor
            if (selected)
            {
                ImGui::Indent();

                char buf[128];
                snprintf(buf, sizeof(buf), "%s", node.label.c_str());

                if (ImGui::InputText("Label", buf, sizeof(buf)))
                {
                    node.label = buf;
                }

                ImGui::InputInt("Value", &node.value);
                ImGui::Checkbox("Fixed Position", &node.is_fixed);

                float pos[2] = {node.position.x, node.position.y};

                if (ImGui::DragFloat2("Position", pos, 0.01f, 0.0f, 1.0f))
                {
                    node.position = ImVec2(pos[0], pos[1]);
                }

                ImGui::Unindent();
            }

            ImGui::PopID();

            if (node_to_delete != static_cast<size_t>(-1))
                break;
        }

        // Safety: remove node outside the iteration loop to prevent iterator invalidation
        if (node_to_delete != static_cast<size_t>(-1))
        {
            m_graph.remove_node(node_to_delete);
            if (m_selected_node == node_to_delete) 
                m_selected_node = static_cast<size_t>(-1);
        }

        ImGui::EndChild();
    }


    void GraphBasedVisualizer::render_edge_edit_panel()
    {
        ImGui::Text(
            "Edges (%zu)",
            m_graph.edge_count()
        );

        ImGui::BeginChild(
            "EdgeList",
            ImVec2(-1, 150),
            true
        );

        std::optional<size_t> edge_to_delete;

        for (size_t i = 0; i < m_graph.edges.size(); ++i)
        {
            auto& edge = m_graph.edges[i];

            ImGui::PushID(static_cast<int>(i));

            std::string text =
                std::to_string(edge.from) +
                "  -->  " +
                std::to_string(edge.to);

            if (m_graph.is_weighted)
            {
                text += " [w=" + std::to_string(edge.weight) + "]";
            }

            float button_width = 25.0f;
            float avail_width = ImGui::GetContentRegionAvail().x;

            // Edge label
            ImGui::Selectable(
                text.c_str(),
                false,
                0, ImVec2(avail_width - button_width, 0)
            );

            ImGui::SameLine(avail_width - button_width + 15);

            // Delete button
            m_icon_manager.render_icon_button(
                "close_button_" + std::to_string(i),
                ui::managers::IconType::CLOSE,
                [&edge_to_delete, i]()
                {
                    edge_to_delete = i;
                },
                ImVec2(11, 11),
                true,
                "Remove"
            );

            if (m_graph.is_weighted)
            {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(60);
                if (ImGui::InputInt("##w", &edge.weight, 1, 5))
                {
                    if (edge.weight < 1)
                    {
                        edge.weight = 1;
                    }
                }
            }

            ImGui::PopID();

            // stopping iteration after marking deletion
            if (edge_to_delete.has_value())
            {
                break;
            }
        }

        // Removing after iteration
        if (edge_to_delete.has_value())
        {
            m_graph.edges.erase(
                m_graph.edges.begin() + static_cast<long>(edge_to_delete.value())
            );
        }
        ImGui::EndChild();
    }

    void GraphBasedVisualizer::render_properties_panel()
    {
        if (ImGui::Checkbox("Directed Graph", &m_graph.is_directed))
        {
            sync_algorithm_with_graph();
        }
        if (ImGui::Checkbox("Weighted Graph", &m_graph.is_weighted))
        {
            sync_algorithm_with_graph();
        }

        ImGui::Spacing();
        ImGui::Text("Statistics:");
        ImGui::Text("  Nodes: %zu", m_graph.node_count());
        ImGui::Text("  Edges: %zu", m_graph.edge_count());
        if (m_graph.node_count() > 1)
        {
            float density = 2.0f * m_graph.edge_count() / (m_graph.node_count() * (m_graph.node_count() - 1));
            ImGui::Text("  Density: %.3f", density);
        }
    }

    void GraphBasedVisualizer::render_algorithm_config_panel()
    {
        ImGui::Text("Algorithm Configuration");

        // Start Node
        ImGui::Text("Start Node:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        int start_node = static_cast<int>(m_graph.start_node);
        if (ImGui::InputInt("##StartNode", &start_node))
        {
            if (start_node >= 0 && m_graph.has_node(start_node))
                m_graph.start_node = start_node;
            sync_algorithm_with_graph();
        }

        // Target Node
        ImGui::Text("Target Node:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        int target_node = m_graph.target_node.has_value() ? static_cast<int>(m_graph.target_node.value()) : -1;
        if (ImGui::InputInt("##TargetNode", &target_node))
        {
            if (target_node >= 0 && m_graph.has_node(target_node))
                m_graph.target_node = target_node;
            else
                m_graph.target_node.reset();
            sync_algorithm_with_graph();
      }

        // Sync button
        if (ImGui::Button("Sync with Algorithm", ImVec2(-1, 0)))
        {
            sync_algorithm_with_graph();
        }

        if (m_execution && m_metadata)
        {
            ImGui::Spacing();
            ImGui::Text("Algorithm: %s", m_metadata->get_display_name().c_str());
            ImGui::Text(
                "Step: %zu / %zu", 
                m_execution->get_current_step_index() + 1, 
                m_execution->get_step_count()
            );
            if (m_graph.target_node.has_value())
            {
                bool found = false;
                // Check if target found (would need to check algorithm state)
                ImGui::TextColored(
                    found ? ImColor(100, 255, 100) : ImColor(255, 200, 100),
                    "Target: %s", found ? "FOUND ✓" : "Searching..."
                );
            }
        }
    }

    void GraphBasedVisualizer::draw_graph()
{
    if (m_graph.node_count() == 0) return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 viewport_pos = ImGui::GetCursorScreenPos();
    ImVec2 viewport_size = ImGui::GetContentRegionAvail();

    auto world_to_screen = [&](const ImVec2& world) -> ImVec2 
    {
        return ImVec2(
            viewport_pos.x + (world.x * viewport_size.x + m_view_offset.x) * m_zoom,
            viewport_pos.y + (world.y * viewport_size.y + m_view_offset.y) * m_zoom
        );
    };

    // Drawing edges first
    for (const auto& edge : m_graph.edges)
    {
        auto from_it = m_graph.nodes.find(edge.from);
        auto to_it = m_graph.nodes.find(edge.to);
        if (from_it == m_graph.nodes.end() || to_it == m_graph.nodes.end()) 
            continue;

        ImVec2 from_screen = world_to_screen(from_it->second.position);
        ImVec2 to_screen = world_to_screen(to_it->second.position);

        // Checking if this edge is active (being explored in current step)
        bool is_active_edge = false;
        if (m_execution)
        {
            const auto step = m_execution->get_current_step();
            for (const auto& active_edge : step.visualization.graph_state.active_edges)
            {
                if ((active_edge.first == edge.from && active_edge.second == edge.to) ||
                    (active_edge.first == edge.to && active_edge.second == edge.from))
                {
                    is_active_edge = true;
                    break;
                }
            }
        }

        ImU32 edge_color;
        if (is_active_edge)
            edge_color = ImColor(255, 165, 0, 255);  // Orange for active edge
        else if (from_it->second.visited && to_it->second.visited)
            edge_color = ImColor(100, 200, 100, 200);  // Green for explored edges
        else
            edge_color = ImColor(80, 80, 120, 180);    // Dim for unexplored

        draw_list->AddLine(from_screen, to_screen, edge_color, is_active_edge ? 3.0f : 2.0f);

        // Draw arrow for directed graphs
        if (m_graph.is_directed)
        {
            ImVec2 dir(to_screen.x - from_screen.x, to_screen.y - from_screen.y);
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 0.01f)
            {
                ImVec2 unit(dir.x / len, dir.y / len);
                ImVec2 perp(-unit.y, unit.x);
                float arrow_size = 10.0f;
                ImVec2 arrow_base(to_screen.x - unit.x * arrow_size, to_screen.y - unit.y * arrow_size);
                draw_list->AddTriangleFilled(to_screen,
                    ImVec2(arrow_base.x - perp.x * 5, arrow_base.y - perp.y * 5),
                    ImVec2(arrow_base.x + perp.x * 5, arrow_base.y + perp.y * 5), edge_color);
            }
        }

        // Draw edge weight
        if (m_show_edge_weights && edge.weight != 1)
        {
            ImVec2 mid(
                (from_screen.x + to_screen.x) * 0.5f, 
                (from_screen.y + to_screen.y) * 0.5f
            );
            std::string w = std::to_string(edge.weight);
            ImVec2 ts = ImGui::CalcTextSize(w.c_str());
            draw_list->AddRectFilled(
                ImVec2(
                    mid.x - ts.x * 0.5f - 3, 
                    mid.y - ts.y * 0.5f - 2
                ),
                ImVec2(
                    mid.x + ts.x * 0.5f + 3, 
                    mid.y + ts.y * 0.5f + 2
                ),
                ImColor(30, 30, 40, 200), 4
            );
            draw_list->AddText(
                ImVec2(
                    mid.x - ts.x * 0.5f, 
                    mid.y - ts.y * 0.5f
                ), 
                ImColor(255,255,255), 
                w.c_str()
            );
        }
    }

    // Drawing nodes
    for (auto& [id, node] : m_graph.nodes)
    {
        ImVec2 screen_pos = world_to_screen(node.position);
        float size = (18.0f + node.value * 0.2f) / m_zoom;
        size = std::clamp(size, 10.0f, 40.0f);

        if (m_hovered_node == id) size *= 1.2f;
        if (m_selected_node == id) size *= 1.3f;

        // Getting current algorithm state for coloring
        bool is_current_node = false;
        bool is_frontier = false;
        bool is_path_node = false;

        if (m_execution)
        {
            const auto step = m_execution->get_current_step();

            // Checking if this is the current node being processed
            if (step.visualization.highlighted_index.has_value() &&
                step.visualization.highlighted_index.value() == id)
            {
                is_current_node = true;
            }

            // Checking if this is a frontier node
            for (size_t fn : step.visualization.graph_state.frontier_nodes)
            {
                if (fn == id)
                {
                    is_frontier = true;
                    break;
                }
            }

            // Checking if this is on the path (when target found)
            for (size_t pn : step.visualization.graph_state.path)
            {
                if (pn == id)
                {
                    is_path_node = true;
                    break;
                }
            }
        }

        // Determine node color based on state (priority order)
        ImU32 color;

        if (is_current_node)
        {
            // Current node being processed - Bright Red/Orange
            color = ImColor(255, 80, 80, 255);
        }
        else if (is_path_node)
        {
            // Path from start to target - Purple
            color = ImColor(155, 48, 255, 255);
        }
        else if (id == m_graph.start_node && !node.visited)
        {
            // Start node (not yet visited) - Green
            color = ImColor(0, 200, 0, 255);
        }
        else if (m_graph.target_node.has_value() && 
                 id == m_graph.target_node.value() && 
                 !node.visited)
        {
            // Target node - Red
            color = ImColor(255, 50, 50, 255);
        }
        else if (node.visited)
        {
            // Already visited nodes - Blue
            color = ImColor(65, 105, 225, 255);  // Royal Blue
        }
        else if (is_frontier)
        {
            // Frontier nodes (in queue) - Yellow/Orange
            color = ImColor(255, 200, 50, 255);
        }
        else
        {
            // Unvisited nodes (not yet discovered) - Gray
            color = ImColor(100, 100, 120, 200);
        }

        // Draw node circle
        draw_list->AddCircleFilled(screen_pos, size, color);

        // Draw border (thicker for current node)
        float border_thickness = is_current_node ? 3.0f : 1.5f;
        draw_list->AddCircle(
            screen_pos, 
            size, 
            ImColor(255, 255, 255, 180), 
            0, 
            border_thickness
        );

        // Draw inner glow for current node
        if (is_current_node)
        {
            draw_list->AddCircle(
                screen_pos, 
                size + 4, 
                ImColor(255, 80, 80, 100), 
                0, 
                2.0f
            );
        }

        // Draw node label
        if (m_show_node_labels)
        {
            std::string label = node.label.empty() ? std::to_string(id) : node.label;
            ImVec2 ts = ImGui::CalcTextSize(label.c_str());
            draw_list->AddText(
                ImVec2(
                    screen_pos.x - ts.x * 0.5f, 
                    screen_pos.y + size + 4
                ),
                ImColor(220, 220, 240), 
                label.c_str()
            );
        }

        if (m_show_node_ids)
        {
            std::string str_id = "ID: " + std::to_string(node.id);
            ImVec2 ts = ImGui::CalcTextSize(str_id.c_str());
            draw_list->AddText(
                ImVec2(
                    screen_pos.x - ts.x * 0.5f, 
                    screen_pos.y + size + 20
                ),
                ImColor(255, 204, 153), 
                str_id.c_str()
            );
        }

        // Drawing distance from start
        if (m_show_distances && node.distance >= 0)
        {
            std::string d = "d=" + std::to_string(node.distance);
            ImVec2 ts = ImGui::CalcTextSize(d.c_str());
            draw_list->AddText(
                ImVec2(
                    screen_pos.x - ts.x * 0.5f, 
                    screen_pos.y - size - 13),
                    ImColor(100, 200, 255), 
                    d.c_str()
                );
        }

        // Drawing node value inside the node
        std::string value_str = std::to_string(node.value);
        ImVec2 ts_val = ImGui::CalcTextSize(value_str.c_str());
        draw_list->AddText(
            ImVec2(
                screen_pos.x - ts_val.x * 0.5f, 
                screen_pos.y - ts_val.y * 0.5f
            ),
            ImColor(255, 255, 255, 220), 
            value_str.c_str()
        );
    }
}

    void GraphBasedVisualizer::handle_mouse_interaction()
    {
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mouse_pos = io.MousePos;
        ImVec2 viewport_pos = ImGui::GetCursorScreenPos();
        ImVec2 viewport_size = ImGui::GetContentRegionAvail();

        auto screen_to_world = [&](const ImVec2& screen) -> ImVec2
        {
            return {
                ((screen.x - viewport_pos.x) / m_zoom - m_view_offset.x) / viewport_size.x,
                ((screen.y - viewport_pos.y) / m_zoom - m_view_offset.y) / viewport_size.y
            };
        };

        // Update hovered node
        m_hovered_node = static_cast<size_t>(-1);
        for (const auto& [id, node] : m_graph.nodes)
        {
            ImVec2 screen_pos = ImVec2(
                viewport_pos.x + (node.position.x * viewport_size.x + m_view_offset.x) * m_zoom,
                viewport_pos.y + (node.position.y * viewport_size.y + m_view_offset.y) * m_zoom
            );
            float size = (15.0f + node.value * 0.3f) / m_zoom;
            if (std::hypot(mouse_pos.x - screen_pos.x, mouse_pos.y - screen_pos.y) <= size)
            {
                m_hovered_node = id;
                break;
            }
        }

        // Node dragging
        if (ImGui::IsMouseClicked(0) && 
            m_hovered_node != static_cast<size_t>(-1))
        {
            m_is_dragging_node = true;
            m_selected_node = m_hovered_node;
        }

        if (m_is_dragging_node && m_selected_node != static_cast<size_t>(-1))
        {
            if (ImGui::IsMouseDown(0))
            {
                ImVec2 world_pos = screen_to_world(mouse_pos);
                m_graph.nodes[m_selected_node].position = world_pos;
                m_graph.nodes[m_selected_node].is_fixed = true;
            }
            else
            {
                m_is_dragging_node = false;
            }
        }

        // Quick connect mode
        if (m_node_to_connect != static_cast<size_t>(-1) && 
            m_hovered_node != static_cast<size_t>(-1) && 
            ImGui::IsMouseClicked(0))
        {
            if (m_hovered_node != m_node_to_connect)
            {
                m_graph.add_edge(m_node_to_connect, m_hovered_node, 1);
                ImVec2 bounds(800, 600);
                m_layout_engine.compute_layout(m_graph, m_current_layout, bounds);
            }
            m_node_to_connect = static_cast<size_t>(-1);
        }

        // Panning
        if (ImGui::IsMouseClicked(2) || (io.KeyAlt && ImGui::IsMouseClicked(0)))
        {
            m_is_panning = true;
            m_pan_start = mouse_pos;
        }

        if (m_is_panning)
        {
            if (ImGui::IsMouseDown(2) || (io.KeyAlt && ImGui::IsMouseDown(0)))
            {
                ImVec2 delta(mouse_pos.x - m_pan_start.x, mouse_pos.y - m_pan_start.y);
                m_view_offset.x += delta.x / m_zoom;
                m_view_offset.y += delta.y / m_zoom;
                m_pan_start = mouse_pos;
            }
            else
            {
                m_is_panning = false;
            }
        }

        // Zoom
        if (ImGui::IsWindowHovered() && io.MouseWheel != 0)
        {
            float old_zoom = m_zoom;
            m_zoom *= (1.0f + io.MouseWheel * 0.1f);
            m_zoom = std::clamp(m_zoom, MIN_ZOOM, MAX_ZOOM);

            // Zoom to mouse position
            ImVec2 before = screen_to_world(mouse_pos);
            ImVec2 after = screen_to_world(mouse_pos);
            m_view_offset.x += (after.x - before.x) * viewport_size.x;
            m_view_offset.y += (after.y - before.y) * viewport_size.y;
        }
    }

    void GraphBasedVisualizer::handle_keyboard_shortcuts()
    {
        ImGuiIO& io = ImGui::GetIO();

        if (ImGui::IsKeyDown(ImGuiKey_Delete) && 
            m_selected_node != static_cast<size_t>(-1))
        {
            m_graph.remove_node(m_selected_node);
            m_selected_node = static_cast<size_t>(-1);
        }

        if (ImGui::IsKeyDown(ImGuiKey_R) && io.KeyCtrl)
        {
            ImVec2 bounds(800, 600);
            m_layout_engine.compute_layout(
                m_graph, 
                m_current_layout, 
                bounds
            );
        }

        if (ImGui::IsKeyDown(ImGuiKey_Home))
        {
            m_zoom = 1.0f;
            m_view_offset = ImVec2(0, 0);
        }
    }

    void GraphBasedVisualizer::sync_algorithm_with_graph()
    {
        if (!m_execution) return;

        // This would call a method on the algorithm to update its graph
        LOG_DEBUG(
            "Syncing graph with algorithm: {} nodes, {} edges",
            m_graph.node_count(),
            m_graph.edge_count()
        );

        // Example of how you would sync:
        if (auto* bfs = dynamic_cast<BFS*>(m_execution))
        {
            bfs->set_graph_structure(m_graph.get_adjacency_matrix());
            bfs->set_start_node(m_graph.start_node);
            bfs->set_target_node(m_graph.target_node);
            bfs->initialize(m_graph.get_node_values());
        }
        else if (auto* dfs = dynamic_cast<DFS*>(m_execution))
        {
            dfs->set_graph_structure(m_graph.get_adjacency_matrix());
            dfs->set_start_node(m_graph.start_node);
            dfs->set_target_node(m_graph.target_node);
            dfs->initialize(m_graph.get_node_values());
        }
        // Check for Dijkstra - uses weighted adjacency
        else if (auto* dijkstra = dynamic_cast<Dijkstra*>(m_execution))
        {
            // Build weighted adjacency list from graph
            std::vector<std::vector<std::pair<size_t, int>>> weighted_adj(m_graph.node_count());
            
            for (const auto& edge : m_graph.edges)
            {
                weighted_adj[edge.from].emplace_back(edge.to, edge.weight);
                if (!m_graph.is_directed)
                {
                    weighted_adj[edge.to].emplace_back(edge.from, edge.weight);
                }
            }
            
            dijkstra->set_graph_structure_weighted(weighted_adj);
            dijkstra->set_start_node(m_graph.start_node);
            dijkstra->set_target_node(m_graph.target_node);
            dijkstra->initialize(m_graph.get_node_values());
        }

        else if (auto* astar = dynamic_cast<AStar*>(m_execution))
        {
            // Build weighted adjacency list
            std::vector<std::vector<std::pair<size_t, int>>> weighted_adj(m_graph.node_count());
            
            for (const auto& edge : m_graph.edges)
            {
                weighted_adj[edge.from].emplace_back(edge.to, edge.weight);
                if (!m_graph.is_directed)
                {
                    weighted_adj[edge.to].emplace_back(edge.from, edge.weight);
                }
            }
            
            astar->set_graph_structure_weighted(weighted_adj);
            astar->set_start_node(m_graph.start_node);
            astar->set_target_node(m_graph.target_node.value_or(0));
            
            // Extract node positions from layout for heuristic
            std::vector<std::pair<float, float>> positions(m_graph.node_count());
            for (const auto& [id, node] : m_graph.nodes)
            {
                if (id < positions.size())
                {
                    positions[id] = {node.position.x, node.position.y};
                }
            }
            astar->set_node_positions(positions);
            astar->initialize(m_graph.get_node_values());
        }

        else if (auto* bellman_ford = dynamic_cast<BellmanFord*>(m_execution))
        {
            // Build weighted adjacency list from graph
            std::vector<std::vector<std::pair<size_t, int>>> weighted_adj(m_graph.node_count());

            for (const auto& edge : m_graph.edges)
            {
                weighted_adj[edge.from].emplace_back(edge.to, edge.weight);
                if (!m_graph.is_directed)
                {
                    weighted_adj[edge.to].emplace_back(edge.from, edge.weight);
                }
            }

            bellman_ford->set_graph_structure_weighted(weighted_adj);
            bellman_ford->set_start_node(m_graph.start_node);
            bellman_ford->set_target_node(m_graph.target_node);
            bellman_ford->initialize(m_graph.get_node_values());
        }
        else if (auto* topo = dynamic_cast<TopologicalSort*>(m_execution))
        {
            // For topological sort, ensure graph is directed
            m_graph.is_directed = true;
            topo->set_graph_structure(m_graph.get_adjacency_matrix());
            topo->set_graph_directed(true);
            topo->set_target_node(m_graph.target_node);
            topo->initialize(m_graph.get_node_values());
        }
    }

    VisualizationType GraphBasedVisualizer::get_visualization_type() const
    {
        return VisualizationType::GRAPH_BASED_VISUALIZATION;
    }

    bool GraphBasedVisualizer::supports_algorithm(const AlgorithmType& type) const
    {
        return type == AlgorithmType::BFS || 
               type == AlgorithmType::DFS ||
               type == AlgorithmType::DIJKSTRA || 
               type == AlgorithmType::A_STAR ||
               type == AlgorithmType::BELLMAN_FORD || 
               type == AlgorithmType::TOPOLOGICAL_SORT;
    }

    void GraphBasedVisualizer::set_visualization_style(VisualizationStyle style)
    {
        // Convert and apply (implementation can be expanded)
    }

    void GraphBasedVisualizer::set_graph_layout(const std::string& layout)
    {
        for (int i = 0; i < IM_ARRAYSIZE(LAYOUT_NAMES); ++i)
        {
            if (std::string(LAYOUT_NAMES[i]) == layout)
            {
                m_current_layout = static_cast<GraphLayoutEngine::LayoutType>(i);
                ImVec2 bounds(800, 600);
                m_layout_engine.compute_layout(
                    m_graph, 
                    m_current_layout,
                    bounds
                );
                return;
            }
        }
    }

    void GraphBasedVisualizer::set_show_edge_weights(bool show)
    {
        m_show_edge_weights = show;
    }

    void GraphBasedVisualizer::set_show_node_labels(bool show)
    {
        m_show_node_labels = show;
    }

    void GraphBasedVisualizer::set_show_node_ids(bool show)
    {
        m_show_node_ids = show;
    }

    void GraphBasedVisualizer::set_show_distances(bool show)
    {
        m_show_distances = show;
    }

    void GraphBasedVisualizer::sync_from_shared_state(const SharedGraphState& state)
    {
        m_graph = state.graph;

        // Sync to algorithm
        sync_algorithm_with_graph();
    }

} // namespace c2l::algorithms