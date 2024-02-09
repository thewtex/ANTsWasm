cmake_minimum_required(VERSION 3.16.3)

include(FetchContent)
FetchContent_Declare(
  ants_inner
  GIT_REPOSITORY https://github.com/ANTsX/ANTs
  GIT_TAG        v2.5.1  # 2024-01-24
)
FetchContent_GetProperties(ants_inner)
if(NOT ants_inner_POPULATED)
  FetchContent_Populate(ants_inner)
endif()

project(ANTsWasm)

if(NOT ITK_SOURCE_DIR)
  find_package(ITK REQUIRED)
endif()

set(ANTsWasm_INCLUDE_DIRS
  ${ANTsWasm_SOURCE_DIR}/include
  ${ants_inner_SOURCE_DIR}/Utilities
  ${ants_inner_SOURCE_DIR}/ImageRegistration
  ${ants_inner_SOURCE_DIR}/Tensor
  ${ants_inner_SOURCE_DIR}/Temporary
  ${ants_inner_SOURCE_DIR}/Examples
  ${ants_inner_SOURCE_DIR}/Examples/include
)

if(NOT ITK_SOURCE_DIR)
  find_package(ITK 5.4 REQUIRED)  # we need at least 71c5c83 from 2024-02-02 for debug mode
  list(APPEND CMAKE_MODULE_PATH ${ITK_CMAKE_DIR})
  include(ITKModuleExternal)
else()
  set(ITK_DIR ${CMAKE_BINARY_DIR})
  itk_module_impl()
endif()
