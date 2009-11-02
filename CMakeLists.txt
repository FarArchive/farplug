CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(7z2)

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
SET(sources plugin.cpp archive.cpp pathutils.cpp strutils.cpp plugin.def ${bin}/msg.h)

# configure project
ADD_DEFINITIONS(-D_FAR_USE_FARFINDDATA -D_WIN32_WINNT=0x0500)
INCLUDE_DIRECTORIES(${src} ${src}/7z_api ${far_inc} ${bin})
ADD_LIBRARY(${PROJECT_NAME} SHARED ${sources})
ADD_CUSTOM_TARGET(lng_hlf DEPENDS ${bin}/en.lng VERBATIM)
ADD_DEPENDENCIES(${PROJECT_NAME} lng_hlf)
ADD_DEPENDENCIES(lng_hlf check_rev)

# generate language files
ADD_CUSTOM_COMMAND(
  OUTPUT ${bin}/msg.h ${bin}/en.lng.1
  COMMAND ${msgc} -in ${src}/en.msg -out ${bin}/msg.h ${bin}/en.lng.1
  DEPENDS ${src}/en.msg
  VERBATIM)

enc_file(${bin}/en.lng.1 ${bin}/en.lng us-ascii)
