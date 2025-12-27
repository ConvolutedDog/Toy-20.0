cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0077 NEW)

option(USE_CLANG "Use Clang compiler" OFF)
if(USE_CLANG)
  find_program(CLANG_C_COMPILER NAMES clang)
  find_program(CLANG_CXX_COMPILER NAMES clang++)
  if(CLANG_C_COMPILER AND CLANG_CXX_COMPILER)
    set(CMAKE_C_COMPILER ${CLANG_C_COMPILER})
    set(CMAKE_CXX_COMPILER ${CLANG_CXX_COMPILER})
  endif()
else()
  set(CMAKE_C_COMPILER gcc)
  set(CMAKE_CXX_COMPILER g++)
endif()

project(Toy LANGUAGES CXX C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to conform to")

add_compile_options(-Wfatal-errors -w)

set(CMAKE_BUILD_WITH_INSTALL_NAME_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

message(STATUS "Using CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "Using CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_package(MLIR REQUIRED CONFIG)
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/lib)
set(MLIR_BINARY_DIR ${CMAKE_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(HandleLLVMOptions)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  message(STATUS "Forcing Debug Flags: -g -O0")
  add_compile_options(-g -O0)
  remove_definitions(-DNDEBUG)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(
      "-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=${CMAKE_BINARY_DIR}"
    )
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
      "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=${CMAKE_BINARY_DIR}"
    )
  endif()
endif()

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})
link_directories(${LLVM_BUILD_LIBRARY_DIR})
add_definitions(${LLVM_DEFINITIONS})

add_custom_target(Toy ALL)

macro(add_toy_chapter name)
  add_dependencies(Toy ${name})
  add_llvm_example(${name} ${ARGN})
endmacro(add_toy_chapter name)

add_subdirectory(Ch1)
add_subdirectory(Ch2)
add_subdirectory(Ch3)
add_subdirectory(Ch4)
add_subdirectory(Ch5)
add_subdirectory(Ch6)
add_subdirectory(Ch7)
add_subdirectory(MyToy)
