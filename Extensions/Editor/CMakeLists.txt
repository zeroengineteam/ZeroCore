# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(Editor "") 

target_include_directories(Editor
    PUBLIC 
        ${ParentDirectory}
        ${CurrentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(Editor
                      PUBLIC
                      Engine
                      Widget
                      UiWidget
                      Gameplay
                      Sound
                      Geometry
)

zero_set_all_common_project_settings(
  Target Editor 
  Filter Extensions 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)
