# CMake build script for the ccnet project
#
# Building (out of source build):
# > mkdir build && cd build
# > cmake .. [-DSETTINGS=VALUE]
# > cmake --build .
#
# Install:
# > cmake --build .
if(EXISTS CMakeLists.txt)
  message(FATAL_ERROR
    "Looks like you are trying to run cmake from the base source directory.\n"
    "** RUNNING CMAKE FROM THE BASE DIRECTORY WILL NOT WORK **\n"
    "To Fix:\n"
    " 1. Remove the CMakeCache.txt file in this directory. ex: rm CMakeCache.txt\n"
    " 2. Create a build directory from here. ex: mkdir build\n"
    " 3. cd into that directory. ex: cd build\n"
    " 4. Run cmake from the build directory. ex: cmake ..\n"
    " 5. Run make from the build directory. ex: make\n"
    "Full paste-able example:\n"
    "( rm -f CMakeCache.txt; mkdir build; cd build; cmake ..; make )\n"
    )
endif()
cmake_minimum_required(VERSION 2.8)

# Add path for custom modules
set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_SOURCE_DIR}/cmake/Modules/"
  )

project(ccnet)

set(CCNET_VERSION_MAJOR 3)
set(CCNET_VERSION_MINOR 0)
set(CCNET_VERSION_PATCH 5)
set(CCNET_VERSION_STRING "${CCNET_VERSION_MAJOR}.${CCNET_VERSION_MINOR}.${CCNET_VERSION_PATCH}")

if (NOT PACKAGE_VERSION)
  set(PACKAGE_VERSION "${CCNET_VERSION_STRING}")
endif()
add_definitions(-DPACKAGE_VERSION=\"${PACKAGE_VERSION}\")

set(PACKAGE_NAME ccnet)
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "freeplant@gmail.com")
set(PACKAGE_URL "http://seafile.com")

set(BUG_REPORT_EMAIL "${PACKAGE_BUGREPORT}" CACHE STRING
  "Default Email where bug reports are to be submitted.")
set(BUG_REPORT_URL "https://github.com/haiwen/seafile/issues" CACHE STRING
  "Default Url where bug reports are to be submitted.")

