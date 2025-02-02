{
  description = "Monitors running Wine instances";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };
  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      ...
    }:
    {
      overlays.default = import ./overlay.nix;
      nixosModules.default = import ./module.nix;
    }
    // flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [ self.overlays.default ];
        };
        inherit (pkgs) wineman;
        winemanClangTidy = wineman.overrideAttrs (prev: {
          nativeBuildInputs = (prev.nativeBuildInputs or [ ]) ++ [
            pkgs.clang-tools
          ];
          cmakeFlags = (prev.cmakeFlags or [ ]) ++ [
            "-DUSE_CLANG_TIDY=ON"
          ];
        });
        format = pkgs.writeShellApplication {
          name = "format";

          runtimeInputs = [
            pkgs.clang-tools
            pkgs.cmake-format
            pkgs.nixfmt-rfc-style
            pkgs.yamlfmt
          ];

          text = ''
            if [[ $# -lt 1 ]]; then
              >&2 echo "Usage: $0 --check | --write"
              exit 0
            fi

            NIXFMT_ARGS=()
            CLANG_FORMAT_ARGS=("--verbose")
            CMAKE_FORMAT_ARGS=("--log-level" "debug")
            YAMLFMT_ARGS=()

            case $1 in
              -w|--write)
                NIXFMT_ARGS+=("--verify")
                CLANG_FORMAT_ARGS+=("-i")
                CMAKE_FORMAT_ARGS+=("-i")
                shift
                ;;
              -c|--check)
                NIXFMT_ARGS+=("--check")
                CLANG_FORMAT_ARGS+=("--dry-run" "-Werror")
                CMAKE_FORMAT_ARGS+=("--check")
                YAMLFMT_ARGS+=("-dry" "-lint")
                shift
                ;;
              *)
                >&2 echo "Unknown option $1"
                exit 1
                ;;
            esac

            >&2 echo "Running nixfmt."
            find . -not -path '*/.*' -not -path 'build' -iname '*.nix' -print0 | \
              xargs -0 nixfmt "''${NIXFMT_ARGS[@]}"

            >&2 echo "Running clang-format."
            find . -not -path '*/.*' -not -path 'build' \( -iname '*.h' -o -iname '*.cpp' \) -print0 | \
              xargs -0 clang-format "''${CLANG_FORMAT_ARGS[@]}"

            >&2 echo "Running cmake-format."
            find . -not -path '*/.*' -not -path 'build' \( -iname 'CMakeLists.txt' -o -iname '*.cmake' \) -print0 | \
              xargs -0 cmake-format "''${CMAKE_FORMAT_ARGS[@]}"

            >&2 echo "Running yamlfmt."
            yamlfmt "''${YAMLFMT_ARGS[@]}" '**.yml' .clang-format .clang-tidy
          '';
        };
      in
      {
        packages = {
          inherit wineman winemanClangTidy format;
          default = wineman;
        };

        checks = {
          format = pkgs.runCommandLocal "check-format" { } ''
            cd ${self}
            ${pkgs.lib.getExe format} --check
            touch $out
          '';

          inherit winemanClangTidy;
        };

        devShells.default = pkgs.mkShell {
          nativeBuildInputs = [
            pkgs.cmake-format
            pkgs.clang-tools
            pkgs.qtcreator
          ];
          inputsFrom = [ wineman ];
        };
      }
    );
}
