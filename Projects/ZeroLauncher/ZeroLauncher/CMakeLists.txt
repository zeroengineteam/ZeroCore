################################################################################
# Generated using Joshua T. Fisher's 'CMake Builder'.
# Link: https://github.com/playmer/CmakeBuilder 
################################################################################
# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_executable(ZeroLauncher "")

target_include_directories(ZeroLauncher
    PUBLIC 
        ${ParentDirectory}
        ${CurrentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(ZeroLauncher
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
                      ZeroLauncherSharedLibrary
)

zero_multitarget_output_directories(
                      ZeroLauncher
                      LIBRARY_DIRECTORY ${zero_library_dir}
                      RUNTIME_DIRECTORY ${zero_binary_dir}
)

#set the correct subsystems for executable targets, and set stack size for the editor
set_target_properties(
    ZeroLauncher 
    PROPERTIES 
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /STACK:8388608 /DEBUG ${common_linker_flags}"
)

zero_set_all_common_project_settings(
  Target ZeroLauncher
  Filter Projects/ZeroLauncher
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)

set(platformSpecificSource)
# get the rc file so we get icons on windows builds
if (${platform} STREQUAL "Windows")
    set(platformSpecificSource
    ${zero_platform_data}/ZeroLauncher/ZeroLauncher.rc
    ${zero_platform_data}/ZeroLauncher/Resource.h
    )
endif()

target_sources(ZeroLauncher
    PRIVATE
    ${platformSpecificSource}
)

set(SourceDir ${zero_core_path})
set(OutputDir ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${zero_core_path}/BuildInfoTemplate.data BuildInfo.data ESCAPE_QUOTES) 

zero_add_custom_files(Target ZeroLauncher CustomFiles
    ${zero_platform_data}/ZeroLauncher/ZeroLauncher.rc
    ${zero_platform_data}/ZeroLauncher/Resource.h
)
