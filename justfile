default:
    @just --list

build:
    cmake -S . -B build/native
    cmake --build build/native

run: build
    ./build/native/jam

web:
    emcmake cmake -S . -B build/web
    cmake --build build/web

serve: web
    emrun build/web/jam.html
