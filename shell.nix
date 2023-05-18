{ pkgs ? import <nixpkgs> {} }:
let
  avr-gcc = pkgs.pkgsCross.avr.buildPackages.gcc;
in
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    avr-gcc
    avrdude
    clang-tools
    bear
    jq
    picocom
    python3
  ];
}
