cmake_minimum_required(VERSION 3.0)
project(libtermpod LANGUAGES C)

set(PROJECT_VERSION 1.0)
set(PROJECT_DESCRIPTION "POD game archive format library written in C for all existing versions")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if(CMAKE_C_COMPILER_LOADED)
  include (CheckIncludeFile)
  include (CheckCSourceCompiles)
elseif(CMAKE_CXX_COMPILER_LOADED)
  include (CheckIncludeFileCXX)
  include (CheckCXXSourceCompiles)
else()
  message(FATAL_ERROR "FindMath only works if either C or CXX language is enabled")
endif()

include(GNUInstallDirs)
include(FindPkgConfig)

if(MSYS AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
set(CMAKE_INSTALL_PREFIX $ENV{MINGW_PREFIX} CACHE PATH $ENV{MINGW_PREFIX} FORCE)
endif()

if(WIN32)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
else()
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
endif()

#set( CMAKE_C_STANDARD_REQUIRED ON )
#set( CMAKE_C_STANDARD gnu2x )

set(SOURCE_FILES
	src/path.c
	src/mkdir_p.c
	src/pod_crc.c
	src/pod_common.c
	src/libtermpod.c
	src/pod_zip.c
	src/pod1.c
	src/pod2.c
	src/pod3.c
	src/pod4.c
	src/pod5.c
	src/pod6.c
	src/epd.c
)

set(INCLUDE_FILES
	src/path.h
	src/mkdir_p.h
	src/pod_crc.h
	src/pod_common.h
	src/libtermpod.h
	src/pod_zip.h
	src/pod1.h
	src/pod2.h
	src/pod3.h
	src/pod4.h
	src/pod5.h
	src/pod6.h
	src/epd.h
)

set(EXAMPLE_SOURCE_FILES
)

add_subdirectory ("Deps/Libzip")

#pkg_search_module(ZIP REQUIRED libzip)
#pkg_search_module(CRC REQUIRED libcrcle)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(TERMPOD_ARCH "x64")
else()
  set(TERMPOD_ARCH "x86")
endif()

# termpod library
add_library(libtermpod ${SOURCE_FILES} ${INCLUDE_FILES}
	${CRC_INCLUDE} ${ZIP_INCLUDE})
# target_compile_options( libtermpod PUBLIC ${CRC_CFLAGS} ${ZIP_CFLAGS})
# target_include_directories(libtermpod PUBLIC ${CRC_INCLUDEDIR} ${ZIP_INCLUDEDIR})
# target_link_libraries( libtermpod PUBLIC ${CRC_LIBRARIES} ${ZIP_LIBRARIES} )
target_include_directories( libtermpod PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Deps/libzip/lib" "${CMAKE_CURRENT_SOURCE_DIR}/Deps/libzip/Deps/zlib" )
target_link_libraries ( libtermpod PUBLIC zip )
