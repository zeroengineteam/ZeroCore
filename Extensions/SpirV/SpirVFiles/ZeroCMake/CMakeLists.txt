################################################################################
# Author: Joshua Davis
# Copyright 2018, DigiPen Institute of Technology
################################################################################
# get our parent directory
get_filename_component(ParentDirectory ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)

# get our current directory
set(CurrentDirectory ${CMAKE_CURRENT_LIST_DIR})
set(SPIRV-Tools-Dir ${CMAKE_CURRENT_LIST_DIR}/..)

if(use_spirv_shared_library)
  set(SpirVToolsCompileOptions
        -DSPIRV_TOOLS_IMPLEMENTATION
        -DSPIRV_TOOLS_SHAREDLIB
  )
endif()

################################################################################
# define all of the libraries that are in this folder
################################################################################
add_library(SPIRV-Tools "")

if (use_spirv_shared_library)
  add_library(SPIRV-Tools-opt SHARED "")
else ()
  add_library(SPIRV-Tools-opt "")
endif ()
  
################################################################################
# Explicitly define the folders as they are not organized how we want
################################################################################
set_target_properties(SPIRV-Tools PROPERTIES FOLDER "Extensions")
set_target_properties(SPIRV-Tools-opt PROPERTIES FOLDER "Extensions")

################################################################################
# include filelists for each library
################################################################################
include(SPIRV-Tools.cmake)
include(SPIRV-Tools-opt.cmake)

################################################################################
# define include directories for all of our libraries
################################################################################
target_include_directories(SPIRV-Tools 
    PUBLIC 
        ${SPIRV-Tools-Dir}/generated
        ${SPIRV-Tools-Dir}/external/SPIRV-Headers
        ${SPIRV-Tools-Dir}/external/SPIRV-Headers/include
        ${SPIRV-Tools-Dir}/external/SPIRV-Headers/include/spirv
        ${SPIRV-Tools-Dir}/include
        ${SPIRV-Tools-Dir}/include/spirv-tools
        ${SPIRV-Tools-Dir}/source
        ${SPIRV-Tools-Dir}/source/comp
        ${SPIRV-Tools-Dir}/source/link
        ${SPIRV-Tools-Dir}/source/opt
        ${SPIRV-Tools-Dir}/source/util
        ${SPIRV-Tools-Dir}/source/val
        ${SPIRV-Tools-Dir}/tools
        ${SPIRV-Tools-Dir}/tools/as
        ${SPIRV-Tools-Dir}/tools/cfg
        ${SPIRV-Tools-Dir}/tools/comp
        ${SPIRV-Tools-Dir}/tools/dis
        ${SPIRV-Tools-Dir}/tools/link
        ${SPIRV-Tools-Dir}/tools/opt
        ${SPIRV-Tools-Dir}/tools/stats
        ${SPIRV-Tools-Dir}/tools/val
        ${SPIRV-Tools-Dir}

)
target_include_directories(SPIRV-Tools-opt
    PUBLIC 
        ${SPIRV-Tools-Dir}/generated
        ${SPIRV-Tools-Dir}/external/SPIRV-Headers
        ${SPIRV-Tools-Dir}/external/SPIRV-Headers/include
        ${SPIRV-Tools-Dir}/external/SPIRV-Headers/include/spirv
        ${SPIRV-Tools-Dir}/include
        ${SPIRV-Tools-Dir}/include/spirv-tools
        ${SPIRV-Tools-Dir}/source
        ${SPIRV-Tools-Dir}/source/comp
        ${SPIRV-Tools-Dir}/source/link
        ${SPIRV-Tools-Dir}/source/opt
        ${SPIRV-Tools-Dir}/source/util
        ${SPIRV-Tools-Dir}/source/val
        ${SPIRV-Tools-Dir}/tools
        ${SPIRV-Tools-Dir}/tools/as
        ${SPIRV-Tools-Dir}/tools/cfg
        ${SPIRV-Tools-Dir}/tools/comp
        ${SPIRV-Tools-Dir}/tools/dis
        ${SPIRV-Tools-Dir}/tools/link
        ${SPIRV-Tools-Dir}/tools/opt
        ${SPIRV-Tools-Dir}/tools/stats
        ${SPIRV-Tools-Dir}/tools/val
        ${SPIRV-Tools-Dir}
)

################################################################################
# set the linked libraries for all of our targets
################################################################################
target_link_libraries(SPIRV-Tools
                      PUBLIC
)
target_link_libraries(SPIRV-Tools-opt
                      PUBLIC
                      SPIRV-Tools
)

################################################################################
# set the output directories for all of our targets
################################################################################
zero_multitarget_output_directories(SPIRV-Tools
                      LIBRARY_DIRECTORY ${zero_library_dir}
                      RUNTIME_DIRECTORY ${zero_binary_dir}
)

zero_multitarget_output_directories(SPIRV-Tools-opt
                      LIBRARY_DIRECTORY ${zero_library_dir}
                      RUNTIME_DIRECTORY ${zero_binary_dir}
)

