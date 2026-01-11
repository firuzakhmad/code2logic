#ifndef CODE2LOGIC_ALGORITHM_TYPES_HPP
#define CODE2LOGIC_ALGORITHM_TYPES_HPP

#include <cstdint>
#include <string>

namespace c2l::algorithms
{
    enum class AlgorithmType : uint8_t
    {
        // Sorting
        BUBBLE_SORT,
        QUICK_SORT,
        MERGE_SORT,
        INSERTION_SORT,
        SELECTION_SORT,
        HEAP_SORT,

        // Searching
        BINARY_SEARCH,
        LINEAR_SEARCH,
        INTERPOLATION_SEARCH,

        // Graph
        BFS,
        DFS,
        TOPOLOGICAL_SORT,

        // Pathfinding
        DIJKSTRA,
        A_STAR,
        BELLMAN_FORD,

        // Tree
        BST_INSERT,
        BST_SEARCH,
        AVL_INSERT,

        UNKNOWN,
        COUNT
    };

    // Visualization strategy
    enum class VisualizationType : uint8_t
    {
        ARRAY_BASED,
        GRAPH_BASED,
        TREE_BASED,
        GRID_BASED,
        COMPARISON_BASED
    };

    // Algorithm categories
    enum class AlgorithmCategory : uint8_t
    {
        SORTING,
        SEARCHING,
        GRAPH,
        PATHFINDING,
        TREE,
        DYNAMIC_PROGRAMMING,
        DATA_STRUCTURES,

        UNKNOWN
    };

    inline const char* algorithm_type_to_string(AlgorithmType type)
    {
        switch (type)
        {
            case AlgorithmType::BUBBLE_SORT:           return "Bubble Sort";
            case AlgorithmType::QUICK_SORT:            return "Quick Sort";
            case AlgorithmType::MERGE_SORT:            return "Merge Sort";
            case AlgorithmType::INSERTION_SORT:        return "Insertion Sort";
            case AlgorithmType::SELECTION_SORT:        return "Selection Sort";
            case AlgorithmType::HEAP_SORT:             return "Heap Sort";

            case AlgorithmType::BINARY_SEARCH:         return "Binary Search";
            case AlgorithmType::LINEAR_SEARCH:         return "Linear Search";
            case AlgorithmType::INTERPOLATION_SEARCH:  return "Interpolation Search";

            case AlgorithmType::BFS:                   return "Breadth-First Search";
            case AlgorithmType::DFS:                   return "Depth-First Search";
            case AlgorithmType::TOPOLOGICAL_SORT:      return "Topological Sort";

            case AlgorithmType::DIJKSTRA:              return "Dijkstra";
            case AlgorithmType::A_STAR:                return "A*";
            case AlgorithmType::BELLMAN_FORD:          return "Bellman-Ford";

            case AlgorithmType::BST_INSERT:            return "BST Insert";
            case AlgorithmType::BST_SEARCH:            return "BST Search";
            case AlgorithmType::AVL_INSERT:            return "AVL Insert";

            default:                                   return "Unknown Algorithm";
        }
    }

    inline AlgorithmType string_to_algorithm_type(const std::string& name)
    {
        if (name == "Bubble Sort")             return AlgorithmType::BUBBLE_SORT;
        if (name == "Quick Sort")              return AlgorithmType::QUICK_SORT;
        if (name == "Merge Sort")              return AlgorithmType::MERGE_SORT;
        if (name == "Insertion Sort")          return AlgorithmType::INSERTION_SORT;
        if (name == "Selection Sort")          return AlgorithmType::SELECTION_SORT;
        if (name == "Heap Sort")               return AlgorithmType::HEAP_SORT;

        if (name == "Binary Search")           return AlgorithmType::BINARY_SEARCH;
        if (name == "Linear Search")           return AlgorithmType::LINEAR_SEARCH;
        if (name == "Interpolation Search")    return AlgorithmType::INTERPOLATION_SEARCH;

        if (name == "Breadth-First Search")    return AlgorithmType::BFS;
        if (name == "Depth-First Search")      return AlgorithmType::DFS;
        if (name == "Topological Sort")        return AlgorithmType::TOPOLOGICAL_SORT;

        if (name == "Dijkstra")                return AlgorithmType::DIJKSTRA;
        if (name == "A*")                      return AlgorithmType::A_STAR;
        if (name == "Bellman-Ford")            return AlgorithmType::BELLMAN_FORD;

        if (name == "BST Insert")              return AlgorithmType::BST_INSERT;
        if (name == "BST Search")              return AlgorithmType::BST_SEARCH;
        if (name == "AVL Insert")              return AlgorithmType::AVL_INSERT;

        return AlgorithmType::UNKNOWN;
    }

    // Categorization
    inline AlgorithmCategory get_algorithm_category(AlgorithmType type)
    {
        switch (type)
        {
            case AlgorithmType::BUBBLE_SORT:
            case AlgorithmType::QUICK_SORT:
            case AlgorithmType::MERGE_SORT:
            case AlgorithmType::INSERTION_SORT:
            case AlgorithmType::SELECTION_SORT:
            case AlgorithmType::HEAP_SORT:
                return AlgorithmCategory::SORTING;

            case AlgorithmType::BINARY_SEARCH:
            case AlgorithmType::LINEAR_SEARCH:
            case AlgorithmType::INTERPOLATION_SEARCH:
                return AlgorithmCategory::SEARCHING;

            case AlgorithmType::BFS:
            case AlgorithmType::DFS:
            case AlgorithmType::TOPOLOGICAL_SORT:
                return AlgorithmCategory::GRAPH;

            case AlgorithmType::DIJKSTRA:
            case AlgorithmType::A_STAR:
            case AlgorithmType::BELLMAN_FORD:
                return AlgorithmCategory::PATHFINDING;

            case AlgorithmType::BST_INSERT:
            case AlgorithmType::BST_SEARCH:
            case AlgorithmType::AVL_INSERT:
                return AlgorithmCategory::TREE;

            default:
                return AlgorithmCategory::UNKNOWN;
        }
    }

    inline const char* algorithm_category_to_string(const AlgorithmCategory category)
    {
        switch (category)
        {
            case AlgorithmCategory::SORTING:                return "Sorting";
            case AlgorithmCategory::SEARCHING:              return "Searching";
            case AlgorithmCategory::GRAPH:                  return "Graph";
            case AlgorithmCategory::PATHFINDING:            return "Pathfinding";
            case AlgorithmCategory::DYNAMIC_PROGRAMMING:    return "Dynamic Programming";
            case AlgorithmCategory::DATA_STRUCTURES:        return "Data Structures";

            default: return "Unknown algorithm category";
        }
    }

    // ----------------------------
    // Visualization selection
    // ----------------------------
    inline VisualizationType get_visualization_type(AlgorithmType type)
    {
        switch (get_algorithm_category(type))
        {
            case AlgorithmCategory::SORTING:
            case AlgorithmCategory::SEARCHING:
                return VisualizationType::ARRAY_BASED;

            case AlgorithmCategory::GRAPH:
                return VisualizationType::GRAPH_BASED;

            case AlgorithmCategory::PATHFINDING:
                return VisualizationType::GRID_BASED;

            case AlgorithmCategory::TREE:
                return VisualizationType::TREE_BASED;

            default:
                return VisualizationType::ARRAY_BASED;
        }
    }

} // namespace c2l::algorithms

#endif // CODE2LOGIC_ALGORITHM_TYPES_HPP
