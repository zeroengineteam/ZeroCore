# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(Physics "") 

target_include_directories(Physics
    PUBLIC 
        ${ParentDirectory}
        ${CurrentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(Physics
                      PUBLIC
                      Common
                      Meta
                      Serialization
                      Geometry
                      SpatialPartition

)

zero_set_all_common_project_settings(
  Target Physics 
  Filter Systems 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)
