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
in
pkgs.mkShell {
  packages = with pkgs; [
    clang-tools
    cmake
    emscripten
    just
    pkg-config
    sdl3
  ];

  SDL3_EMSCRIPTEN_DIR="${sdl3Emscripten}/lib/cmake/SDL3";
}
