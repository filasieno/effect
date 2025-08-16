# Wrapper to the default flake devShell for Linux
let
  flake = builtins.getFlake (toString ./.);
in
  flake.devShells.x86_64-linux.default
