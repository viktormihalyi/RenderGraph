enable_testing ()

set (Sources
    Sources/Camera.cpp
    Sources/Frustum.cpp
    Sources/TestEnvironment.cpp
    Sources/TestEnvironment.hpp
    Sources/TestMain.cpp

    Sources/LCGTest.cpp
    Sources/RenderGraphPassTest.cpp
    Sources/RenderGraphAbstractionTest.cpp
    Sources/RenderGraphTests.cpp
    Sources/VizHFTests.cpp
    Sources/FontRenderingTests.cpp

    Sources/LogInitializer.cpp
)

add_executable (RenderGraphTest ${Sources})

target_precompile_headers (RenderGraphTest PRIVATE PrecompiledHeader.hpp)

set (BUILD_GMOCK OFF CACHE BOOL "" FORCE)

FetchContent_Declare (
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        e2239ee6043f73722e7aa812a459f54a28552929 # release-1.11.0
)

FetchContent_MakeAvailable (googletest)

target_include_directories (RenderGraphTest
    PRIVATE
        Sources

        $<TARGET_PROPERTY:RenderGraph,INTERFACE_INCLUDE_DIRECTORIES>

        $<TARGET_PROPERTY:gtest,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries (RenderGraphTest
    PUBLIC
        RenderGraph
        gtest
)

if (MSVC)
target_compile_options (RenderGraphTest PRIVATE /W4 /WX)
target_compile_options (RenderGraphTest
    PRIVATE
        /wd4099     # PDB 'filename' was not found with 'object/library' or at 'path'; linking object as if no debug info
        /wd4251     # 'type' : class 'type1' needs to have dll-interface to be used by clients of class 'type2'
        /wd4275     # non - DLL-interface class 'class_1' used as base for DLL-interface class 'class_2'
        /wd26812    # The enum type type-name is unscoped. Prefer 'enum class' over 'enum' (Enum.3)
)
endif ()

set_target_properties (RenderGraphTest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_test (NAME RenderGraphTest
          COMMAND RenderGraphTest
          WORKING_DIRECTORY bin)

add_custom_target (RenderGraphTest_CopyTestFiles ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Sources/TestData
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/TestData
)

