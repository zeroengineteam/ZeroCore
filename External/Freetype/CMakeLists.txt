add_library(FreeType)

PROJECT(FreeType C CXX)

set(FreeTypeDir ${CMAKE_CURRENT_LIST_DIR}/Repo)

zero_set_all_common_external_project_settings(
  Target FreeType 
  Filter External 
)

target_compile_definitions(FreeType
  PUBLIC
    FT2_BUILD_LIBRARY
    FT_CONFIG_OPTION_SYSTEM_ZLIB
    FT_CONFIG_OPTION_USE_PNG
)

target_link_libraries(FreeType
  PUBLIC
  Zlib2
  Libpng
)

target_include_directories(FreeType
  PUBLIC
    ${FreeTypeDir}/include/
    ${FreeTypeDir}/include/freetype/
)
#set_target_properties(FreeType PROPERTIES FOLDER "External")

target_sources(FreeType
  PRIVATE
    ${FreeTypeDir}/include/ft2build.h
    ${FreeTypeDir}/src/autofit/autofit.c
    ${FreeTypeDir}/src/base/ftbase.c
    ${FreeTypeDir}/src/base/ftbbox.c
    ${FreeTypeDir}/src/base/ftbdf.c
    ${FreeTypeDir}/src/base/ftbitmap.c
    ${FreeTypeDir}/src/base/ftcid.c
    ${FreeTypeDir}/src/base/ftdebug.c
    ${FreeTypeDir}/src/base/ftfstype.c
    ${FreeTypeDir}/src/base/ftgasp.c
    ${FreeTypeDir}/src/base/ftglyph.c
    ${FreeTypeDir}/src/base/ftgxval.c
    ${FreeTypeDir}/src/base/ftinit.c
    ${FreeTypeDir}/src/base/ftmm.c
    ${FreeTypeDir}/src/base/ftotval.c
    ${FreeTypeDir}/src/base/ftpatent.c
    ${FreeTypeDir}/src/base/ftpfr.c
    ${FreeTypeDir}/src/base/ftstroke.c
    ${FreeTypeDir}/src/base/ftsynth.c
    ${FreeTypeDir}/src/base/ftsystem.c
    ${FreeTypeDir}/src/base/fttype1.c
    ${FreeTypeDir}/src/base/ftwinfnt.c
    ${FreeTypeDir}/src/bdf/bdf.c
    ${FreeTypeDir}/src/bzip2/ftbzip2.c
    ${FreeTypeDir}/src/cache/ftcache.c
    ${FreeTypeDir}/src/cff/cff.c
    ${FreeTypeDir}/src/cid/type1cid.c
    ${FreeTypeDir}/src/gzip/ftgzip.c
    ${FreeTypeDir}/src/lzw/ftlzw.c
    ${FreeTypeDir}/src/pcf/pcf.c
    ${FreeTypeDir}/src/pfr/pfr.c
    ${FreeTypeDir}/src/psaux/psaux.c
    ${FreeTypeDir}/src/pshinter/pshinter.c
    ${FreeTypeDir}/src/psnames/psnames.c
    ${FreeTypeDir}/src/raster/raster.c
    ${FreeTypeDir}/src/sfnt/sfnt.c
    ${FreeTypeDir}/src/smooth/smooth.c
    ${FreeTypeDir}/src/truetype/truetype.c
    ${FreeTypeDir}/src/type1/type1.c
    ${FreeTypeDir}/src/type42/type42.c
    ${FreeTypeDir}/src/winfonts/winfnt.c
)
