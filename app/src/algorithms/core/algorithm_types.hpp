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

        // Path_finding
        DIJKSTRA,
        A_STAR,
        BELLMAN_FORD,

        // Tree
        BST_INSERT,
        BST_SEARCH,
        AVL_INSERT,

        UNKNOWN
    };

    enum class AlgorithmCategory : uint8_t
    {
        SORTING,
        SEARCHING,
        GRAPH,
        PATH_FINDING,
        TREE,
        DYNAMIC_PROGRAMMING,
        DATA_STRUCTURES,

        UNKNOWN
    };

    enum class VisualizationType : uint8_t
    {
        ARRAY_BASED,
        GRAPH_BASED,
        TREE_BASED,
        GRID_BASED,
        COMPARISON_BASED,

        UNKNOWN
    };

    struct AlgorithmInfo
    {
        std::string_view    id;            // JSON / filename / stable key
        std::string_view    display_name;  // UI only
        AlgorithmType       type;
        AlgorithmCategory   category;
        std::string_view    display_category;
        VisualizationType  visualization;
    };

    constexpr AlgorithmInfo ALGORITHMS[] =
    {
        // Sorting
        { 
            "bubble_sort",      
            "Bubble Sort",      
            AlgorithmType::BUBBLE_SORT,      
            AlgorithmCategory::SORTING,       
            "Sorting",      
            VisualizationType::ARRAY_BASED 
        },
        { 
            "quick_sort",       
            "Quick Sort",       
            AlgorithmType::QUICK_SORT,      
            AlgorithmCategory::SORTING,
            "Sorting",       
            VisualizationType::ARRAY_BASED 
        },
        { 
            "merge_sort",       
            "Merge Sort",       
            AlgorithmType::MERGE_SORT,     
            AlgorithmCategory::SORTING,
            "Sorting",
            VisualizationType::ARRAY_BASED 
        },
        { 
            "insertion_sort",   
            "Insertion Sort",   
            AlgorithmType::INSERTION_SORT,   
            AlgorithmCategory::SORTING,
            "Sorting",       
            VisualizationType::ARRAY_BASED 
        },
        { 
            "selection_sort",   
            "Selection Sort",   
            AlgorithmType::SELECTION_SORT,
            AlgorithmCategory::SORTING,
            "Sorting",       
            VisualizationType::ARRAY_BASED 
        },
        { 
            "heap_sort",        
            "Heap Sort",        
            AlgorithmType::HEAP_SORT,      
            AlgorithmCategory::SORTING,
            "Sorting",       
            VisualizationType::ARRAY_BASED 
        },

        // Searching
        { 
            "binary_search",    
            "Binary Search",    
            AlgorithmType::BINARY_SEARCH,    
            AlgorithmCategory::SEARCHING,
            "Searching",     
            VisualizationType::ARRAY_BASED 
        },
        { 
            "linear_search",    
            "Linear Search",    
            AlgorithmType::LINEAR_SEARCH,  
            AlgorithmCategory::SEARCHING,
            "Searching",     
            VisualizationType::ARRAY_BASED 
        },
        { 
            "interpolation_search",
            "Interpolation Search",
            AlgorithmType::INTERPOLATION_SEARCH,
            AlgorithmCategory::SEARCHING,
            "Searching", 
            VisualizationType::ARRAY_BASED 
        },

        // Graph
        { 
            "bfs",              
            "Breadth-First Search", 
            AlgorithmType::BFS,           
            AlgorithmCategory::GRAPH,
            "Graph",
            VisualizationType::GRAPH_BASED 
        },
        { 
            "dfs",              
            "Depth-First Search",   
            AlgorithmType::DFS,          
            AlgorithmCategory::GRAPH,
            "Graph",      
            VisualizationType::GRAPH_BASED 
        },
        { 
            "topological_sort", 
            "Topological Sort",     
            AlgorithmType::TOPOLOGICAL_SORT,
            AlgorithmCategory::GRAPH,
            "Graph",      
            VisualizationType::GRAPH_BASED 
        },

        // Path_finding
        { 
            "dijkstra",         
            "Dijkstra",         
            AlgorithmType::DIJKSTRA,    
            AlgorithmCategory::PATH_FINDING,
            "Path Finding",
            VisualizationType::GRID_BASED 
        },
        { 
            "a_star",           
            "A*",               
            AlgorithmType::A_STAR,          
            AlgorithmCategory::PATH_FINDING,
            "Finding",  
            VisualizationType::GRID_BASED 
        },
        { 
            "bellman_ford",     
            "Bellman-Ford",     
            AlgorithmType::BELLMAN_FORD,
            AlgorithmCategory::PATH_FINDING,
            "Finding",  
            VisualizationType::GRID_BASED 
        },

        // Tree
        { 
            "bst_insert",       
            "BST Insert",       
            AlgorithmType::BST_INSERT,
            AlgorithmCategory::TREE,
            "Tree",          
            VisualizationType::TREE_BASED 
        },
        { 
            "bst_search",       
            "BST Search",       
            AlgorithmType::BST_SEARCH,  
            AlgorithmCategory::TREE,
            "Tree",          
            VisualizationType::TREE_BASED 
        },
        { 
            "avl_insert",       
            "AVL Insert",       
            AlgorithmType::AVL_INSERT,   
            AlgorithmCategory::TREE,
            "Tree",          
            VisualizationType::TREE_BASED 
        }
    };

    inline const AlgorithmInfo* get_algorithm_info(AlgorithmType type)
    {
        for (const auto& algo : ALGORITHMS)
            if (algo.type == type)
                return &algo;

        return nullptr;
    }

    inline AlgorithmType id_to_algorithm_type(std::string_view id)
    {
        for (const auto& algo : ALGORITHMS)
            if (algo.id == id)
                return algo.type;

        return AlgorithmType::UNKNOWN;
    }

    inline std::string_view algorithm_display_name(AlgorithmType type)
    {
        if (const auto* info = get_algorithm_info(type))
            return info->display_name;

        return "Unknown Algorithm";
    }

    inline std::string_view algorithm_display_category(AlgorithmType type)
    {
        if (const auto* info = get_algorithm_info(type))
            return info->display_category;

        return "Unknown Algorithm";
    }

    inline std::string_view algorithm_id(AlgorithmType type)
    {
        if (const auto* info = get_algorithm_info(type))
            return info->id;

        return "unknown";
    }

    inline AlgorithmCategory algorithm_category(AlgorithmType type)
    {
        if (const auto* info = get_algorithm_info(type))
            return info->category;

        return AlgorithmCategory::UNKNOWN;
    }

    inline VisualizationType visualization_type(AlgorithmType type)
    {
        if (const auto* info = get_algorithm_info(type))
            return info->visualization;

        return VisualizationType::ARRAY_BASED;
    }

} // namespace c2l::algorithms

#endif // CODE2LOGIC_ALGORITHM_TYPES_HPP
