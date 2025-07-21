{
  description = "Effect library";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = { self, nixpkgs }: 
  
  let 
    system = "x86_64-linux";

    clangOverlay = self: super: {
      llvmPackages = super.llvmPackages_latest;
      clang-tools = super.clang-tools.override {
        enableLibcxx = true;
      };
      libcxx = super.llvmPackages.libcxx;
    };

    pkgs = import nixpkgs { 
      inherit system;
      overlays = [ clangOverlay ];
    };
  in  
  
  {

    packages.x86_64-linux = {
      effect = pkgs.stdenv.mkDerivation {
        name = "effect";
        version = "0.0.1";
        srcs = [./src ];

        nativeBuildInputs = with pkgs; [
          clang 
          clang-tools          
        ];          

        buildInputs = with pkgs; [
          llvmPackages.libcxx 
        ];

        buildPhase = ''
          mkdir -p $out
          clang++ effects.cc -std=c++2c -lc++ -lc++abi -fno-exceptions -fno-rtti -o $out/effect
        '';

        installPhase = ''
          mkdir -p $out
        ''; 
      };
      default = self.packages.x86_64-linux.effect;
    };

    devShells.x86_64-linux =
      let
        mkOverriddenShell = pkgs.mkShell.override {
          # stdenv = pkgs.llvmPackages.stdenv;
        };
      in
      {

      default = mkOverriddenShell
        {
          nativeBuildInputs = with pkgs; [
            lldb            
          ];
          
          buildInputs = with pkgs; [
            llvmPackages.libcxx 
            clang 
            clang-tools
          ];

          shellHook = ''
            export COMPILER="clang++"
            export CC="clang++"
            export CXX="clang++"
            export PS1='$ '
            echo "C++ compiler : ${pkgs.clang}"
            echo "libcxx path  : ${pkgs.llvmPackages.libcxx}"
          '';
        };

    };
  };
}
