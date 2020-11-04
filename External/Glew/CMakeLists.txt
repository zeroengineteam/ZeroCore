add_library(Glew)

PROJECT(Glew C CXX)

set(GlewDir ${CMAKE_CURRENT_LIST_DIR}/Repo)

zero_set_all_common_external_project_settings(
  Target Glew 
  Filter External 
)

target_compile_definitions(Glew
  PUBLIC
    GLEW_STATIC
)

target_sources(Glew
  PRIVATE
    ${GlewDir}/src/glew.c
)

target_include_directories(Glew
  PUBLIC
    ${GlewDir}/include/
)
