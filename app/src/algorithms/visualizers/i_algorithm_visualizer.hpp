#ifndef CODE2LOGIC_I_ALGORITHM_VISUALIZER_HPP
#define CODE2LOGIC_I_ALGORITHM_VISUALIZER_HPP

#include "algorithms/core/i_simple_algorithm.hpp"
#include "algorithms/core/algorithm_types.hpp"
#include "algorithms/core/i_algorithm_metadata.hpp"

namespace c2l::algorithms
{
	class IAlgorithmVisualizer
	{
	public:
		explicit IAlgorithmVisualizer() = default;
		virtual ~IAlgorithmVisualizer() = default;

		virtual void initialize(
			ISimpleAlgorithm* execution,
       		const IAlgorithmMetadata* metadata,
       		const bool show_sidebar_controller) 					= 0;
		virtual void render() 										= 0;
		virtual void update(double delta_time) 						= 0;

		[[nodiscard]] virtual VisualizationType 
		get_visualization_type() const 	= 0;
		[[nodiscard]] virtual bool 
		supports_algorithm(const AlgorithmType& type) const = 0;

		// Graph-specific methods
		virtual void set_visualization_style(
			[[maybe_unused]] VisualizationStyle style)
		{}
        virtual void set_graph_layout(
        	[[maybe_unused]] const std::string& layout) 			
        {}
        virtual void set_show_edge_weights(
        	[[maybe_unused]] bool show) 							
        {}
        virtual void set_show_node_labels(
        	[[maybe_unused]] bool show) 				
        {}
        virtual void set_show_distances(
        	[[maybe_unused]] bool show) 					
        {}

		[[nodiscard]] virtual size_t get_total_visited_nodes() const
		{
			return 0;
		}

		[[nodiscard]] virtual size_t get_total_explored_nodes() const
		{
			return 0;
		}

	protected:
		bool m_show_sidebar_controller	{false};
	};
} // namespace c2l::algorithms

#endif // CODE2LOGIC_I_ALGORITHM_VISUALIZER_HPP