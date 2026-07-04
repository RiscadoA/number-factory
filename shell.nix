{ pkgs ? import <nixpkgs> { } }:

let
  sdl3Emscripten = pkgs.stdenvNoCC.mkDerivation {
    pname = "sdl3-emscripten";
    inherit (pkgs.sdl3) version src;

    nativeBuildInputs = with pkgs; [
      cmake
      emscripten
      ninja
    ];

    configurePhase = ''
      runHook preConfigure

      export HOME="$TMPDIR"
      export EM_CACHE="$TMPDIR/emscripten-cache"

      emcmake cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$out" \
        -DSDL_SHARED=OFF \
        -DSDL_STATIC=ON \
        -DSDL_TESTS=OFF \
        -DSDL_TEST_LIBRARY=OFF

      runHook postConfigure
    '';

    buildPhase = ''
      runHook preBuild
      cmake --build build --parallel "$NIX_BUILD_CORES"
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      cmake --install build
      runHook postInstall
    '';

    dontStrip = true;
  };

  sdl3TtfEmscripten = pkgs.stdenvNoCC.mkDerivation {
    pname = "sdl3-ttf-emscripten";
    inherit (pkgs.sdl3-ttf) version src;

    nativeBuildInputs = with pkgs; [
      cmake
      emscripten
      ninja
    ];

    postUnpack = ''
      mkdir -p "$sourceRoot/external/freetype"
      tar -xf ${pkgs.freetype.src} --strip-components=1 \
        -C "$sourceRoot/external/freetype"
    '';

    configurePhase = ''
      runHook preConfigure

      export HOME="$TMPDIR"
      export EM_CACHE="$TMPDIR/emscripten-cache"

      emcmake cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$out" \
        -DSDL3_DIR="${sdl3Emscripten}/lib/cmake/SDL3" \
        -DBUILD_SHARED_LIBS=OFF \
        -DSDLTTF_VENDORED=ON \
        -DSDLTTF_HARFBUZZ=OFF \
        -DSDLTTF_PLUTOSVG=OFF \
        -DSDLTTF_SAMPLES=OFF \
        -DSDLTTF_INSTALL=ON

      runHook postConfigure
    '';

    buildPhase = ''
      runHook preBuild
      cmake --build build --parallel "$NIX_BUILD_CORES"
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      cmake --install build
      runHook postInstall
    '';

    dontStrip = true;
  };
in
pkgs.mkShell {
  packages = with pkgs; [
    clang-tools
    cmake
    emscripten
    just
    pkg-config
    sdl3
    sdl3-ttf
  ];

  SDL3_EMSCRIPTEN_DIR="${sdl3Emscripten}/lib/cmake/SDL3";
  SDL3_TTF_EMSCRIPTEN_DIR="${sdl3TtfEmscripten}/lib/cmake/SDL3_ttf";
}
