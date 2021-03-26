# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})

add_library(ZeroLauncherSharedLibrary SHARED "")

target_include_directories(ZeroLauncherSharedLibrary
    PUBLIC 
        ${ParentDirectory}
        ${CurrentDirectory}
)

include(${CMAKE_CURRENT_LIST_DIR}/Generated.cmake)

target_link_libraries(ZeroLauncherSharedLibrary
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
                      ZeroLauncherSharedLibrary
                      LIBRARY_DIRECTORY ${zero_library_dir}
                      RUNTIME_DIRECTORY ${zero_binary_dir}
)

#set the correct subsystems for executable targets, and set stack size for the editor
set_target_properties(
    ZeroLauncherSharedLibrary 
    PROPERTIES 
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /STACK:8388608 /DEBUG ${common_linker_flags}"
)

zero_set_all_common_project_settings(
  Target ZeroLauncherSharedLibrary 
  Filter Projects/ZeroLauncher 
  PrecompiledHeader "Precompiled.hpp"
  PrecompiledSource "Precompiled.cpp"
)

target_compile_options(
    ZeroLauncherSharedLibrary
    PRIVATE
         
    PUBLIC
    PRIVATE
    ${common_flags}
    "-D \"_UNICODE\"" 
    "-D \"UNICODE\""
    "-D \"CURL_STATICLIB\"" 
    "-D \"WIN32\"" 
    "-D \"_WINDOWS\"" 
    "-D \"_USRDLL\"" 
    "-D \"ZeroLauncherSharedLibrary_EXPORTS\"" 
    "-D \"_WINDLL\"" 
    "-D \"_CRT_SECURE_NO_WARNINGS\"" 
    "-D \"ZeroExportDll\"" 
)

 add_custom_command(
    TARGET ZeroLauncherSharedLibrary POST_BUILD
    # executes "cmake -E copy_if_different
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    # input file
    ${zero_binary_dir}/ZeroLauncherSharedLibrary/ZeroLauncherSharedLibrary.dll
    #output file
    ${zero_binary_dir}/ZeroLauncher/ZeroLauncherSharedLibrary.dll
)
add_custom_command(
    TARGET ZeroLauncherSharedLibrary POST_BUILD
    # executes "cmake -E copy_if_different
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    # input file
    ${zero_binary_dir}/ZeroLauncherSharedLibrary/ZeroLauncherSharedLibrary.pdb
    #output file
    ${zero_binary_dir}/ZeroLauncher/ZeroLauncherSharedLibrary.pdb
)
