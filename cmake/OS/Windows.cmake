################################################################################
# Author: Joshua Shlemmer
# Copyright 2017, DigiPen Institute of Technology
# Set Windows platform definitions
################################################################################

#define ZERO_PLATFORM_WINDOWS 1
#define PLATFORM_HARDWARE 1
#define PLATFORM_NAME "Win"
add_definitions(-DZERO_PLATFORM_WINDOWS=1 -DPLATFORM_HARDWARE=1 -DPLATFORM_NAME="Win" -D_UNICODE -DUNICODE)

set(platform "Windows")
set(platform_library "Windows")

remove_definitions(-DMBCS -D_MBCS)
