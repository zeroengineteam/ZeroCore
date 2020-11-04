# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(Engine "") 

target_include_directories(Engine
    PUBLIC 
        ${ParentDirectory}
        ${CurrentDirectory}
        ${CurrentDirectory}/Action
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(Engine
                      PUBLIC
                      Common
                      Zilch
                      Platform
                      Meta
                      Serialization
                      Geometry
                      SpatialPartition
)

zero_set_all_common_project_settings(
  Target Engine 
  Filter Systems 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)
