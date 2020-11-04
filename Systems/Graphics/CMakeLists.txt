# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(Graphics "") 

target_include_directories(Graphics
    PUBLIC 
        ${ParentDirectory}
        ${CurrentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(Graphics
                      PUBLIC
                      Common
                      Zilch
                      Meta
                      Serialization
                      Geometry
                      SpatialPartition
                      ZilchShaders
                      FreeType
)

zero_set_all_common_project_settings(
  Target Graphics 
  Filter Systems 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)
