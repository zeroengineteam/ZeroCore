# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_executable(ZeroEditor "") 

target_include_directories(ZeroEditor
    PUBLIC 
        ${ParentDirectory}
        ${CurrentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(ZeroEditor
                      PUBLIC
                      Common
                      Gameplay
                      SpatialPartition
                      CodeTranslator
                      ZilchShaders
                      Serialization
                      Support
                      Engine 
                      Geometry 
                      Widget 
                      Editor 
                      ZilchScript 
                      Networking 
                      Meta 
                      Physics 
                      Sound 
                      Platform 
                      Content 
                      Dash 
                      UiWidget
                      Startup
                      Zilch
                      Graphics
                      SPIRV-Tools
                      SPIRV-Tools-opt
                      FreeType
                      Glew
                      Opus
)

zero_multitarget_output_directories(
                      ZeroEditor
                      LIBRARY_DIRECTORY ${zero_library_dir}
                      RUNTIME_DIRECTORY ${zero_binary_dir}
)

#set the correct subsystems for executable targets, and set stack size for the editor
set_target_properties(
    ZeroEditor 
    PROPERTIES 
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /STACK:8388608 /DEBUG ${common_linker_flags}"
)

zero_set_all_common_project_settings(
  Target ZeroEditor 
  Filter Projects 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)

set(SourceDir ${zero_core_path})
set(OutputDir ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${zero_core_path}/BuildInfoTemplate.data BuildInfo.data ESCAPE_QUOTES) 

zero_add_custom_files(Target ZeroEditor CustomFiles
    ${zero_platform_data}/ZeroEditor/ZeroEditor.rc
    ${zero_platform_data}/ZeroEditor/Resource.h
)
