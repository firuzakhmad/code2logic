# Cross-platform compiler warnings
function(set_target_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
        target_compile_definitions(${target_name} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target_name} PRIVATE 
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wconversion
            -Wsign-conversion
            -Wdouble-promotion
            -Wformat=2
            -Wnull-dereference
        )
        
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target_name} PRIVATE -Wmisleading-indentation)
        endif()
        
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(${target_name} PRIVATE -Weverything -Wno-c++98-compat)
        endif()
    endif()
endfunction()