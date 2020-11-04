################################################################################
# Generated using Joshua T. Fisher's 'CMake Builder'.
# Link: https://github.com/playmer/CmakeBuilder 
################################################################################
#add_subdirectory(Project)
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(Platform "") 

target_include_directories(Platform
    PUBLIC 
        ${CurrentDirectory}
        ${CurrentDirectory}/Windows
)        

include(Windows/CMakeLists.txt)

target_link_libraries(Platform
                      PUBLIC
                      Common
                      GL
                      Glew
                      Curl
)

zero_set_all_common_project_settings(
  Target Platform
  Filter ZeroLibraries
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
  TargetSubfolder "Windows"
)
