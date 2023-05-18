{ pkgs ? import <nixpkgs> {} }:
let
  avr-gcc = pkgs.pkgsCross.avr.buildPackages.gcc;
in
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    avr-gcc

    # avr programmer
    avrdude

    # contains the program clangd which is a LSP server for C
    clang-tools

    # tool for generating compile_commands.json needed by clangd
    bear

    # used to add avr include paths to compile_commands.json
    jq

    # serial console
    picocom
  ];
}
