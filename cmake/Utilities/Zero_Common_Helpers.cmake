function(zero_set_common_compiler_definitions)
  set(oneValueArgs Target)
  cmake_parse_arguments(Parsed "" "${oneValueArgs}" "" ${ARGN})

  target_compile_options(  
            ${Parsed_Target}
            PRIVATE
             
            PUBLIC
                -GS -analyze-  -Zc:wchar_t
                
            PRIVATE
                -W3 -wd"4302"
                ${common_flags}
        )
endfunction()

function(zero_set_common_linker_flags  target)
  set_target_properties(
        ${target}
        PROPERTIES 
        STATIC_LIBRARY_FLAGS "${common_library_flags}"
        STATIC_LIBRARY_FLAGS_RELEASE "/LTCG"
    )
endfunction()

MACRO(ADD_MSVC_PRECOMPILED_HEADER)
  set(oneValueArgs Target PrecompiledHeader PrecompiledSource TargetSubfolder)
  cmake_parse_arguments(Parsed "" "${oneValueArgs}" "" ${ARGN})
  
  if("${Parsed_TargetSubfolder}" STREQUAL "")
    set(FullPrecompiledSourceName "${Parsed_PrecompiledSource}")
  else()
    set(FullPrecompiledSourceName "${Parsed_TargetSubfolder}/${Parsed_PrecompiledSource}")
  endif()
  
  set_target_properties(${Parsed_Target} PROPERTIES COMPILE_FLAGS "/Yu\"${Parsed_PrecompiledHeader}\"")
  set_source_files_properties("${FullPrecompiledSourceName}" PROPERTIES COMPILE_FLAGS "/Yc\"${Parsed_PrecompiledHeader}\"")
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

function(zero_set_precompiled_headers)
  set(oneValueArgs Target Filter PrecompiledHeader PrecompiledSource TargetSubfolder)
  cmake_parse_arguments(Parsed "" "${oneValueArgs}" "" ${ARGN})
  ADD_MSVC_PRECOMPILED_HEADER(Target ${Parsed_Target} PrecompiledHeader ${Parsed_PrecompiledHeader} PrecompiledSource ${Parsed_PrecompiledSource})
endfunction()

function(zero_source_ignore_precompiled_header source)
  set_source_files_properties(${source} PROPERTIES COMPILE_FLAGS "/Y-")
endfunction()

function(zero_set_common_output_settings)
  set(oneValueArgs Target PrecompiledHeader PrecompiledSource TargetSubfolder)
  cmake_parse_arguments(Parsed "" "${oneValueArgs}" "" ${ARGN})

  zero_multitarget_output_settings(
        ${Parsed_Target}
        CONFIGS ${supported_configs}
        BASEPATH ${zero_build_out}
        PLATFORM ${platform}
        CONFIG ${configuration}
        BITS ${bit}
        TOOLSET ${CMAKE_VS_PLATFORM_TOOLSET}
        TARGET_SUBFOLDER ${Parsed_TargetSubfolder}
    )
    ADD_MSVC_PRECOMPILED_HEADER(Target ${Parsed_Target} PrecompiledHeader ${Parsed_PrecompiledHeader} PrecompiledSource ${Parsed_PrecompiledSource} TargetSubfolder ${Parsed_TargetSubfolder})
    
endfunction()

function(zero_set_common_output_directories)
  zero_multitarget_output_directories(
      ${target}
      LIBRARY_DIRECTORY ${zero_library_dir}
      RUNTIME_DIRECTORY ${zero_binary_dir}
  )
endfunction()

function(zero_set_all_common_project_settings)
  set(oneValueArgs Target Filter PrecompiledHeader PrecompiledSource TargetSubfolder)
  cmake_parse_arguments(Parsed "" "${oneValueArgs}" "" ${ARGN})
  
  zero_set_common_compiler_definitions(Target ${Parsed_Target})
  zero_set_common_linker_flags(${Parsed_Target})
  zero_set_common_output_settings(Target ${Parsed_Target} PrecompiledHeader ${Parsed_PrecompiledHeader} PrecompiledSource ${Parsed_PrecompiledSource} TargetSubfolder ${Parsed_TargetSubfolder})
  zero_set_common_output_directories(${Parsed_Target})

  zero_subfolder_source_group(${zero_core_path} ${Parsed_Filter} ${Parsed_Target} "")
  
  set_target_properties(${Parsed_Target} PROPERTIES FOLDER "${Parsed_Filter}")
endfunction()

function(zero_set_all_common_external_project_settings)
  set(oneValueArgs Target Filter PrecompiledHeader PrecompiledSource TargetSubfolder)
  set(multiValueArgs IgnorePrecompiledHeader CONFIGURATIONS)
  cmake_parse_arguments(Parsed "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  
  zero_set_common_compiler_definitions(Target ${Parsed_Target})
  zero_set_common_linker_flags(${Parsed_Target})
  #zero_set_common_output_settings(Target ${Parsed_Target} PrecompiledHeader ${Parsed_PrecompiledHeader} PrecompiledSource ${Parsed_PrecompiledSource} TargetSubfolder ${Parsed_TargetSubfolder})
  #zero_set_common_output_directories(${Parsed_Target})
  
  
  if(DEFINED Parsed_PrecompiledHeader)
    zero_set_precompiled_headers(
      Target ${Parsed_Target} 
      Filter ${Parsed_Filter} 
      PrecompiledHeader ${Parsed_PrecompiledHeader}
      PrecompiledSource  ${Parsed_PrecompiledSource}
    )
    add_definitions(/FI${Parsed_PrecompiledHeader})
  endif()

  foreach(ignorePrecompiledHeader ${Parsed_IgnorePrecompiledHeader})
    zero_source_ignore_precompiled_header(${ignorePrecompiledHeader})
  endforeach()

  add_definitions(/W0 /wd4267 /WX-)
  set_target_properties(${Parsed_Target} PROPERTIES FOLDER "${Parsed_Filter}")

endfunction()

function(zero_add_visualizer_files)
  set(oneValueArgs Target)
  set(multiValueArgs VisualizerFiles)
  cmake_parse_arguments(Parsed "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio*")
    foreach(visualizerFile ${Parsed_VisualizerFiles})
        target_sources(${Parsed_Target} PRIVATE ${visualizerFile})
    endforeach()
  endif()
endfunction()


function(zero_add_custom_files)
  set(oneValueArgs Target)
  set(multiValueArgs CustomFiles)
  cmake_parse_arguments(Parsed "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio*")
    foreach(customFile ${Parsed_CustomFiles})
        target_sources(${Parsed_Target} PRIVATE ${customFile})
    endforeach()
  endif()
endfunction()