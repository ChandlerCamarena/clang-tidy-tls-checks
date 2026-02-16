cmake_minimum_required(VERSION 3.20)
project(clang_tidy_tls_checks LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prefer LLVM 18 config packages if installed under /usr/lib/llvm-18
list(APPEND CMAKE_PREFIX_PATH "/usr/lib/llvm-18/lib/cmake")

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_library(TlsTidyModule MODULE
  module/TlsChecksModule.cpp
  checks/DisableVerifyCheck.cpp
  checks/InsecureProtocolMethodCheck.cpp
)

# Make sure the module has a predictable output name.
set_target_properties(TlsTidyModule PROPERTIES
  OUTPUT_NAME "TlsTidyModule"
)

# Clang-Tidy is built from a set of Clang libraries.
# On Ubuntu packages, these are typically available by name.
target_link_libraries(TlsTidyModule PRIVATE
  clangTidy
  clangTidyUtils
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangLex
  clangTooling
)

# This avoids "lib" prefix differences in some environments; harmless otherwise.
set_target_properties(TlsTidyModule PROPERTIES PREFIX "")

