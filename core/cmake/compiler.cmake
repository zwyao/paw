###############################################################
# 该脚本用以控制编译器的行为。
#

# 定义编译器的行为
macro(evnet_compiler_and_linker)
    if (ndebug)
        add_definitions(-DNDEBUG)
        message(STATUS "–DNDEBUG")
    endif()

    if (NOT CONFIGURED_ONCE)
        SET(CMAKE_CXX_FLAGS "${warnings}"
            CACHE STRING "Flags used by the compiler during all build types." FORCE)
        SET(CMAKE_C_FLAGS   "${warnings}"
            CACHE STRING "Flags used by the compiler during all build types." FORCE)
    endif()
endmacro()
