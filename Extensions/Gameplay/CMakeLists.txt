# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(Gameplay "") 

target_include_directories(Gameplay
    PUBLIC 
        ${ParentDirectory}
        ${CurrentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(Gameplay
                      PUBLIC
                      Common
                      Platform
                      Zilch
                      Geometry
                      Meta
                      Support
                      Engine
)

zero_set_all_common_project_settings(
  Target Gameplay 
  Filter Extensions 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)
