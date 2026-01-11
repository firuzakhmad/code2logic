#ifndef CODE2LOGIC_I_ALGORITHM_VISUALIZER_HPP
#define CODE2LOGIC_I_ALGORITHM_VISUALIZER_HPP

#include "algorithms/i_simple_algorithm.hpp"
#include "algorithm_types.hpp"

namespace c2l::algorithms
{
	class IAlgorithmVisualizer
	{
	public:
		virtual ~IAlgorithmVisualizer() = default;

		virtual void initialize(const ISimpleAlgorithm* algorithm) 	= 0;
		virtual void render() 										= 0;
		virtual void update(double delta_time) 						= 0;

		[[nodiscard]] virtual VisualizationType get_visualization_type() const 	= 0;
		[[nodiscard]] virtual bool supports_algorithm(const AlgorithmType& type) const = 0;
	};
} // namespace c2l::algorithms

#endif // CODE2LOGIC_I_ALGORITHM_VISUALIZER_HPP