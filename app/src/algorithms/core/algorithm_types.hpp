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
        JUMP_SEARCH,
        INTERPOLATION_SEARCH,

        // Graph
        BFS,
        DFS,
        TOPOLOGICAL_SORT,
        A_STAR,
        DIJKSTRA,
        FLOYD_WARSHALL,
        

        // Path_finding
        GRID_DIJKSTRA,
        GRID_A_STAR, 
        JUMP_POINT_SEARCH,
        BELLMAN_FORD,
        PRIMS_MST,
        GRID_DFS, 
        THETA_STAR, 
        BEST_FIRST_SEARCH,
        TRACE,

        // Tree
        BST_INSERT,
        BST_SEARCH,
        AVL_INSERT,
        GRID_BFS,

        UNKNOWN, 
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
        ARRAY_BASED_VISUALIZATION,
        PATH_FINDING_BASED_VISUALIZATION,
        GRAPH_BASED_VISUALIZATION,
        TREE_BASED_VISUALIZATION,
        GRID_BASED_VISUALIZATION,
        COMPARISON_BASED_VISUALIZATION,

        UNKNOWN
    };

    struct AlgorithmInfo
    {
        std::string_view    id;            // JSON / filename / stable key
        std::string_view    display_name;  // UI only
        AlgorithmType       type;
        AlgorithmCategory   category;
        std::string_view    display_category;
        VisualizationType   visualization;
        std::string_view    display_visualization;
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
            VisualizationType::ARRAY_BASED_VISUALIZATION,
            "Array Based Visualization"
        },
        { 
            "quick_sort",       
            "Quick Sort",       
            AlgorithmType::QUICK_SORT,      
            AlgorithmCategory::SORTING,
            "Sorting",       
            VisualizationType::ARRAY_BASED_VISUALIZATION,
            "Array Based Visualization"
        },
        { 
            "merge_sort",       
            "Merge Sort",       
            AlgorithmType::MERGE_SORT,     
            AlgorithmCategory::SORTING,
            "Sorting",
            VisualizationType::ARRAY_BASED_VISUALIZATION,
            "Array Based Visualization"
        },
        { 
            "insertion_sort",   
            "Insertion Sort",   
            AlgorithmType::INSERTION_SORT,   
            AlgorithmCategory::SORTING,
            "Sorting",       
            VisualizationType::ARRAY_BASED_VISUALIZATION,
            "Array Based Visualization"
        },
        { 
            "selection_sort",   
            "Selection Sort",   
            AlgorithmType::SELECTION_SORT,
            AlgorithmCategory::SORTING,
            "Sorting",       
            VisualizationType::ARRAY_BASED_VISUALIZATION,
            "Array Based Visualization"
        },
        { 
            "heap_sort",        
            "Heap Sort",        
            AlgorithmType::HEAP_SORT,      
            AlgorithmCategory::SORTING,
            "Sorting",       
            VisualizationType::ARRAY_BASED_VISUALIZATION,
            "Array Based Visualization"
        },

        // Searching
        { 
            "binary_search",    
            "Binary Search",    
            AlgorithmType::BINARY_SEARCH,    
            AlgorithmCategory::SEARCHING,
            "Searching",     
            VisualizationType::ARRAY_BASED_VISUALIZATION,
            "Array Based Visualization"
        },
        { 
            "linear_search",    
            "Linear Search",    
            AlgorithmType::LINEAR_SEARCH,  
            AlgorithmCategory::SEARCHING,
            "Searching",     
            VisualizationType::ARRAY_BASED_VISUALIZATION,
            "Array Based Visualization"
        },
        {
            "jump_search",
            "Jump Search",
            AlgorithmType::JUMP_SEARCH,
            AlgorithmCategory::SEARCHING,
            "Searching",
            VisualizationType::ARRAY_BASED_VISUALIZATION,
            "Array Based Visualization"
        },
        { 
            "interpolation_search",
            "Interpolation Search",
            AlgorithmType::INTERPOLATION_SEARCH,
            AlgorithmCategory::SEARCHING,
            "Searching", 
            VisualizationType::ARRAY_BASED_VISUALIZATION,
            "Array Based Visualization"
        },

        // Graph
        { 
            "bfs",              
            "Breadth-First Search", 
            AlgorithmType::BFS,           
            AlgorithmCategory::GRAPH,
            "Graph",
            VisualizationType::GRAPH_BASED_VISUALIZATION,
            "Graph Based Visualization"
        },
        { 
            "dfs",              
            "Depth-First Search",   
            AlgorithmType::DFS,          
            AlgorithmCategory::GRAPH,
            "Graph",      
            VisualizationType::GRAPH_BASED_VISUALIZATION,
            "Graph Based Visualization"
        },
        { 
            "topological_sort", 
            "Topological Sort",     
            AlgorithmType::TOPOLOGICAL_SORT,
            AlgorithmCategory::GRAPH,
            "Graph",      
            VisualizationType::GRAPH_BASED_VISUALIZATION,
            "Graph Based Visualization"
        },

        // Path_finding
        { 
            "dijkstra",         
            "Dijkstra",         
            AlgorithmType::DIJKSTRA,    
            AlgorithmCategory::GRAPH,
            "Graph",
            VisualizationType::GRAPH_BASED_VISUALIZATION,
            "Graph Based Visualization"
        },
        {
            "prim",
            "Prim's Algorithm",
            AlgorithmType::PRIMS_MST,
            AlgorithmCategory::GRAPH,
            "Graph",
            VisualizationType::GRAPH_BASED_VISUALIZATION,
            "Graph Based Visualization"
        },
        { 
            "a_star",           
            "A*",               
            AlgorithmType::A_STAR,          
            AlgorithmCategory::GRAPH,
            "Graph",  
            VisualizationType::GRAPH_BASED_VISUALIZATION,
            "Graph Based Visualization"
        },
        {
            "grid_a_star",
            "A*",
            AlgorithmType::GRID_A_STAR,
            AlgorithmCategory::PATH_FINDING,
            "Path Finding",
            VisualizationType::PATH_FINDING_BASED_VISUALIZATION,
            "Path Finding Based Visualization"
        },
        {
            "jump_point_search",
            "Jump Point Search (JPS)",
            AlgorithmType::JUMP_POINT_SEARCH,
            AlgorithmCategory::PATH_FINDING,
            "Path Finding",
            VisualizationType::PATH_FINDING_BASED_VISUALIZATION,
            "Path Finding Based Visualization"
        },
        {
            "grid_dijkstra",
            "Dijkstra's Algorithm",
            AlgorithmType::GRID_DIJKSTRA,
            AlgorithmCategory::PATH_FINDING,
            "Path Finding",
            VisualizationType::PATH_FINDING_BASED_VISUALIZATION,
            "Path Finding Based Visualization"
        },
        {
            "grid_bfs",
            "Breadth-First Search",
            AlgorithmType::GRID_BFS,
            AlgorithmCategory::PATH_FINDING,
            "Path Finding",
            VisualizationType::PATH_FINDING_BASED_VISUALIZATION,
            "Path Finding Based Visualization"
        },
        {
            "grid_dfs",
            "Depth-First Search",
            AlgorithmType::GRID_DFS,
            AlgorithmCategory::PATH_FINDING,
            "Path Finding",
            VisualizationType::PATH_FINDING_BASED_VISUALIZATION,
            "Path Finding Based Visualization"
        },
        {
            "theta_star",
            "Theta*",
            AlgorithmType::THETA_STAR,
            AlgorithmCategory::PATH_FINDING,
            "Path Finding",
            VisualizationType::PATH_FINDING_BASED_VISUALIZATION,
            "Path Finding Based Visualization"
        },
        {
            "best_first_search",
            "Best-First Search",
            AlgorithmType::BEST_FIRST_SEARCH,
            AlgorithmCategory::PATH_FINDING,
            "Path Finding",
            VisualizationType::PATH_FINDING_BASED_VISUALIZATION,
            "Path Finding Based Visualization"
        },
        {
            "trace",
            "Trace",
            AlgorithmType::TRACE,
            AlgorithmCategory::PATH_FINDING,
            "Path Finding",
            VisualizationType::PATH_FINDING_BASED_VISUALIZATION,
            "Path Finding Based Visualization"
        },
        { 
            "bellman_ford",     
            "Bellman-Ford",     
            AlgorithmType::BELLMAN_FORD,
            AlgorithmCategory::GRAPH,
            "Graph",
            VisualizationType::GRAPH_BASED_VISUALIZATION,
            "Graph Based Visualization"
        },

        // Tree
        { 
            "bst_insert",       
            "BST Insert",       
            AlgorithmType::BST_INSERT,
            AlgorithmCategory::TREE,
            "Tree",          
            VisualizationType::TREE_BASED_VISUALIZATION,
            "Tree Based Visualization"
        },
        { 
            "bst_search",       
            "BST Search",       
            AlgorithmType::BST_SEARCH,  
            AlgorithmCategory::TREE,
            "Tree",          
            VisualizationType::TREE_BASED_VISUALIZATION,
            "Tree Based Visualization"
        },
        { 
            "avl_insert",       
            "AVL Insert",       
            AlgorithmType::AVL_INSERT,   
            AlgorithmCategory::TREE,
            "Tree",          
            VisualizationType::TREE_BASED_VISUALIZATION,
            "Tree Based Visualization"
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

        return "Unknown";
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

        return VisualizationType::ARRAY_BASED_VISUALIZATION;
    }

    inline std::string_view display_visualization(AlgorithmType type)
    {
        if (const auto* info = get_algorithm_info(type))
            return info->display_visualization;

        return "Unknown Visualization";;
    }

} // namespace c2l::algorithms

#endif // CODE2LOGIC_ALGORITHM_TYPES_HPP
