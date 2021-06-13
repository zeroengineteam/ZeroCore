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


set(ZeroMajorVersion 0)
set(ZeroMinorVersion 0)
set(ZeroPatchVersion 0)
set(ZeroRevisionId 0)
set(ZeroShortChangeSet 0)
set(ZeroChangeSet 0)
set(ZeroChangeSetDate 0)

configure_file(${zero_core_path}/BuildVersionTemplate.data ${CurrentDirectory}/BuildVersion.inl ESCAPE_QUOTES) 
