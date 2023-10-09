include(FetchContent)

# Max/MSP sdk
FetchContent_Declare(
  max_sdk
  GIT_REPOSITORY "https://github.com/jcelerier/max-sdk-base"
  GIT_TAG        main
  GIT_PROGRESS   true
)
FetchContent_Populate(max_sdk)

set(AVND_MAXSDK_PATH "${max_sdk_SOURCE_DIR}")

# PureData
if(WIN32)
  FetchContent_Declare(
    puredata
    URL "http://msp.ucsd.edu/Software/pd-0.54-0.msw.zip"
  )
  FetchContent_Populate(puredata)
  set(CMAKE_PREFIX_PATH "${puredata_SOURCE_DIR}/src;${puredata_SOURCE_DIR}/bin;${CMAKE_PREFIX_PATH}")
else()
  FetchContent_Declare(
    puredata
    GIT_REPOSITORY "https://github.com/pure-data/pure-data"
    GIT_TAG        master
    GIT_PROGRESS   true
  )
  FetchContent_Populate(puredata)
  set(CMAKE_PREFIX_PATH "${puredata_SOURCE_DIR}/src;${CMAKE_PREFIX_PATH}")
endif()


# ConcurrentQueue
FetchContent_Declare(
  concurrentqueue
  GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
  GIT_TAG        master
  GIT_PROGRESS   true
)
FetchContent_MakeAvailable(concurrentqueue)

# LeapSDK
if(NOT LEAPSDK_PATH AND NOT LEAPSDK_URL)
    message(FATAL_ERROR "LEAPSDK_PATH must point to the LeapC SDK folder:
    cmake . -DLEAPSDK_PATH=c:/dev/LeapSDK")
endif()

if(LEAPSDK_URL)
  FetchContent_Declare(
    LeapSDK
    GIT_REPOSITORY "${LEAPSDK_URL}"
    GIT_TAG        main
    GIT_PROGRESS   true
  )
  FetchContent_MakeAvailable(LeapSDK)
elseif(LEAPSDK_PATH)
  set(LeapSDK_SOURCE_DIR "${LEAPSDK_PATH}")
endif()

find_path(LEAPC_HEADERS LeapC.h
  HINTS
    "${LeapSDK_SOURCE_DIR}/include"
)
find_library(LEAPC_LIBRARY NAMES LeapC
  HINTS
    "${LeapSDK_SOURCE_DIR}/lib/x64"
)

add_library(LeapC::Leap SHARED IMPORTED)
set_target_properties(LeapC::Leap PROPERTIES
    IMPORTED_LOCATION ${LEAPC_LIBRARY}
    IMPORTED_IMPLIB ${LEAPC_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES "${LEAPC_HEADERS}"
)
if(WIN32)
  find_file(LEAPC_RUNTIME NAMES LeapC.dll HINTS "${LeapSDK_SOURCE_DIR}/lib/x64")
else()
  # Important: the official .dylib on mac is libLeapC.dylib but when linking against
  # it, the resulting external looks for libLeapC.5.dylib
  find_file(LEAPC_RUNTIME NAMES libLeapC.5.dylib HINTS "${LeapSDK_SOURCE_DIR}/lib/x64")
endif()

# Avendish
if(AVENDISH_EXTERNAL_SOURCE_DIR)
FetchContent_Declare(
    avendish
    DOWNLOAD_COMMAND ""
    SOURCE_DIR "${AVENDISH_EXTERNAL_SOURCE_DIR}"
)
else()
FetchContent_Declare(
  avendish
  GIT_REPOSITORY "https://github.com/celtera/avendish"
  GIT_TAG main
  GIT_PROGRESS true
)
endif()
FetchContent_Populate(avendish)

set(CMAKE_PREFIX_PATH "${avendish_SOURCE_DIR};${CMAKE_PREFIX_PATH}")
find_package(Avendish REQUIRED)
