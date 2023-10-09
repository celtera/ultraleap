include(FetchContent)

# Avendish
FetchContent_Declare(
  avendish
  GIT_REPOSITORY "https://github.com/celtera/avendish"
  GIT_TAG main
  GIT_PROGRESS true
)
FetchContent_Populate(avendish)

set(CMAKE_PREFIX_PATH "${avendish_SOURCE_DIR};${CMAKE_PREFIX_PATH}")
find_package(Avendish REQUIRED)

# ConcurrentQueue
FetchContent_Declare(
  concurrentqueue
  GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(concurrentqueue)

# LeapSDK
if(NOT LEAPSDK_PATH)
    message(FATAL_ERROR "LEAPSDK_PATH must point to the LeapC SDK folder:
    cmake . -DLEAPSDK_PATH=c:/dev/LeapSDK")
endif()

find_path(LEAPC_HEADERS LeapC.h HINTS "${LEAPSDK_PATH}/include")
find_library(LEAPC_LIBRARY NAMES LeapC HINTS "${LEAPSDK_PATH}/lib/x64")
find_file(LEAPC_RUNTIME NAMES LeapC.dll HINTS "${LEAPSDK_PATH}/lib/x64")
add_library(LeapC::Leap SHARED IMPORTED)
set_target_properties(LeapC::Leap PROPERTIES
    IMPORTED_LOCATION ${LEAPC_LIBRARY}
    IMPORTED_IMPLIB ${LEAPC_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES "${LEAPC_HEADERS}"
)
