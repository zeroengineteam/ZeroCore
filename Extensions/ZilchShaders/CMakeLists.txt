# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(ZilchShaders "") 

target_include_directories(ZilchShaders
    PUBLIC 
        ${ParentDirectory}
        ${CurrentDirectory}
        
        ${SpirVDir}/SPIRV-Cross
        ${SpirVDir}/SpirVFiles
        ${SpirVDir}/SpirVFiles/external
        ${SpirVDir}/SpirVFiles/external/SPIRV-Headers/include
        ${SpirVDir}/SpirVFiles/include
        ${SpirVDir}/SpirVFiles/include/spirv-tools
        ${SpirVDir}/SpirVFiles/build
        ${SpirVDir}/SpirVFiles/tools
        ${SpirVDir}/SpirVFiles/source
)

include(${CMAKE_CURRENT_LIST_DIR}/CMakeFiles.cmake)

target_link_libraries(ZilchShaders
                      PUBLIC
                      Common
                      Zilch
                      Meta
                      Serialization
                      Geometry
                      SPIRV-Tools
                      SPIRV-Cross
)

zero_set_all_common_project_settings(
  Target ZilchShaders 
  Filter Extensions 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)
