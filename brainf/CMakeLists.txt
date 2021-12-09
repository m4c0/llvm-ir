cmake_minimum_required(VERSION 3.18)
project(brainf LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LLVM_DIR "/usr" CACHE PATH "Path to LLVM installation")
set(LLVM_INCLUDE_DIR "${LLVM_DIR}/include" CACHE PATH "Path to LLVM includes")
set(LLVM_LIBRARY_DIR "${LLVM_DIR}/lib" CACHE PATH "Path to LLVM libraries")

find_library(CLANG clang HINTS ${LLVM_LIBRARY_DIR} REQUIRED)
find_library(CLANGPP clang-cpp HINTS ${LLVM_LIBRARY_DIR} REQUIRED)
find_library(LLVM LLVM HINTS ${LLVM_LIBRARY_DIR} REQUIRED)

function(exe NAME)
  add_executable(${NAME} ${NAME}.cpp)
  target_compile_features(${NAME} PRIVATE cxx_std_20)
  target_include_directories(${NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LLVM_INCLUDE_DIR}
    )
  target_link_libraries(${NAME} PRIVATE ${CLANG} ${CLANGPP} ${LLVM})
endfunction()

exe(brainf)
exe(handcrafted)
