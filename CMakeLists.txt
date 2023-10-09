cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(MyProcessor)

include(dependencies.cmake)

include_directories(${concurrentqueue_SOURCE_DIR})
include_directories(${MAXSDK_MAX_INCLUDE_DIR})
include_directories(${MAXSDK_MSP_INCLUDE_DIR})

# Create a target for our base library
add_library(UltraLeap STATIC
  src/Model.cpp
  src/Model.hpp
  src/Leap.hpp
)

target_link_libraries(UltraLeap PUBLIC
    Boost::boost
    LeapC::Leap
)

avnd_make(
  TARGET UltraLeap
  MAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/Model.hpp"
  MAIN_CLASS ul::UltraLeap
  C_NAME avnd_ultraleap
  BACKENDS max
)

# Create a Max package
if(MAX_PACKAGE_FOLDER)
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

        EXTERNALS UltraLeap_max
        SUPPORT "${LEAPC_RUNTIME}"
    )
endif()
