{
  description = "ak library";

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
      libak = pkgs.stdenv.mkDerivation {
        name = "libak";
        version = "0.0.1";
        srcs = [./.];

        nativeBuildInputs = with pkgs; [
          clang 
          clang-tools          
          liburing.dev
          doxygen
          valgrind 
          graphviz
        ];          

        buildInputs = with pkgs; [
          liburing
          llvmPackages.libcxx 
        ];

        buildPhase = ''
          make doxygen
        '';

        checkPhase = ''
          make test
        '';

        installPhase = ''
          mkdir -p $out/share/doc/api
          mkdir -p $out/include
          cp ./src/ak.hpp $out/include/ak.hpp
          cp -R ./build/doc $out/share/doc/api
        ''; 
      };

      libak-examples-echo = pkgs.stdenv.mkDerivation {
        name = "libak-examples-echo";
        version = "0.0.1";
        srcs = [./examples/echo];

        nativeBuildInputs = with pkgs; [
          clang 
          clang-tools          
          liburing.dev
          doxygen
          valgrind 
          graphviz
        ];          

        buildInputs = with pkgs; [
          self.packages.x86_64-linux.libak
        ];

        buildPhase = ''
          ./compile.sh
        '';

        checkPhase = ''
          
        '';

        installPhase = ''
          mkdir -p $out/bin
          cp ./build/echo-client $out/bin/echo-client
          cp ./build/echo-server $out/bin/echo-server
        ''; 
      };

      default = self.packages.x86_64-linux.libak;
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
              doxygen
              valgrind 
              graphviz
              ragel
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
