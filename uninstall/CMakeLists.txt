CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(UnInstall)

# useful values
SET(src ${CMAKE_CURRENT_SOURCE_DIR})
SET(bin ${CMAKE_CURRENT_BINARY_DIR})
SET(top ${src}/..)

IF(DEFINED MSVC)
  SET(CFG_OPT_TYPE "size")
  SET(CFG_RUNTIME_TYPE "none")
  SET(CFG_ENTRY_NAME 0)
  INCLUDE(${top}/cmake/MSVC.cmake)
ENDIF(DEFINED MSVC)

SET(FARAPI18 FALSE CACHE BOOL "Select target Far API version")

MACRO(get_local_path file_pattern)
  SET(local_path "")
  FILE(GLOB_RECURSE file_list ${top}/${file_pattern})
  FOREACH(file_path ${file_list})
    GET_FILENAME_COMPONENT(file_dir ${file_path} PATH)
    LIST(APPEND local_path ${file_dir})
  ENDFOREACH(file_path)
ENDMACRO(get_local_path)

# find version info generator
get_local_path("svnrev.*")
FIND_PROGRAM(svnrev "svnrev" ${local_path})
IF(NOT svnrev)
  MESSAGE(FATAL_ERROR "svnrev.exe not found")
ENDIF(NOT svnrev)
FILE(TO_NATIVE_PATH ${svnrev} svnrev)

# FAR SDK include files
get_local_path("plugin.hpp")
FIND_PATH(far_inc NAMES plugin.hpp farcolor.hpp farkeys.hpp PATHS ${local_path})
IF(NOT far_inc)
  MESSAGE(FATAL_ERROR "plugin.hpp not found")
ENDIF(NOT far_inc)
IF(FARAPI18)
  SET(far_inc ${far_inc}/unicode)
ENDIF(FARAPI18)

# figure out library suffix
IF(CMAKE_CL_64)
  SET(suffix 64)
ELSE(CMAKE_CL_64)
  SET(suffix 32)
ENDIF(CMAKE_CL_64)
IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  SET(suffix ${suffix}d)
ENDIF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")

# project source files
SET(sources UnInstall.cpp ${bin}/UnInstall.rc)
IF(FARAPI18)
  SET(sources ${sources} UnInstallW.def)
ELSE(FARAPI18)
  SET(sources ${sources} UnInstall.def)
ENDIF(FARAPI18)

# configure project
ADD_DEFINITIONS(-D_FAR_USE_FARFINDDATA -D_CRT_SECURE_NO_WARNINGS)
IF(FARAPI18)
  ADD_DEFINITIONS(-DFARAPI18 -DUNICODE -D_UNICODE)
ELSE(FARAPI18)
  ADD_DEFINITIONS(-DFARAPI17)
ENDIF(FARAPI18)
INCLUDE_DIRECTORIES(${src} ${far_inc})
ADD_LIBRARY(${PROJECT_NAME} SHARED ${sources})
ADD_DEPENDENCIES(${PROJECT_NAME} check_rev)

MACRO(enc_file src dst src_enc)
  IF(FARAPI18)
    ADD_CUSTOM_COMMAND(OUTPUT ${dst} COMMAND iconv -f ${src_enc} -t utf-16 ${src} > ${dst} DEPENDS ${src} VERBATIM)
  ELSE(FARAPI18)
    ADD_CUSTOM_COMMAND(OUTPUT ${dst} COMMAND ${CMAKE_COMMAND} -E copy ${src} ${dst} DEPENDS ${src} VERBATIM)
  ENDIF(FARAPI18)
ENDMACRO(enc_file)

IF(FARAPI18)
  SET(m4_def -D __FARAPI18__)
ELSE(FARAPI18)
  SET(m4_def -D __FARAPI17__)
ENDIF(FARAPI18)
MACRO(m4_process src dst)
  ADD_CUSTOM_COMMAND(OUTPUT ${dst} COMMAND m4 -P ${m4_def} -I ${bin} ${src} > ${dst} DEPENDS ${src} ${bin}/version.m4 VERBATIM)
ENDMACRO(m4_process)

m4_process(${src}/UnInstall.rc.m4 ${bin}/UnInstall.rc)
enc_file(${src}/UnInstall_Eng.lng ${bin}/UnInstall_Eng.lng us-ascii)
enc_file(${src}/UnInstall_Rus.lng ${bin}/UnInstall_Rus.lng cp866)
m4_process(${src}/UnInstall_Eng.hlf.m4 ${bin}/UnInstall_Eng.hlf.1)
enc_file(${bin}/UnInstall_Eng.hlf.1 ${bin}/UnInstall_Eng.hlf us-ascii)
m4_process(${src}/UnInstall_Rus.hlf.m4 ${bin}/UnInstall_Rus.hlf.1)
enc_file(${bin}/UnInstall_Rus.hlf.1 ${bin}/UnInstall_Rus.hlf cp866)
m4_process(${src}/File_ID.diz.m4 ${bin}/File_ID.diz.1)
enc_file(${bin}/File_ID.diz.1 ${bin}/File_ID.diz cp866)
enc_file(${src}/ReadMe.Rus.txt ${bin}/ReadMe.Rus.txt cp866)
enc_file(${src}/WhatsNew.Rus.txt ${bin}/WhatsNew.Rus.txt cp866)
enc_file(${src}/TechInfo.Rus.reg ${bin}/TechInfo.Rus.reg cp866)

# generate version information
ADD_CUSTOM_TARGET(check_rev ALL svnversion -cn ${src} > .svnrev COMMAND ${svnrev} ${bin}/revision < .svnrev COMMAND rm .svnrev VERBATIM)
ADD_CUSTOM_COMMAND(OUTPUT ${bin}/version.m4
  COMMAND ${svnrev} ${src}/version.m4.tmpl ${bin}/version.m4 ${src}/version ${bin}/revision
  DEPENDS ${src}/version.m4.tmpl ${src}/version ${bin}/revision
  VERBATIM)
m4_process(${src}/version.rc.m4 ${bin}/version.rc)

# copy generated files into test directory
IF(FARAPI18)
  SET(uni_suffix "_uni")
ENDIF(FARAPI18)
IF(CMAKE_CL_64)
  SET(x64_suffix "_x64")
ENDIF(CMAKE_CL_64)
IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  SET(dbg_suffix "_dbg")
ENDIF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")

# prepare distribution archive
FILE(READ ${src}/version version)
SET(distrib_package "${bin}/${PROJECT_NAME}_${version}${uni_suffix}${x64_suffix}${dbg_suffix}.7z")
GET_TARGET_PROPERTY(library_path ${PROJECT_NAME} LOCATION)
SET(distrib_files ${library_path} ${bin}/UnInstall_Eng.hlf ${bin}/UnInstall_Eng.lng ${bin}/UnInstall_Rus.hlf ${bin}/UnInstall_Rus.lng ${bin}/File_ID.diz ${bin}/ReadMe.Rus.txt ${bin}/TechInfo.Rus.reg ${bin}/WhatsNew.Rus.txt ${bin}/${PROJECT_NAME}.map ${bin}/${PROJECT_NAME}.pdb)
ADD_CUSTOM_TARGET(distrib
  COMMAND ${CMAKE_COMMAND} -E remove ${distrib_package}
  COMMAND 7z a -mx=9 ${distrib_package} ${distrib_files}
  DEPENDS ${PROJECT_NAME} ${distrib_files}
  VERBATIM)
