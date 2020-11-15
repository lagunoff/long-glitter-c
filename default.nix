{}:
let
  nixpkgs = import (builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/dd2727773a23d5aac1f084f0b0891bf5b797199d.tar.gz";
  }) {};

  long-glitter = nixpkgs.stdenv.mkDerivation {
    name = "long-glitter";
    nativeBuildInputs = [
      nixpkgs.pkg-config
      nixpkgs.ctags
      nixpkgs.gdb
    ];
    buildInputs = [
      nixpkgs.xorg.libX11
      nixpkgs.xorg.libXext
      nixpkgs.xorg.libXcursor
      nixpkgs.xorg.libXrender
      nixpkgs.xorg.libXft
      nixpkgs.wayland
      nixpkgs.wayland-protocols
      nixpkgs.cairo
    ];
  };
in long-glitter
