cmake_minimum_required(VERSION 3.20)
project(clang_tidy_security_checks LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
add_library(SecurityMiscPlugin MODULE
  module/SecurityMiscModule.cpp
  checks/TrustBoundaryPaddingLeakCheck.cpp
)
set_target_properties(SecurityMiscPlugin PROPERTIES
  OUTPUT_NAME "SecurityMiscPlugin"
  PREFIX ""
  LINK_FLAGS "-Wl,-z,nodelete"
)
target_compile_options(SecurityMiscPlugin PRIVATE -fno-rtti)
