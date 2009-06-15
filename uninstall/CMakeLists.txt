CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(UnInstall)

# useful values
SET(src ${CMAKE_CURRENT_SOURCE_DIR})
SET(bin ${CMAKE_CURRENT_BINARY_DIR})
SET(top ${src}/..)

INCLUDE(${top}/cmake/common.cmake)

IF(DEFINED MSVC)
  SET(CFG_OPT_TYPE "size")
  SET(CFG_RUNTIME_TYPE "none")
  SET(CFG_ENTRY_NAME 0)
  INCLUDE(${top}/cmake/MSVC.cmake)
ENDIF(DEFINED MSVC)

# project source files
SET(sources UnInstall.cpp ${bin}/UnInstall.rc)
IF(FARAPI18)
  SET(sources ${sources} UnInstallW.def)
ELSE(FARAPI18)
  SET(sources ${sources} UnInstall.def)
ENDIF(FARAPI18)

# configure project
ADD_DEFINITIONS(-D_FAR_USE_FARFINDDATA -DSTRSAFE_NO_DEPRECATE -D_WIN32_WINNT=0x0500)
IF(FARAPI18)
  ADD_DEFINITIONS(-DFARAPI18 -DUNICODE -D_UNICODE)
ELSE(FARAPI18)
  ADD_DEFINITIONS(-DFARAPI17)
ENDIF(FARAPI18)
INCLUDE_DIRECTORIES(${src} ${far_inc})
ADD_LIBRARY(${PROJECT_NAME} SHARED ${sources})
ADD_CUSTOM_TARGET(lng_hlf DEPENDS ${bin}/UnInstall_Eng.hlf ${bin}/UnInstall_Eng.lng ${bin}/UnInstall_Rus.hlf ${bin}/UnInstall_Rus.lng VERBATIM)
ADD_DEPENDENCIES(${PROJECT_NAME} lng_hlf)
ADD_DEPENDENCIES(lng_hlf check_rev)

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

gen_ver()
m4_process(${src}/version.rc.m4 ${bin}/version.rc)

gen_distrib(${bin}/UnInstall_Eng.hlf ${bin}/UnInstall_Eng.lng ${bin}/UnInstall_Rus.hlf ${bin}/UnInstall_Rus.lng ${bin}/File_ID.diz ${bin}/ReadMe.Rus.txt ${bin}/TechInfo.Rus.reg ${bin}/WhatsNew.Rus.txt)
