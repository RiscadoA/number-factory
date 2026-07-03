{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  packages = with pkgs; [
    cmake
    pkg-config
    clang-tools
    sdl3
  ];
}
