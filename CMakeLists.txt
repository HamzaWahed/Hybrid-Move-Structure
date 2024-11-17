include(FetchContent)

cmake_minimum_required(VERSION 3.0)

project(Hybrid-Move-Structure)

include_directories("include")

# ----------------------------------------
# Add sdsl
# ----------------------------------------
FetchContent_Declare(sdsl GIT_REPOSITORY https://github.com/simongog/sdsl-lite)

FetchContent_GetProperties(sdsl)
if(NOT sdsl_POPULATED)
  FetchContent_Populate(sdsl)

  set(GENERATE_DOC
      OFF
      CACHE BOOL "Do not generate doxygen for sdsl-lite")

  add_subdirectory(${sdsl_SOURCE_DIR} ${sdsl_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")
# compile with address sanitizer add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)

add_executable(test src/LF.cpp)
target_link_libraries(test sdsl)
target_compile_options(test PUBLIC -O2)
