# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(ZilchScript "")

target_include_directories(ZilchScript
    PUBLIC 
        ${CurrentDirectory}
        ${ParentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(ZilchScript
                      PUBLIC
                      Common
                      Zilch
                      Meta
                      Serialization
                      Editor
)

zero_set_all_common_project_settings(
  Target ZilchScript 
  Filter Systems 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)
