# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(SpatialPartition "")

target_include_directories(SpatialPartition
    PUBLIC 
        ${CurrentDirectory}
        ${ParentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(SpatialPartition
                      PUBLIC
                      Common
                      Serialization
)

zero_set_all_common_project_settings(
  Target SpatialPartition 
  Filter ZeroLibraries 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)
