add_library(Libpng)

PROJECT(Libpng C CXX)

set(LibpngDir ${CMAKE_CURRENT_LIST_DIR}/Repo)

zero_set_all_common_external_project_settings(
  Target Libpng 
  Filter External 
)

target_link_libraries(Libpng
  PUBLIC
  Zlib2
)

target_include_directories(Libpng
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${LibpngDir}/
)

target_sources(Libpng
  PRIVATE
    ${LibpngDir}/png.c
    ${LibpngDir}/png.h
    ${LibpngDir}/pngconf.h
    ${LibpngDir}/pngdebug.h
    ${LibpngDir}/pngerror.c
    ${LibpngDir}/pngget.c
    ${LibpngDir}/pnginfo.h
    ${LibpngDir}/pngmem.c
    ${LibpngDir}/pngpread.c
    ${LibpngDir}/pngpriv.h
    ${LibpngDir}/pngread.c
    ${LibpngDir}/pngrio.c
    ${LibpngDir}/pngrtran.c
    ${LibpngDir}/pngrutil.c
    ${LibpngDir}/pngset.c
    ${LibpngDir}/pngstruct.h
    ${LibpngDir}/pngtrans.c
    ${LibpngDir}/pngwio.c
    ${LibpngDir}/pngwrite.c
    ${LibpngDir}/pngwtran.c
    ${LibpngDir}/pngwutil.c
)
