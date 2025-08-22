{
  description = "ak library";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = { self, nixpkgs }:
  let
    system = "x86_64-linux";

    akOverlay = self: super: {
      llvmPackages = super.llvmPackages_latest;
      clang-tools = super.clang-tools.override {
        enableLibcxx = true;
      };
      libcxx = super.llvmPackages.libcxx;
     
      
    };

    pkgs = import nixpkgs {
      inherit system;
      overlays = [ akOverlay ];
    };

    # Setup custom gtest/gbenchmark
    cmakeFlagsArray = ''cmakeFlagsArray+=(-DCMAKE_CXX_FLAGS="-fno-exceptions -fno-rtti")'';

    ak_gtest = pkgs.gtest.overrideAttrs (old: {
      stdenv = pkgs.llvmPackages.stdenv;
      preConfigure = cmakeFlagsArray;
    });

    ak_gbenchmark = pkgs.gbenchmark.overrideAttrs (old: {
      stdenv = pkgs.llvmPackages.stdenv;
      cmakeFlags = (old.cmakeFlags or []) ++ [
        "-DBENCHMARK_ENABLE_EXCEPTIONS=OFF"
        "-DBENCHMARK_ENABLE_TESTING=OFF"
        "-DBENCHMARK_ENABLE_GTEST_TESTS=OFF"
      ];
      preConfigure = cmakeFlagsArray;
    });

  in  

  {

    packages.x86_64-linux = {
      libak = pkgs.llvmPackages.stdenv.mkDerivation {
        pname = "libak";
        description = "ak library";
        version = "0.0.1";
        src = ./libak;

        outputs = [ "out" "dev" ];

        nativeBuildInputs = with pkgs; [
          clang 
          clang-tools          
          liburing.dev
          valgrind 
          ak_gtest
          ak_gbenchmark
        ];          

        buildInputs = with pkgs; [
          liburing
          llvmPackages.libcxx 
        ];

        buildPhase = ''
          make CONFIG=release lib
        '';

        checkPhase = ''
          make test
        '';

        installPhase = ''
          mkdir -p $out/lib
          cp build/libak.so $out/lib/

          mkdir -p $dev/lib
          cp build/libak.a $dev/lib/

          mkdir -p $dev/include
          for file in $(find src -name '*_api.hpp' -o -name '*_api_inl.hpp' -o -name 'ak.hpp'); do
            install -D -m644 "$file" "$dev/include/$file"
          done
        ''; 
      };

      libak-examples-echo = pkgs.llvmPackages.stdenv.mkDerivation {
        name = "libak-examples-echo";
        version = "0.0.1";
        src = ./libak/examples/echo;

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
          stdenv = pkgs.llvmPackages.stdenv;
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
              python3
              bear
              gnumake
              clang 
              clang-tools
              ccache
            ] ++ [ak_gtest ak_gbenchmark];
            
            buildInputs = with pkgs; [
              liburing
              llvmPackages.libcxx 
            ];

            shellHook = ''
              export PROJECT_ROOT=$(git rev-parse --show-toplevel)
              export LIBAK_ROOT="$PROJECT_ROOT/libak"
              export TERM=xterm-256color
              export COMPILER="clang++"
              export CC="clang++"
              export CXX="clang++"
              export PS1='\[\033[1;33m\](libak)\[\033[0m\] \[\033[1;32m\][\w]\[\033[0m\] $ '
              echo "liburing inc   : ${pkgs.liburing.dev}" 
              echo "liburing lib   : ${pkgs.liburing}" 
              echo "C++ compiler   : ${pkgs.clang}"
              echo "libcxx path    : ${pkgs.llvmPackages.libcxx}"
              echo "clangd path    : ${pkgs.clang-tools}/bin/clangd"
              echo "gtest inc      : ${pkgs.gtest.dev}/include"
              echo "gtest lib      : ${pkgs.gtest}/lib"
              echo "gbenchmark inc : ${pkgs.gbenchmark}/include"
              echo "gbenchmark lib : ${pkgs.gbenchmark}/lib"
              export PKG_CONFIG_PATH="${pkgs.gtest.dev}/lib/pkgconfig:$PKG_CONFIG_PATH"
              export PKG_CONFIG_PATH="${pkgs.gbenchmark}/lib/pkgconfig:$PKG_CONFIG_PATH"
              export PROJECT_ROOT=$(git rev-parse --show-toplevel)
              cd $PROJECT_ROOT/libak
              pwd
            '';
          };
        };
  };
}
