cmake_minimum_required(VERSION 3.16)
project(jam
  VERSION 0.1.0
  LANGUAGES C
)

if(EMSCRIPTEN)
  set(SDL3_DIR "$ENV{SDL3_EMSCRIPTEN_DIR}" CACHE PATH
    "Path to the Emscripten SDL3 CMake package"
  )

  if(NOT IS_DIRECTORY "${SDL3_DIR}")
    message(FATAL_ERROR
      "SDL3_DIR must point to an SDL3 package built for Emscripten. "
      "Enter the Nix shell or pass -DSDL3_DIR=/path/to/lib/cmake/SDL3."
    )
  endif()

  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

find_package(SDL3 REQUIRED)

add_executable(jam
  src/main.c
  src/utils.c
  src/level.c
  src/board.c
  src/entity.c
  src/pipe.c
)
target_link_libraries(jam PRIVATE SDL3::SDL3)
