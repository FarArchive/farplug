CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(wmexplorer)

# useful values
SET(src ${CMAKE_CURRENT_SOURCE_DIR})
SET(bin ${CMAKE_CURRENT_BINARY_DIR})
SET(top ${src}/..)

INCLUDE(${top}/cmake/common.cmake)

IF(DEFINED MSVC)
  SET(CFG_OPT_TYPE "size")
  INCLUDE(${top}/cmake/MSVC.cmake)
ENDIF(DEFINED MSVC)

# find ActiveSync SDK
FIND_PATH(rapi2_inc "rapi2.h" ${src}/rapi)
IF(NOT rapi2_inc)
  MESSAGE(FATAL_ERROR "rapi2.h not found")
ENDIF(NOT rapi2_inc)

# project source files
SET(cpp_sources plugin.cpp rapi_proc.cpp util.cpp ui.cpp dlgapi.cpp options.cpp filepath.cpp file_filters.cpp file_info.cpp)
add_precomp_header("headers.hpp" "headers.cpp" cpp_sources)
SET(sources ${cpp_sources} ${bin}/version.rc ${bin}/msg.h)
IF(FARAPI18)
  SET(sources ${sources} plugin_uni.def)
ELSE(FARAPI18)
  SET(sources ${sources} plugin.def)
ENDIF(FARAPI18)

# configure project
ADD_DEFINITIONS(-D__A_IDXSZ_TYPE__=unsigned -D_FAR_USE_FARFINDDATA -D_CRT_SECURE_NO_WARNINGS -D_CRT_NON_CONFORMING_SWPRINTFS -D_WIN32_DCOM -D_WIN32_WINNT=0x0501)
IF(FARAPI18)
  ADD_DEFINITIONS(-DFARAPI18)
ELSE(FARAPI18)
  ADD_DEFINITIONS(-DFARAPI17)
ENDIF(FARAPI18)
INCLUDE_DIRECTORIES(${src} ${top} ${far_inc} ${rapi2_inc} ${bin})
ADD_LIBRARY(${PROJECT_NAME} SHARED ${sources})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} version)
ADD_CUSTOM_TARGET(lng_hlf DEPENDS ${bin}/${PROJECT_NAME}_en.lng ${bin}/${PROJECT_NAME}_ru.lng ${bin}/${PROJECT_NAME}_en.hlf ${bin}/${PROJECT_NAME}_ru.hlf VERBATIM)
ADD_DEPENDENCIES(${PROJECT_NAME} lng_hlf)
ADD_DEPENDENCIES(lng_hlf check_rev)

# generate language files
ADD_CUSTOM_COMMAND(
  OUTPUT ${bin}/msg.h ${bin}/en.lng ${bin}/ru.lng
  COMMAND ${msgc} -in ${src}/en.msg ${src}/ru.msg -out ${bin}/msg.h ${bin}/en.lng ${bin}/ru.lng
  DEPENDS ${src}/en.msg ${src}/ru.msg
  VERBATIM)

enc_file(${bin}/en.lng ${bin}/${PROJECT_NAME}_en.lng us-ascii)
enc_file(${bin}/ru.lng ${bin}/${PROJECT_NAME}_ru.lng cp866)

m4_process(${src}/en.hlf.m4 ${bin}/${PROJECT_NAME}_en.hlf.1)
enc_file(${bin}/${PROJECT_NAME}_en.hlf.1 ${bin}/${PROJECT_NAME}_en.hlf us-ascii)
m4_process(${src}/ru.hlf.m4 ${bin}/${PROJECT_NAME}_ru.hlf.1)
enc_file(${bin}/${PROJECT_NAME}_ru.hlf.1 ${bin}/${PROJECT_NAME}_ru.hlf cp866)

enc_file(${src}/history_en.txt ${bin}/history_en.txt us-ascii)
enc_file(${src}/history_ru.txt ${bin}/history_ru.txt cp866)

m4_process(${src}/file_id.diz.m4 ${bin}/file_id.diz.1)
enc_file(${bin}/file_id.diz.1 ${bin}/file_id.diz cp866)

gen_ver()
m4_process(${src}/version.rc.m4 ${bin}/version.rc)

gen_distrib(${bin}/${PROJECT_NAME}_en.lng ${bin}/${PROJECT_NAME}_ru.lng ${bin}/${PROJECT_NAME}_en.hlf ${bin}/${PROJECT_NAME}_ru.hlf ${bin}/history_en.txt ${bin}/history_ru.txt ${bin}/file_id.diz)
