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

add_executable(jam
  src/main.c
  src/game_state.c
  src/start_menu_state.c
  src/gameplay_state.c
  src/pause_state.c
  src/utils.c
  src/level.c
  src/board.c
  src/entity.c
  src/pipe.c
  src/input.c
  src/output.c
  src/splitter.c
  src/item.c
)

find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)
target_link_libraries(jam PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(jam PRIVATE ${MATH_LIBRARY})
endif()

if(EMSCRIPTEN)
  set_target_properties(jam PROPERTIES OUTPUT_NAME "index")
  set_property(TARGET jam APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
  )
  target_link_options(jam PRIVATE
    "--shell-file"
    "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
    "--preload-file"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets"
    "-sALLOW_MEMORY_GROWTH=1"
  )
endif()
