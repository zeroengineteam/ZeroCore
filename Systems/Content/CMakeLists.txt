# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(Content "") 

target_include_directories(Content
    PUBLIC 
        ${ParentDirectory}
        ${CurrentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(Content
                      PUBLIC
                      Common
                      Zilch
                      Platform
                      Meta
                      Serialization
                      Assimp
                      Nvtt
                      Opus
)

zero_set_all_common_project_settings(
  Target Content 
  Filter Systems 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)

add_definitions(/WX-)
