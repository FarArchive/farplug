
SET(FARAPI18 FALSE CACHE BOOL "Select target Far API version")

# find message file compiler
FIND_PROGRAM(msgc "msgc" ${top}/msgc/bin)
IF(NOT msgc)
  MESSAGE(FATAL_ERROR "msgc.exe not found")
ENDIF(NOT msgc)

# find version info generator
FIND_PROGRAM(svnrev "svnrev" ${top}/svnrev/bin)
IF(NOT svnrev)
  MESSAGE(FATAL_ERROR "svnrev.exe not found")
ENDIF(NOT svnrev)

# find m4 macro processor
FIND_PROGRAM(m4 "m4" ${top}/tools)
IF(NOT m4)
  MESSAGE(FATAL_ERROR "m4 not found")
ENDIF(NOT m4)

# find m4 macro processor
FIND_PROGRAM(iconv "iconv" ${top}/tools)
IF(NOT iconv)
  MESSAGE(FATAL_ERROR "iconv not found")
ENDIF(NOT iconv)

# find 7-zip
FIND_PROGRAM(arc "7z" ${top}/tools)
IF(NOT arc)
  MESSAGE(FATAL_ERROR "7z not found")
ENDIF(NOT arc)

# FAR SDK include files
FIND_PATH(far_inc NAMES plugin.hpp farcolor.hpp farkeys.hpp PATHS ${top}/farsdk)
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

# convert file encoding to unicode if needed
MACRO(enc_file src dst src_enc)
  IF(FARAPI18)
    ADD_CUSTOM_COMMAND(OUTPUT ${dst} COMMAND ${iconv} -f ${src_enc} -t utf-16 ${src} > ${dst} DEPENDS ${src} VERBATIM)
  ELSE(FARAPI18)
    ADD_CUSTOM_COMMAND(OUTPUT ${dst} COMMAND ${CMAKE_COMMAND} -E copy ${src} ${dst} DEPENDS ${src} VERBATIM)
  ENDIF(FARAPI18)
ENDMACRO(enc_file)

# process m4 macro file
IF(FARAPI18)
  SET(m4_def -D __FARAPI18__)
ELSE(FARAPI18)
  SET(m4_def -D __FARAPI17__)
ENDIF(FARAPI18)
MACRO(m4_process src dst)
  ADD_CUSTOM_COMMAND(OUTPUT ${dst} COMMAND ${m4} -P ${m4_def} -I ${bin} ${src} > ${dst} DEPENDS ${src} ${bin}/version.m4 VERBATIM)
ENDMACRO(m4_process)

# generate version information
MACRO(gen_ver)
  ADD_CUSTOM_TARGET(check_rev ALL
    COMMAND svnversion -cn ${src} > .svnrev
    COMMAND ${svnrev} ${bin}/revision < .svnrev
    COMMAND ${CMAKE_COMMAND} -E remove .svnrev
    VERBATIM)
  ADD_DEPENDENCIES(${PROJECT_NAME} check_rev)
  ADD_CUSTOM_COMMAND(OUTPUT ${bin}/version.m4
    COMMAND ${svnrev} ${src}/version.m4.tmpl ${bin}/version.m4 ${src}/version ${bin}/revision
    DEPENDS ${src}/version.m4.tmpl ${src}/version ${bin}/revision
    VERBATIM)
ENDMACRO(gen_ver)

# prepare distribution archive
MACRO(gen_distrib)
  FILE(READ ${src}/version distr_suffix)
  IF(FARAPI18)
    SET(distr_suffix "${distr_suffix}_uni")
  ENDIF(FARAPI18)
  IF(CMAKE_CL_64)
    SET(distr_suffix "${distr_suffix}_x64")
  ENDIF(CMAKE_CL_64)
  IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    SET(distr_suffix "${distr_suffix}_dbg")
  ENDIF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  SET(distrib_package "${bin}/${PROJECT_NAME}_${distr_suffix}.7z")
  GET_TARGET_PROPERTY(library_path ${PROJECT_NAME} LOCATION)
  SET(distrib_files ${library_path} ${bin}/${PROJECT_NAME}.map ${ARGV})
  IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    SET(distrib_files ${distrib_files} ${bin}/${PROJECT_NAME}.pdb)
  ENDIF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  ADD_CUSTOM_TARGET(distrib
    COMMAND ${CMAKE_COMMAND} -E remove ${distrib_package}
    COMMAND ${arc} a -mx=9 ${distrib_package} ${distrib_files}
    DEPENDS ${PROJECT_NAME} ${distrib_files}
    VERBATIM)
  ADD_DEPENDENCIES(distrib check_rev)
ENDMACRO(gen_distrib)
