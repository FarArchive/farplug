CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ntfsfile)

# useful values
SET(src ${CMAKE_CURRENT_SOURCE_DIR})
SET(bin ${CMAKE_CURRENT_BINARY_DIR})
SET(top ${src}/..)

INCLUDE(${top}/cmake/common.cmake)

IF(DEFINED MSVC)
  SET(CFG_THREADS 1)
  INCLUDE(${top}/cmake/MSVC.cmake)
ENDIF(DEFINED MSVC)

# project source files
SET(cpp_sources main.cpp content.cpp file_panel.cpp ntfs_file.cpp options.cpp utils.cpp volume.cpp dlgapi.cpp defragment.cpp mftindex.cpp filever.cpp compress_files.cpp)
add_precomp_header("headers.hpp" "headers.cpp" cpp_sources)
SET(sources ${cpp_sources} ${bin}/version.rc ${bin}/msg.h)
IF(FARAPI18)
  SET(sources ${sources} plugin_uni.def)
ELSE(FARAPI18)
  SET(sources ${sources} plugin.def)
ENDIF(FARAPI18)

# configure project
ADD_DEFINITIONS(-D__A_IDXSZ_TYPE__=unsigned -D_FAR_USE_FARFINDDATA -D_WIN32_WINNT=0x0500 -D_CRT_SECURE_NO_WARNINGS -D_CRT_NON_CONFORMING_SWPRINTFS)
IF(FARAPI18)
  ADD_DEFINITIONS(-DFARAPI18)
ELSE(FARAPI18)
  ADD_DEFINITIONS(-DFARAPI17)
ENDIF(FARAPI18)
INCLUDE_DIRECTORIES(${src} ${top} ${far_inc} ${src}/lzo/include ${src}/openssl/include ${bin})
LINK_DIRECTORIES(${src}/lzo/lib ${src}/openssl/lib)
ADD_LIBRARY(${PROJECT_NAME} SHARED ${sources})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} lzo2_${suffix} libeay${suffix} mpr version imagehlp)
ADD_CUSTOM_TARGET(lng_hlf DEPENDS ${bin}/en.hlf ${bin}/ru.hlf ${bin}/en.lng VERBATIM)
ADD_DEPENDENCIES(${PROJECT_NAME} lng_hlf)
ADD_DEPENDENCIES(lng_hlf check_rev)

# generate language files
ADD_CUSTOM_COMMAND(
  OUTPUT ${bin}/msg.h ${bin}/en.lng.1
  COMMAND ${msgc} -in ${src}/en.msg -out ${bin}/msg.h ${bin}/en.lng.1
  DEPENDS ${src}/en.msg
  VERBATIM)

enc_file(${bin}/en.lng.1 ${bin}/en.lng us-ascii)

m4_process(${src}/en.hlf.m4 ${bin}/en.hlf.1)
enc_file(${bin}/en.hlf.1 ${bin}/en.hlf us-ascii)
m4_process(${src}/ru.hlf.m4 ${bin}/ru.hlf.1)
enc_file(${bin}/ru.hlf.1 ${bin}/ru.hlf cp866)

enc_file(${src}/history_ru.txt ${bin}/history_ru.txt cp866)

m4_process(${src}/file_id.diz.m4 ${bin}/file_id.diz.1)
enc_file(${bin}/file_id.diz.1 ${bin}/file_id.diz cp866)

gen_ver()
m4_process(${src}/version.rc.m4 ${bin}/version.rc)

gen_distrib(${bin}/history_ru.txt ${bin}/file_id.diz ${bin}/en.hlf ${bin}/ru.hlf ${bin}/en.lng)

MACRO(gen_installer)
  FILE(READ ${src}/version version)
  SET(distr_suffix "${version}")
  IF(FARAPI18)
    SET(distr_suffix "${distr_suffix}_uni")
  ENDIF(FARAPI18)
  SET(platform "x86")
  IF(CMAKE_CL_64)
    SET(distr_suffix "${distr_suffix}_x64")
    SET(platform "x64")
  ENDIF(CMAKE_CL_64)
  IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    SET(distr_suffix "${distr_suffix}_dbg")
  ENDIF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  SET(distrib_package "${bin}/${PROJECT_NAME}_${distr_suffix}")
  GET_TARGET_PROPERTY(library_path ${PROJECT_NAME} LOCATION)
  SET(distrib_files ${library_path} ${bin}/${PROJECT_NAME}.map ${ARGV})
  ADD_CUSTOM_TARGET(installer
    COMMAND ${CMAKE_COMMAND} -E remove ${distrib_package}.msi
    COMMAND nmake -f ${src}/installer/makefile DISTRIB=${distrib_package} INSTALLER=${src}/installer OUTDIR=${bin} PLATFORM=${platform} VERSION=${version} FAR_VERSION="2.0.1447"
    DEPENDS ${PROJECT_NAME} ${distrib_files}
    VERBATIM)
  ADD_DEPENDENCIES(installer check_rev)
ENDMACRO(gen_installer)

gen_installer(${bin}/history_ru.txt ${bin}/file_id.diz ${bin}/en.hlf ${bin}/ru.hlf ${bin}/en.lng)
