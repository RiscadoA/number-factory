cmake_minimum_required(VERSION 3.16)
project(jam
  VERSION 0.1.0
  LANGUAGES C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(EMSCRIPTEN)
  set(SDL3_DIR "$ENV{SDL3_EMSCRIPTEN_DIR}" CACHE PATH
    "Path to the Emscripten SDL3 CMake package"
  )
  set(SDL3_ttf_DIR "$ENV{SDL3_TTF_EMSCRIPTEN_DIR}" CACHE PATH
    "Path to the Emscripten SDL3_ttf CMake package"
  )

  if(NOT IS_DIRECTORY "${SDL3_DIR}")
    message(FATAL_ERROR
      "SDL3_DIR must point to an SDL3 package built for Emscripten. "
      "Enter the Nix shell or pass -DSDL3_DIR=/path/to/lib/cmake/SDL3."
    )
  endif()
  if(NOT IS_DIRECTORY "${SDL3_ttf_DIR}")
    message(FATAL_ERROR
      "SDL3_ttf_DIR must point to an SDL3_ttf package built for Emscripten. "
      "Enter the Nix shell or pass -DSDL3_ttf_DIR=/path/to/lib/cmake/SDL3_ttf."
    )
  endif()

  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)

add_executable(jam
  src/main.c
  src/utils.c
  src/level.c
  src/board.c
  src/entity.c
  src/pipe.c
)
target_link_libraries(jam PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)

if(EMSCRIPTEN)
  target_link_options(jam PRIVATE
    "--preload-file"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets"
  )
endif()
