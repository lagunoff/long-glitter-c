{}:
let
  nixpkgs = import (builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/dd2727773a23d5aac1f084f0b0891bf5b797199d.tar.gz";
  }) {};

  long-glitter-cpp = nixpkgs.stdenv.mkDerivation {
    name = "long-glitter-cpp";
    nativeBuildInputs = [
      nixpkgs.gdb
    ];
    buildInputs = [
      nixpkgs.SDL2
      nixpkgs.SDL2_ttf
      nixpkgs.lightning
    ];
  };
in { shell = long-glitter-cpp; }
