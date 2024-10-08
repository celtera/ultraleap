cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(Ultraleap)

set(CMAKE_CXX_STANDARD 23)
set(AVND_DISABLE_AUTOMAXREF ON CACHE "" INTERNAL)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(dependencies.cmake)

include_directories(${concurrentqueue_SOURCE_DIR})
include_directories(${MAXSDK_MAX_INCLUDE_DIR})
include_directories(${MAXSDK_MSP_INCLUDE_DIR})

## Creating the external ##
# Create a target for our base library

add_library(UltraLeap_base STATIC
  src/Model.cpp
  src/Model.hpp
  src/Leap.hpp
  src/TripleBuffer.hpp
)

set_target_properties(UltraLeap_base PROPERTIES
    PREFIX "prefix_bind_"
    IMPORT_PREFIX "import_bind_"
)


target_link_libraries(UltraLeap_base PUBLIC
    Avendish::Avendish
    Boost::boost
    LeapC::Leap
)

set(BACKENDS_TO_BUILD "dump")
if(WIN32 OR APPLE)
  list(APPEND BACKENDS_TO_BUILD "max")
endif()
if(NOT MSVC)
  list(APPEND BACKENDS_TO_BUILD "pd")
endif()

avnd_make(
  TARGET UltraLeap_base
  MAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/Model.hpp"
  MAIN_CLASS ul::UltraLeap
  C_NAME ultraleap
  BACKENDS "${BACKENDS_TO_BUILD}"
)


## Packaging ##
if(MSVC)
  return()
endif()

set(FILES_TO_INSTALL "${LEAPC_RUNTIME}")
if(WIN32 AND NOT MSVC)
  get_filename_component(cxx_path "${CMAKE_CXX_COMPILER}" PATH)
  set(FILES_TO_INSTALL
        ${FILES_TO_INSTALL}
        "${cxx_path}/libc++.dll"
        "${cxx_path}/libunwind.dll"
  )
endif()

# Create a Max package
if(MAX_PACKAGE_FOLDER AND TARGET UltraLeap_base_max)
  avnd_create_max_package(
      CODESIGN
      NOTARIZE

      NAME UltraLeap
      SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/max/package"
      PACKAGE_ROOT "${MAX_PACKAGE_FOLDER}"

      KEYCHAIN_FILE "$ENV{MAC_CODESIGN_KEYCHAIN}"
      KEYCHAIN_PASSWORD "$ENV{MAC_CODESIGN_KEYCHAIN_PASSWORD}"
      CODESIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/src/entitlements.plist"
      CODESIGN_IDENTITY "$ENV{MAC_CODESIGN_IDENTITY}"
      NOTARIZE_TEAM "$ENV{MAC_CODESIGN_TEAM}"
      NOTARIZE_EMAIL "$ENV{MAC_CODESIGN_EMAIL}"
      NOTARIZE_PASSWORD "$ENV{MAC_NOTARIZE_PASSWORD}"

      EXTERNALS UltraLeap_base_max
      SUPPORT "${FILES_TO_INSTALL}"
  )
endif()

if(PD_PACKAGE_FOLDER AND TARGET UltraLeap_base_pd)
  avnd_create_pd_package(
      NAME UltraLeap
      SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/pd/"
      PACKAGE_ROOT "${PD_PACKAGE_FOLDER}"

      EXTERNALS UltraLeap_base_pd
      SUPPORT "${FILES_TO_INSTALL}"
  )
endif()
