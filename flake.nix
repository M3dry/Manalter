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
        raylib_patched =
          pkgs.raylib.overrideAttrs (oldAttrs: rec {
              patches = oldAttrs.patches ++ [ ./raylib.patch ];
          });
        aetas_magus = pkgs.gcc14Stdenv.mkDerivation {
          pname = "Aetas Magus";
          version = "0.1.0";
          src = ./.;
          buildInputs = with pkgs; [
            raylib
            cmake
          ];
          buildPhase = ''
            make aetas_magus
          '';
          installPhase = ''
            mkdir -p $out/bin
            cp src/aetas_magus $out/bin/Aetas\ Magus
          '';
        };
      in rec {
        packages.default = aetas_magus;
        apps.default = flake-utils.lib.mkApp {
          drv = aetas_magus;
          exePath = "/bin/Aetas\ Magus";
        };
        devShell = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            raylib
            cmake
            gcc14
          ];
        };
      }
    );
}
