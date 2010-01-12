CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(msiupdate)

# useful values
SET(src ${CMAKE_CURRENT_SOURCE_DIR})
SET(bin ${CMAKE_CURRENT_BINARY_DIR})
SET(top ${src}/..)

INCLUDE(${top}/cmake/common.cmake)

IF(DEFINED MSVC)
  SET(CFG_THREADS 1)
  SET(CFG_OPT_TYPE "size")
  INCLUDE(${top}/cmake/MSVC.cmake)
ENDIF(DEFINED MSVC)

# project source files
SET(cpp_sources plugin.cpp pathutils.cpp strutils.cpp farutils.cpp sysutils.cpp ui.cpp inet.cpp iniparse.cpp update.cpp options.cpp transact.cpp)
add_precomp_header("headers.hpp" "headers.cpp" cpp_sources)
SET(sources ${cpp_sources} plugin.def ${bin}/msg.h ${bin}/version.rc)

# configure project
ADD_DEFINITIONS(-D_FAR_USE_FARFINDDATA -D_WIN32_WINNT=0x0500)
INCLUDE_DIRECTORIES(${src} ${far_inc} ${bin})
ADD_LIBRARY(${PROJECT_NAME} SHARED ${sources})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} winhttp msi)
ADD_CUSTOM_TARGET(lng_hlf DEPENDS ${bin}/en.lng ${bin}/ru.lng VERBATIM)
ADD_DEPENDENCIES(${PROJECT_NAME} lng_hlf)
ADD_DEPENDENCIES(lng_hlf check_rev)

# generate language files
ADD_CUSTOM_COMMAND(
  OUTPUT ${bin}/msg.h ${bin}/en.lng.1 ${bin}/ru.lng.1
  COMMAND ${msgc} -in ${src}/en.msg ${src}/ru.msg -out ${bin}/msg.h ${bin}/en.lng.1 ${bin}/ru.lng.1
  DEPENDS ${src}/en.msg ${src}/ru.msg
  VERBATIM)

enc_file(${bin}/en.lng.1 ${bin}/en.lng us-ascii)
enc_file(${bin}/ru.lng.1 ${bin}/ru.lng cp866)

gen_ver()
m4_process(${src}/version.rc.m4 ${bin}/version.rc)

gen_distrib(${bin}/en.lng ${bin}/ru.lng)
