#ifndef CODE2LOGIC_ALGORITHMS_VISUALIZATION_STYLE_HPP
#define CODE2LOGIC_ALGORITHMS_VISUALIZATION_STYLE_HPP

#include <iostream>
#include <string>
#include <unordered_map>

namespace c2l::algorithms
{
	enum class VisualizationStyle
	{
	    CLASSIC_BARS = 0,
	    ENHANCED_BARS,
	    DOTS,
	    CIRCULAR,
	    NETWORK,
	    WAVEFORM,
	    HEATMAP,
	    PARTICLE_SYSTEM,
	    TREE_VIEW,
	    MOLECULAR,
	    NEURAL_NETWORK,
	    COUNT
	};

	[[nodiscard]] VisualizationStyle string_to_style(
		const std::string& style
	);
} // namespace c2l::algorithms

#endif // CODE2LOGIC_ALGORITHMS_VISUALIZATION_STYLE_HPP
