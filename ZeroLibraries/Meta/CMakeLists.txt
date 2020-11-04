# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(Meta "") 

target_include_directories(Meta
    PUBLIC 
        ${CurrentDirectory}
        ${ParentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(Meta
                      PUBLIC
                      Common
                      Zilch
                      Platform
)

zero_set_all_common_project_settings(
  Target Meta 
  Filter ZeroLibraries 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)
