#ifndef CODE2LOGIC_ARRAY_BASED_VISUALIZER_HPP
#define CODE2LOGIC_ARRAY_BASED_VISUALIZER_HPP

#include "i_algorithm_visualizer.hpp"
#include <imgui.h>

namespace c2l::algorithms
{
	class ArrayBasedVisualizer final : public IAlgorithmVisualizer
	{
	public:
		ArrayBasedVisualizer() = default;
		~ArrayBasedVisualizer() override = default;

		void initialize(
			const ISimpleAlgorithm* execution,
        	const IAlgorithmMetadata* metadata
		) override;
        void render() override;
        void update(double delta_time) override;

        [[nodiscard]] VisualizationType get_visualization_type() const override;
		[[nodiscard]] bool supports_algorithm(const AlgorithmType& type) const override;

	private:
		struct TreeNode {
			int value;
			size_t left_child;
			size_t right_child;
			size_t parent;

			explicit TreeNode(int val = 0) 
				: value(val)
				, left_child(static_cast<size_t>(-1))
				, right_child(static_cast<size_t>(-1))
				, parent(static_cast<size_t>(-1)) 
			{}
		};

		struct Particle
		{
			ImVec2 position;
			ImVec2 velocity;
			ImVec2 target_position;
			int value{};
			float size{};
			bool is_active{};
		};


		const ISimpleAlgorithm* m_execution 	{nullptr};
    	const IAlgorithmMetadata* m_metadata 	{nullptr};
		int m_visualization_style 				{0};
		
		std::vector<Particle> m_particles;
		double m_last_update_time = 0.0;

		void render_header();
        void render_array_visualization(
			const AlgorithmStep& step
		);
        void render_sorting_visualization(
			const AlgorithmStep& step
		);
        void render_searching_visualization(
			const AlgorithmStep& step
		);
        void render_bar_visualization(
			const AlgorithmStep& step
		);
        void render_dot_visualization(
			const AlgorithmStep& step
		);
		void render_enhanced_bar_visualization(
			const AlgorithmStep& step
		);
		void render_circular_visualization(
			const AlgorithmStep& step
		);
		void render_network_visualization(
			const AlgorithmStep& step
		);
		void render_waveform_visualization(
			const AlgorithmStep& step
		);
		void render_heatmap_visualization(
			const AlgorithmStep& step
		);
		void render_particle_visualization(
			const AlgorithmStep& step
		);
		void render_tree_visualization(
			const AlgorithmStep& step
		);
		void render_molecular_visualization(
			const AlgorithmStep& step
		);
		void render_neural_network_visualization(
			const AlgorithmStep& step
		);
		void draw_neural_nodes(
			ImDrawList* draw_list, 
			const std::vector<ImVec2>& nodes,
			const AlgorithmStep& step, 
			const std::string& layer_name
		);

		void draw_comparison_line(
			ImDrawList* draw_list,
			const ImVec2& cursor_pos,
			const AlgorithmStep& step,
			float bar_width,
			float spacing,
			float start_y
		);

        ImU32 get_element_color(
			const AlgorithmStep& step, 
			size_t index
		);
		ImU32 get_enhanced_element_color(
			const AlgorithmStep& step, 
			size_t index
		);
		ImU32 value_to_rainbow_color(
			float ratio
		);
		ImU32 apply_color_variation(
			ImU32 color, 
			float factor
		);
		std::vector<ImVec2> calculate_network_positions(
			const std::vector<int>& data, 
			const ImVec2& region_size
		);
		ImU32 get_network_node_color(
			const AlgorithmStep& step, 
			size_t index, 
			float value_ratio
		);
		ImU32 get_tree_node_color(
			const AlgorithmStep& step, 
			size_t index, 
			float value_ratio
		);
		std::vector<TreeNode> build_binary_tree(
			const std::vector<int>& data
		);
		std::vector<ImVec2> calculate_tree_positions(
			size_t node_count, 
			const ImVec2& region_size
		);

		std::vector<ImVec2> calculate_node_positions(
			size_t count, 
			const ImVec2& region_size
		);
		ImU32 value_to_heatmap_color(
			float ratio
		);
		void initialize_particles(
			const std::vector<int>& data
		);
		void update_particles(
			const AlgorithmStep& step
		);
		void draw_particle_connections(
			ImDrawList* draw_list, 
			const ImVec2& cursor_pos,
			const ImVec2& region_size, 
			const AlgorithmStep& step
		);
		ImU32 get_particle_color(
			const AlgorithmStep& step, 
			size_t index
		);
		std::vector<ImVec2> calculate_tree_layout(
			size_t node_count, 
			const ImVec2& region_size
		);
		ImU32 get_molecular_color(
			const AlgorithmStep& step, 
			size_t index, 
			float radius_ratio
		);
		std::vector<ImVec2> calculate_layer_positions(
			int node_count, 
			const ImVec2& region_size, 
			float x_ratio
		);
		ImU32 get_neural_connection_color(
			float weight
		);
	};
}

#endif // CODE2LOGIC_ARRAY_BASED_VISUALIZER_HPP