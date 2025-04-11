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
        llvm = pkgs.llvmPackages_20;
        pleaseKeepMyInputs = pkgs.writeTextDir "bin/.inputs" (builtins.concatStringsSep " " (builtins.attrValues {inherit nixpkgs;}));
        raylib_patched = pkgs.raylib.overrideAttrs (oldAttrs: {
          patches = (oldAttrs.patches or []) ++ [./raylib.patch];
          cmakeFlags = oldAttrs.cmakeFlags ++ ["-DGRAPHICS=GRAPHICS_API_OPENGL_43"];
        });
        manalter = llvm.stdenv.mkDerivation {
          pname = "Manalter";
          version = "0.1.0";
          src = ./.;
          buildInputs = with pkgs;
            [
              cmake
              lld
              glfw
              libGL.dev
              catch2_3
            ]
            ++ [raylib_patched];
          buildPhase = ''
            make manalter
          '';
          installPhase = ''
            mkdir -p $out/bin
            cp src/manalter $out/bin/manalter
          '';
        };
        hitbox-demo = llvm.stdenv.mkDerivation {
          pname = "Hitbox demo";
          version = "0.1.0";
          src = ./.;
          buildInputs = with pkgs;
            [
              cmake
              lld
              glfw
              libGL.dev
              catch2_3
            ]
            ++ [raylib_patched];
          buildPhase = ''
            make hitbox_demo
          '';
          installPhase = ''
            mkdir -p $out/bin cp src/hitbox_demo $out/bin/hitbox_demo
          '';
        };
      in {
        packages.default = manalter;
        apps = {
          hitbox-demo = flake-utils.lib.mkApp {
            drv = hitbox-demo;
            exePath = "/bin/hitbox_demo";
          };
          default = flake-utils.lib.mkApp {
            drv = manalter;
            exePath = "/bin/manalter";
          };
        };
        devShell = pkgs.mkShell {
          nativeBuildInputs = with pkgs;
            [
              cmake
              lld
              glfw
              libGLU
              gdb
              catch2_3
              renderdoc

              xorg.libX11
              xorg.libXi
              xorg.libXcursor
              xorg.libXrandr
              xorg.libXinerama
            ]
            ++ [raylib_patched pleaseKeepMyInputs llvm.clang-tools llvm.clang];
          shellHook = ''
            export CC=clang
            export CXX=clang++
          '';
        };
      }
    );
}