################################################################################
# Specify any additional target options such as pdb locations
################################################################################
if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC OR (CMAKE_GENERATOR_TOOLSET STREQUAL "LLVM-vs2014"))
    zero_multitarget_output_settings(
        SPIRV-Tools
        CONFIGS ${supported_configs}
        BASEPATH ${zero_build_out}
        PLATFORM ${platform}
        CONFIG ${configuration}
        BITS ${bit}
        TOOLSET ${CMAKE_VS_PLATFORM_TOOLSET}
        TARGET_SUBFOLDER
    )
    zero_multitarget_output_settings(
        SPIRV-Tools-opt
        CONFIGS ${supported_configs}
        BASEPATH ${zero_build_out}
        PLATFORM ${platform}
        CONFIG ${configuration}
        BITS ${bit}
        TOOLSET ${CMAKE_VS_PLATFORM_TOOLSET}
        TARGET_SUBFOLDER
    )
endif()

################################################################################
# set flags and definitions
################################################################################
if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC OR (CMAKE_GENERATOR_TOOLSET STREQUAL "LLVM-vs2014"))
    zero_multitarget_compile_options(  
        SPIRV-Tools
        PRIVATE 
        PUBLIC
            -GS -analyze-  -Zc:wchar_t
        PRIVATE
            -W3
            -wd"4302" 
            -wd"4305"
            -wd"4800"
            -Gd
            -EHsc
            -MP
            -GR-
            -fp:fast 
            -Zc:forScope 
            -Oy-
            -Od
            -Zc:inline
            -MDd
            -nologo
            -D_SCL_SECURE_NO_WARNINGS
            ${SpirVToolsCompileOptions}
            ${common_flags}
            -Gm-
    )
    zero_multitarget_compile_options(  
        SPIRV-Tools-opt
        PRIVATE 
        PUBLIC
            -GS -analyze-  -Zc:wchar_t
        PRIVATE
            -W3
            -wd"4302" 
            -wd"4305"
            -wd"4800"
            -Gd
            -EHsc
            -MP
            -GR-
            -fp:fast 
            -Zc:forScope 
            -Oy-
            -Od
            -Zc:inline
            -MDd
            -nologo
            -D_SCL_SECURE_NO_WARNINGS
            ${SpirVToolsCompileOptions}
            ${common_flags}
            -Gm-
    )
else()
    target_compile_options(
        SPIRV-Tools
        PRIVATE
        PUBLIC
        PRIVATE
        ${common_flags}
    )
    set_target_properties(
        SPIRV-Tools
        PROPERTIES
        LINK_FLAGS ${common_linker_flags}
    )
endif()

################################################################################
# Set linker flags
################################################################################
if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC OR (CMAKE_GENERATOR_TOOLSET STREQUAL "LLVM-vs2014"))
    # set the correct subsystems for executable targets, and set stack size for the editor
    set_target_properties(
        SPIRV-Tools
        PROPERTIES 
        STATIC_LIBRARY_FLAGS "${common_library_flags}"
        STATIC_LIBRARY_FLAGS_RELEASE "/LTCG"
    )
    set_target_properties(
        SPIRV-Tools-opt
        PROPERTIES 
        STATIC_LIBRARY_FLAGS "${common_library_flags}"
        STATIC_LIBRARY_FLAGS_RELEASE "/LTCG"
    )
endif()


if (use_spirv_shared_library)
  add_custom_command(
    TARGET SPIRV-Tools-opt POST_BUILD
    # Copies spirv optimizer dll to the external folder location
    COMMAND ${CMAKE_COMMAND} -E copy   
    # input folder
    ${zero_binary_dir}/SPIRV-Tools-opt/SPIRV-Tools-opt.dll
    #output folder
    ${zero_core_path}/External/SpirVTools/bin/SPIRV-Tools-opt.dll
  )
  add_custom_command(
    TARGET SPIRV-Tools-opt POST_BUILD
    # Copies spirv optimizer lib to the external folder location
    COMMAND ${CMAKE_COMMAND} -E copy   
    # input folder
    ${zero_binary_dir}/SPIRV-Tools-opt/SPIRV-Tools-opt.lib
    #output folder
    ${zero_core_path}/External/SpirVTools/lib/${platform}/${bit}/SPIRV-Tools-opt.lib
  )
  add_custom_command(
    TARGET SPIRV-Tools-opt POST_BUILD
    # Copies spirv include files (required or static linking fails due to missing directory and how cmake was setup)
    COMMAND ${CMAKE_COMMAND} -E copy_directory   
    # input folder
    ${CurrentDirectory}/include
    #output folder
    ${zero_core_path}/External/SpirVTools/include
  )
endif()

################################################################################
# Group source into folders
################################################################################
#zero_subfolder_source_group(${zero_core_path} Extensions/SpirV/SpirVFiles SPIRV-Tools "")