cmake_minimum_required(VERSION 3.18)
project(brainf LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LLVM_DIR "/usr" CACHE PATH "Path to LLVM installation")
set(LLVM_INCLUDE_DIR "${LLVM_DIR}/include" CACHE PATH "Path to LLVM includes")
set(LLVM_LIBRARY_DIR "${LLVM_DIR}/lib" CACHE PATH "Path to LLVM libraries")

find_library(CLANG clang HINTS ${LLVM_LIBRARY_DIR} REQUIRED)
find_library(CLANGPP clang-cpp HINTS ${LLVM_LIBRARY_DIR} REQUIRED)
find_library(LLVM LLVM HINTS ${LLVM_LIBRARY_DIR} REQUIRED)

add_executable(brainf brainf.cpp)
target_compile_features(brainf PRIVATE cxx_std_20)
target_include_directories(brainf PRIVATE 
  ../ext/m4c0-stl/core/inc
  ../ext/m4c0-stl/parser/inc
  inc
  ${LLVM_INCLUDE_DIR}
  )
target_link_libraries(brainf PRIVATE ${CLANG} ${CLANGPP} ${LLVM})

add_executable(handcrafted handcrafted.cpp)
target_compile_features(handcrafted PRIVATE cxx_std_20)
target_include_directories(handcrafted PRIVATE ${LLVM_INCLUDE_DIR})
target_link_libraries(handcrafted PRIVATE ${CLANG} ${CLANGPP} ${LLVM})

