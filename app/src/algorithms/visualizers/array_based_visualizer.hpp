#ifndef CODE2LOGIC_ARRAY_BASED_VISUALIZER_HPP
#define CODE2LOGIC_ARRAY_BASED_VISUALIZER_HPP

#include "algorithms/visualizers/i_algorithm_visualizer.hpp"
#include "algorithms/core/algorithm_metadata_types.hpp"
#include "algorithms/visualizers/visualization_style.hpp"

#include <imgui.h>

namespace c2l::algorithms
{
	class ArrayBasedVisualizer final : public IAlgorithmVisualizer
	{
	public:
		explicit ArrayBasedVisualizer(
			const VisualizationConfig& visualization_config = {}
		);
		~ArrayBasedVisualizer() override = default;

		void initialize(
			const ISimpleAlgorithm* execution,
        	const IAlgorithmMetadata* metadata
		) override;
        void update(double delta_time) override;
        void render() override;

        [[nodiscard]] VisualizationType get_visualization_type() const override;
		[[nodiscard]] bool supports_algorithm(const AlgorithmType& type) const override;

        void set_visualization_style(VisualizationStyle style);
        [[nodiscard]] VisualizationStyle get_visualization_style() const;

	private:
        struct TreeNode 
        {
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
            float lifetime{1.0f};
        };

        void render_array_visualization(
        	const AlgorithmStep& step
    	);

    	void render_classic_bars(
    		const AlgorithmStep& step
		);
		void render_enhanced_bars(
			const AlgorithmStep& step
		);
		void render_dots(
			const AlgorithmStep& step
		);
		void render_circular(
			const AlgorithmStep& step
		);
		void render_network(
			const AlgorithmStep& step
		);
		void render_waveform(
			const AlgorithmStep& step
		);
		void render_heatmap(
			const AlgorithmStep& step
		);
		void render_particle_system(
        	const AlgorithmStep& step
    	);
    	void render_tree_view(
        	const AlgorithmStep& step
    	);
    	void render_molecular(
        	const AlgorithmStep& step
    	);
    	void render_neural_network(
        	const AlgorithmStep& step
    	);


		float calculate_bar_width(
	        size_t data_size, 
	        float available_width
	    ) const; 
	    float calculate_max_bar_height(
	    	const ImVec2& region_size
    	) const;

    	ImU32 get_element_color(
	        const AlgorithmStep& step, 
	        size_t index
        ) const;

        ImU32 get_enhanced_element_color(
	        const AlgorithmStep& step, 
	        size_t index
        ) const;

        ImU32 value_to_rainbow_color(
        	float ratio
    	) const;

        ImU32 apply_color_variation(
        	ImU32 color, 
        	float factor
    	) const;

    	ImVec2 calculate_center(
	        const ImVec2& region_size
	    ) const;

	    std::vector<ImVec2> calculate_network_positions(
	        const std::vector<int>& data, 
	        const ImVec2& region_size
        ) const;

        ImU32 get_network_node_color(
	        const AlgorithmStep& step, 
	        size_t index, 
	        float value_ratio
        ) const;

        ImU32 value_to_heatmap_color(
	        float ratio
	    ) const;

	    void initialize_particles(
	    	const std::vector<int>& data
    	);

    	void update_particles(
    		const AlgorithmStep& step, 
    		float delta_time
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
    	) const;

    	std::vector<ArrayBasedVisualizer::TreeNode> 
    	build_binary_tree(const std::vector<int>& data
    	) const;

    	std::vector<ImVec2> calculate_tree_positions(
	        size_t node_count, 
	        const ImVec2& region_size
        ) const;

    	ImU32 get_tree_node_color(
	        const AlgorithmStep& step, 
	        size_t index, 
	        float value_ratio
        ) const;

        ImU32 get_neural_connection_color(
        	float weight
    	) const;

    	void draw_neural_nodes(
	        ImDrawList* draw_list, 
	        const std::vector<ImVec2>& nodes,
	        const AlgorithmStep& step, 
	        const std::string& layer_name
        ) const;

        std::vector<ImVec2> calculate_layer_positions(
	        int node_count, 
	        const ImVec2& region_size, 
	        float x_ratio
        ) const;

        ImU32 get_molecular_color(
            const AlgorithmStep& step, 
            size_t index, 
            float radius_ratio
        ) const;


        // Members
		const ISimpleAlgorithm* m_execution		{nullptr};
        const IAlgorithmMetadata* m_metadata	{nullptr};
        const VisualizationConfig& m_visualization_config;
        
        VisualizationStyle m_current_style		{VisualizationStyle::CLASSIC_BARS};
        
        std::vector<Particle> m_particles;
        double m_last_update_time				{0.0};

        // Animation state
        float m_animation_time					{0.0f};
        bool m_has_initialized_particles		{false};
        
        // Cached values for responsiveness
        ImVec2 m_last_window_size				{0, 0};
        
        static constexpr const char* STYLE_NAMES[] = {
		    "Classic Bars", "Enhanced Bars", "Dots", "Circular", 
		    "Network", "Waveform", "Heatmap", "Particle System",
		    "Tree View", "Molecular", "Neural Network"
		};
    };
}

#endif // CODE2LOGIC_ARRAY_BASED_VISUALIZER_HPP