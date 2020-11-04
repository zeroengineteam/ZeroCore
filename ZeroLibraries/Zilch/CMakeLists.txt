################################################################################
# Generated using Joshua T. Fisher's 'CMake Builder'.
# Link: https://github.com/playmer/CmakeBuilder 
################################################################################
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(Zilch "") 

target_include_directories(Zilch
    PUBLIC 
        ${CurrentDirectory}
        ${CurrentDirectory}/Project
        ${CurrentDirectory}/Project/Zilch
        ${ParentDirectory}
)        

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(Zilch
                      PUBLIC
                      Common
)

zero_set_all_common_project_settings(
  Target Zilch
  Filter ZeroLibraries
  PrecompiledHeader "Zilch.hpp"
  PrecompiledSource "Zilch.cpp"
  TargetSubfolder "Project/Zilch"
)
