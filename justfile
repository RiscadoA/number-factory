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
    emrun build/web/index.html

itch: web
    cd build/web && cmake -E tar cf ../number-factory-itch.zip --format=zip index.html index.js index.wasm index.data
