# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_executable(ZilchShadersTests "") 

target_include_directories(ZilchShadersTests
    PUBLIC 
        ${ParentDirectory}
        ${CurrentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/CMakeFiles.cmake)

target_link_libraries(ZilchShadersTests
                      PUBLIC
                      Common
                      ZilchShaders
                      SPIRV-Cross
                      Serialization
                      Support
                      Geometry
                      Meta
                      Platform
                      Zilch
                      SPIRV-Cross
                      SPIRV-Tools
                      SPIRV-Tools-opt
                      Glew
)

zero_set_all_common_project_settings(
  Target ZilchShadersTests 
  Filter UnitTests 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)

# Zilch Shader unit tests need the working directory to be set to where the unit
# tests are so it can write output files (to be committed) in the same directory.
set_target_properties(ZilchShadersTests PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${zero_core_path}/UnitTests/ZilchShadersTests/")
