cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(MyProcessor)

include(dependencies.cmake)

add_library(MyProcessor STATIC
  src/Model.cpp
  src/Model.hpp
  src/UI.hpp
  src/Processor.hpp
)

avnd_make_all(
  TARGET MyProcessor
  MAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/Processor.hpp"
  MAIN_CLASS MyProcessor
  C_NAME my_processor
)