# Configure CPack
set(CPACK_PACKAGE_NAME ${PACKAGE_NAME})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "ccnet")
set(CPACK_PACKAGE_VENDOR "Haiwen Inc.")
set(CPACK_PACKAGE_VERSION_MAJOR ${CCNET_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${CCNET_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${CCNET_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENCE.txt")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Lingtao Pan <freeplant@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://www.seafile.com")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsearpc1 (>=1.0) libevent-2.0-5 (>=2.0) libsqlite3-0 (>=3.7.0) libzdb9 (>=2.10) libglib2.0 (>= 2.28.0) libjansson4 (>=2.1)")
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
set(CPACK_RPM_PACKAGE_URL "http://www.seafile.com")
set(CPACK_RPM_PACKAGE_REQUIRES "libsearpc libevent sqlite libzdb glib2 jansson")
include(CPack)

# SET UP DEFAULT CMAKE_BUILD_TYPE
if( NOT CMAKE_CONFIGURATION_TYPES )
  # Build Debug by default
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
  endif ()
else()
  # Using a multi-configuration generator eg MSVC or Xcode
  # that uses CMAKE_CONFIGURATION_TYPES and not CMAKE_BUILD_TYPE
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

option(CCNET_BUILD_CLI
  "Build ccnet cli." ON)
option(CCNET_BUILD_DEMO
  "Build ccnet demo." OFF)
option(CCNET_BUILD_TOOLS
  "Build ccnet tools." ON)
option(CCNET_ENABLE_CLIENT
  "Build ccnet client." ON)
option(CCNET_ENABLE_SERVER
  "Build ccnet server." OFF)
option(CCNET_ENABLE_CLUSTER
  "Build ccnet cluster (abandoned)." OFF)
option(CCNET_BUILD_TESTS
  "Build ccnet tests." OFF)
option(CCNET_BUILD_PYTHON
  "Build ccnet python binding." ON)
option(CCNET_BUILD_STATIC
  "Build static library" OFF)

if(CCNET_BUILD_STATIC)
  set(CCNET_LIBTYPE STATIC)
else()
  set(CCNET_LIBTYPE SHARED)
endif()

# Find Library Dependencies
find_package(Threads REQUIRED)
find_package(GLIB2 2.26.0 REQUIRED)
find_package(JANSSON 2.2.1 REQUIRED)
find_package(OpenSSL 0.9.8 REQUIRED)
find_package(SQLITE3 3.7.0 REQUIRED)
find_package(SearpcCodegen REQUIRED)
find_package(Valac REQUIRED)
find_package(UUID REQUIRED)
find_package(LIBEVENT 2.0 REQUIRED)
find_package(LIBZDB 2.10 REQUIRED)

if(NOT LIBSEARPC_FOUND)
  if(NOT EXISTS "${CMAKE_SOURCE_DIR}/deps/libsearpc")
    set(LIBSEARPC_FOUND false)
    find_package(LIBSEARPC 1.0 REQUIRED)
  else()
    set(LIBSEARPC_FOUND true)
    add_subdirectory(deps/libsearpc)
  endif()
endif(NOT LIBSEARPC_FOUND)

# System Check
# Borrowed from llvm
include(config-ix)

# Verify that we can find a Python 2 interpreter. Python 3 is unsupported.
set(Python_ADDITIONAL_VERSIONS 2.7 2.6 2.5)
include(FindPythonInterp)
if(NOT PYTHONINTERP_FOUND)
  message(FATAL_ERROR
"Unable to find Python interpreter, required for searpc-codegen and pysearpc.
Please install Python or specify the PYTHON_EXECUTABLE CMake variable.")
endif()

# CONFIG.H
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# INSTALLATION PATHS
set(BIN_INSTALL_DIR bin CACHE PATH "Where to install binaries to.")
set(LIB_INSTALL_DIR lib CACHE PATH "Where to install libraries to.")
set(INCLUDE_INSTALL_DIR include CACHE PATH "Where to install headers to.")

# INSTALLATION header files
set(SRC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_INCLUDE_CCNET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/ccnet)
set(CCNET_INCLUDE_DIR ${SRC_INCLUDE_DIR})
set(CCNET_INCLUDE_CCNET_DIR ${SRC_INCLUDE_CCNET_DIR})
file(GLOB SRC_INCLUDE_H include/**/*.h)
file(GLOB SRC_INCLUDE_ONLY_H include/*.h)
install(DIRECTORY ${CCNET_INCLUDE_CCNET_DIR}
  DESTINATION ${INCLUDE_INSTALL_DIR}
  )
install(FILES ${SRC_INCLUDE_ONLY_H}
  DESTINATION ${INCLUDE_INSTALL_DIR}
  )
include_directories(${SRC_INCLUDE_DIR})

# LIB
set(SRC_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
file(GLOB SRC_LIB_H ${SRC_LIB_DIR}/*.h)
add_subdirectory(lib)
set(LIBCCNET_INCLUDE_DIR ${SRC_INCLUDE_DIR})
set(LIBCCNET_INCLUDE_DIRS ${LIBCCNET_INCLUDE_DIR})
set(LIBCCNET_LIBRARIES libccnet)

# NET
file(GLOB SRC_NET_H net/common/*.h)
add_subdirectory(net)

# DEMO
if(CCNET_BUILD_DEMO)
  add_subdirectory(demo)
endif(CCNET_BUILD_DEMO)

# CLI
if(CCNET_BUILD_CLI)
  add_subdirectory(cli)
ENDIF(CCNET_BUILD_CLI)

# TOOLS
if(CCNET_BUILD_TOOLS)
  add_subdirectory(tools)
ENDIF(CCNET_BUILD_TOOLS)

# PYTHON BINDING
if(CCNET_BUILD_PYTHON)
  include(FindPythonLibs)
  if(NOT PYTHONLIBS_FOUND OR NOT PYTHON_EXECUTABLE)
    message(SEND_ERROR "You need Python to build the python bindings")
  else()
    add_subdirectory(python)
  endif()
endif(CCNET_BUILD_PYTHON)

# Unit Tests
if(CCNET_BUILD_TESTS)
  set(SRC_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  enable_testing()
  add_subdirectory(tests)
endif(CCNET_BUILD_TESTS)
