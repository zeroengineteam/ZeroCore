add_library(Zlib2)

PROJECT(Zlib2 C CXX)

set(Zlib2Dir ${CMAKE_CURRENT_LIST_DIR}/zlib-1.2.11/)

zero_set_all_common_external_project_settings(
  Target Zlib2 
  Filter External
)

target_include_directories(Zlib2
  PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
  ${Zlib2Dir}/
)

target_sources(Zlib2
  PRIVATE
    ${Zlib2Dir}/adler32.c
    ${Zlib2Dir}/compress.c
    ${Zlib2Dir}/crc32.c
    ${Zlib2Dir}/crc32.h
    ${Zlib2Dir}/deflate.c
    ${Zlib2Dir}/deflate.h
    ${Zlib2Dir}/gzclose.c
    ${Zlib2Dir}/gzguts.h
    ${Zlib2Dir}/gzlib.c
    ${Zlib2Dir}/gzread.c
    ${Zlib2Dir}/gzwrite.c
    ${Zlib2Dir}/infback.c
    ${Zlib2Dir}/inffast.c
    ${Zlib2Dir}/inffast.h
    ${Zlib2Dir}/inffixed.h
    ${Zlib2Dir}/inflate.c
    ${Zlib2Dir}/inflate.h
    ${Zlib2Dir}/inftrees.c
    ${Zlib2Dir}/inftrees.h
    ${Zlib2Dir}/trees.c
    ${Zlib2Dir}/trees.h
    ${Zlib2Dir}/uncompr.c
    ${Zlib2Dir}/zconf.h
    ${Zlib2Dir}/zlib.h
    ${Zlib2Dir}/zutil.c
    ${Zlib2Dir}/zutil.h
)
