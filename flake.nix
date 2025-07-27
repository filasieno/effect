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
        srcs = [./.];

        nativeBuildInputs = with pkgs; [
          clang 
          clang-tools          
          liburing.dev
          valgrind
        ];          

        buildInputs = with pkgs; [
          liburing
          llvmPackages.libcxx 
        ];

        buildPhase = ''
          make
        '';

        installPhase = ''
          mkdir -p $out
          cp ./build/io $out/io
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
              liburing.dev            
              inotify-tools       
            ];
            
            buildInputs = with pkgs; [
              liburing
              llvmPackages.libcxx 
              clang 
              clang-tools
            ];

            shellHook = ''
              export COMPILER="clang++"
              export CC="clang++"
              export CXX="clang++"
              export PS1='$ '
              echo "liburing inc : ${pkgs.liburing.dev}" 
              echo "liburing lib : ${pkgs.liburing}" 
              echo "C++ compiler : ${pkgs.clang}"
              echo "libcxx path  : ${pkgs.llvmPackages.libcxx}"
            '';
          };
        };
    };
}
