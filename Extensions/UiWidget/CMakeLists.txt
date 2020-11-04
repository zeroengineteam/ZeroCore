# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(UiWidget "")

target_include_directories(UiWidget
    PUBLIC
        ${ParentDirectory}
        ${CurrentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(UiWidget
                      PUBLIC
                      Engine
)

zero_set_all_common_project_settings(
  Target UiWidget 
  Filter Extensions 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)
