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
              pkg-config
              glfw
              libGL.dev
              catch2_3
              julia
            ]
            ++ [raylib_patched llvm.lld];
          buildPhase = ''
            make manalter
          '';
          installPhase = ''
            mkdir -p $out/bin
            cp src/manalter $out/bin/manalter
          '';
        };
      in {
        packages.default = manalter;
        apps = {
          default = flake-utils.lib.mkApp {
            drv = manalter;
            exePath = "/bin/manalter";
          };
        };
        devShell = pkgs.mkShell {
          nativeBuildInputs = with pkgs;
            [
              glfw
              libGLU
              catch2_3

              pkg-config
              cmake
              gdb
              renderdoc
              valgrind
              shaderc

              sdl3
              sdl3-image
              ((imgui.override {
                IMGUI_BUILD_SDL3_BINDING = true;
                IMGUI_BUILD_SDLGPU3_BINDING = true;
                IMGUI_BUILD_VULKAN_BINDING = true;
              }).overrideAttrs (oldAttrs: {
                version = "1.91.8";
                src = fetchFromGitHub {
                  owner = "ocornut";
                  repo = "imgui";
                  tag = "v1.91.8";
                  hash = "sha256-+BuSAXvLvOYOmENzxd1pGDE6llWhTGVu7C3RnoVLVzg=";
                };
              }))
              glm
              xxHash
              vulkan-volk
              vk-bootstrap
              vulkan-headers
              vulkan-loader
              vulkan-memory-allocator
              vulkan-utility-libraries
              vulkan-validation-layers

              julia

              xorg.libX11
              xorg.libXi
              xorg.libXcursor
              xorg.libXrandr
              xorg.libXinerama
            ]
            ++ [raylib_patched pleaseKeepMyInputs llvm.clang-tools llvm.clang llvm.lld];
          shellHook = ''
            export CC=clang
            export CXX=clang++

            # needed for stupid fucking renderdoc to work
            export LD_LIBRARY_PATH="${pkgs.xorg.libX11}/lib:${pkgs.xorg.libXext}/lib:${pkgs.xorg.libXi}/lib:${pkgs.xorg.libXrandr}/lib:${pkgs.vulkan-loader}/lib"
          '';
        };
      }
    );
}
