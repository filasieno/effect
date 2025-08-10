# Bridge non-flake `nix-shell` to the flake's default devShell.
#
# Usage:
#   - `nix-shell` will enter the same environment as `nix develop` for the
#     current system, using the flake's `devShells.<system>.default`.
#
# Requires a Nix version that supports `builtins.getFlake` (Nix 2.4+ with
# experimental features enabled).

let
  flake = builtins.getFlake (toString ./.);
  system = builtins.currentSystem;
in
  flake.devShells.${system}.default


