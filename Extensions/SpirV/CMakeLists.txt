################################################################################
# Author: Joshua Davis
# Copyright 2018, DigiPen Institute of Technology
################################################################################
add_subdirectory(SPIRV-Cross/ZeroCMake)
if(generate_spirv_projects)
  add_subdirectory(SpirVFiles/ZeroCMake)
endif()
