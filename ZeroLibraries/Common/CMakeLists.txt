################################################################################
# Generated using Joshua T. Fisher's 'CMake Builder'.
# Link: https://github.com/playmer/CmakeBuilder 
################################################################################
# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(Common "") 

target_include_directories(Common
    PUBLIC 
        ${CurrentDirectory}
        ${ParentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

zero_add_visualizer_files(Target Common VisualizerFiles "${CMAKE_CURRENT_LIST_DIR}/ZeroVisualizers.natvis")

zero_set_all_common_project_settings(
  Target Common 
  Filter ZeroLibraries 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)
