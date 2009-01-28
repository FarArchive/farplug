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

# FAR SDK include files
IF(FARAPI18)
  SET(far_inc ${top}/farsdk/unicode)
ELSE(FARAPI18)
  SET(far_inc ${top}/farsdk)
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
SET(ver_major "1")
SET(ver_minor "10")
SET(ver_patch "4")
ADD_DEFINITIONS(-D_FAR_USE_FARFINDDATA -D_CRT_SECURE_NO_WARNINGS)
IF(FARAPI18)
  ADD_DEFINITIONS(-DFARAPI18 -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0500)
ELSE(FARAPI18)
  ADD_DEFINITIONS(-DFARAPI17)
ENDIF(FARAPI18)
INCLUDE_DIRECTORIES(${src} ${far_inc})
ADD_LIBRARY(${PROJECT_NAME} SHARED ${sources})

MACRO(enc_file src dst src_enc)
  IF(FARAPI18)
    ADD_CUSTOM_COMMAND(OUTPUT ${dst} COMMAND iconv -f ${src_enc} -t utf-16 ${src} > ${dst} DEPENDS ${src} VERBATIM)
  ELSE(FARAPI18)
    ADD_CUSTOM_COMMAND(OUTPUT ${dst} COMMAND ${CMAKE_COMMAND} -E copy ${src} ${dst} DEPENDS ${src} VERBATIM)
  ENDIF(FARAPI18)
ENDMACRO(enc_file)

SET(m4_def -D __VER_MAJOR__=${ver_major} -D __VER_MINOR__=${ver_minor} -D __VER_PATCH__=${ver_patch})
IF(FARAPI18)
  SET(m4_def ${m4_def} -D __FARAPI18__)
ELSE(FARAPI18)
  SET(m4_def ${m4_def} -D __FARAPI17__)
ENDIF(FARAPI18)
MACRO(m4_process src dst)
  ADD_CUSTOM_COMMAND(OUTPUT ${dst} COMMAND m4 -P ${m4_def} -I ${bin} ${src} > ${dst} DEPENDS ${src} VERBATIM)
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
SET(distrib_package "${bin}/${PROJECT_NAME}_${ver_major}.${ver_minor}.${ver_patch}${uni_suffix}${x64_suffix}${dbg_suffix}.7z")
GET_TARGET_PROPERTY(library_path ${PROJECT_NAME} LOCATION)
SET(distrib_files ${library_path} ${bin}/UnInstall_Eng.hlf ${bin}/UnInstall_Eng.lng ${bin}/UnInstall_Rus.hlf ${bin}/UnInstall_Rus.lng ${bin}/File_ID.diz ${bin}/ReadMe.Rus.txt ${bin}/TechInfo.Rus.reg ${bin}/WhatsNew.Rus.txt ${bin}/${PROJECT_NAME}.map ${bin}/${PROJECT_NAME}.pdb)
ADD_CUSTOM_TARGET(distrib
  COMMAND ${CMAKE_COMMAND} -E remove ${distrib_package}
  COMMAND 7z a -mx=9 ${distrib_package} ${distrib_files}
  DEPENDS ${PROJECT_NAME} ${distrib_files}
  VERBATIM)
