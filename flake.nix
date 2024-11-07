{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    nixpkgs,
    flake-utils,
    ...
  }:
    flake-utils.lib.eachDefaultSystem (
      system: let
        pkgs = nixpkgs.legacyPackages.${system};
        raylib_patched = pkgs.raylib.overrideAttrs (oldAttrs: rec {
          patches = oldAttrs.patches ++ [./raylib.patch];
        });
        manalter = pkgs.gcc14Stdenv.mkDerivation {
          pname = "Manalter";
          version = "0.1.0";
          src = ./.;
          buildInputs = with pkgs;
            [
              cmake
              glfw
              libGL.dev
            ] ++ [raylib_patched];
          buildPhase = ''
            make manalter
          '';
          installPhase = ''
            mkdir -p $out/bin
            cp src/manalter $out/bin/manalter
          '';
        };
      in rec {
        packages.default = manalter;
        apps.default = flake-utils.lib.mkApp {
          drv = manalter;
          exePath = "/bin/manalter";
        };
        devShell = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            cmake
            glfw libGLU
            gcc14
            gdb

            xorg.libX11 xorg.libXi xorg.libXcursor xorg.libXrandr xorg.libXinerama
          ];
        };
      }
    );
}
