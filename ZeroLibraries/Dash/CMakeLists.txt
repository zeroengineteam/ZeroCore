################################################################################
# Generated using Joshua T. Fisher's 'CMake Builder'.
# Link: https://github.com/playmer/CmakeBuilder 
################################################################################
# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(Dash "")

target_include_directories(Dash
    PUBLIC 
        ${CurrentDirectory}
        ${ParentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(Dash
                      PUBLIC
                      Common
                      Zilch                      
                      Geometry
)

zero_set_all_common_project_settings(
  Target Dash 
  Filter ZeroLibraries 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)
