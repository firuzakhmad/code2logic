#include "algorithms/visualizers/visualization_style.hpp"

namespace c2l::algorithms
{
	[[nodiscard]] VisualizationStyle string_to_style(
		const std::string& style)
	{
	    static const std::unordered_map<std::string, VisualizationStyle> map = {
	        {"bars", VisualizationStyle::CLASSIC_BARS},
	        {"classic_bars", VisualizationStyle::CLASSIC_BARS},
	        {"enhanced_bars", VisualizationStyle::ENHANCED_BARS},
	        {"dots", VisualizationStyle::DOTS},
	        {"circular", VisualizationStyle::CIRCULAR},
	        {"network", VisualizationStyle::NETWORK},
	        {"waveform", VisualizationStyle::WAVEFORM},
	        {"heatmap", VisualizationStyle::HEATMAP},
	        {"particle", VisualizationStyle::PARTICLE_SYSTEM},
	        {"tree", VisualizationStyle::TREE_VIEW},
	        {"molecular", VisualizationStyle::MOLECULAR},
	        {"neural", VisualizationStyle::NEURAL_NETWORK}
	    };

	    auto it = map.find(style);
	    if (it != map.end())
	        return it->second;

	    return VisualizationStyle::CLASSIC_BARS; // fallback
	}
}