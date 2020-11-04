# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(Scintilla "") 

target_include_directories(Scintilla
    PUBLIC 
        ${ParentDirectory}
        ${CurrentDirectory}
        ${CurrentDirectory}/src
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(Scintilla
                      PUBLIC
                      Engine
)

zero_set_all_common_project_settings(
  Target Scintilla 
  Filter Extensions 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
  TargetSubfolder "src"
)
