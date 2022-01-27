cmake_minimum_required (VERSION 3.16)

include (FetchContent)
set (FETCHCONTENT_FULLY_DISCONNECTED OFF CACHE BOOL "" FORCE)
set (FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)
set (GIT_SHALLOW ON CACHE BOOL "" FORCE)

project (RenderGraph)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED True)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/lib>)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/lib>)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/bin>)

if (MSVC)
    if (CMAKE_UNITY_BUILD)
        add_compile_options (/bigobj)   # increase number of sections in .obj file
    endif ()
    add_compile_options (/MP)           # build with multiple processes

    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")           # .pdb was not found with .lib
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4099")     # .pdb was not found with .lib
    set (CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4099")     # .pdb was not found with .lib
else ()
    add_compile_options (-fPIC)           # build with multiple processes
endif ()


# ========== MODULES ==========

add_subdirectory (src/RenderGraph)
add_subdirectory (src/RenderGraphTest)
