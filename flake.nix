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
          # gtest.dev
          gtest
          gbenchmark
        ];          

        buildInputs = with pkgs; [
          liburing
          llvmPackages.libcxx 
        ];

        outputs  = [ "out" "doc" ];

        buildPhase = ''
          make doxygen
        '';

        checkPhase = ''
          make test
        '';

        preCheck = ''
          export GTEST_INCLUDE="${pkgs.gtest.dev}/include"
          export GTEST_LIB="${pkgs.gtest}/lib"
        '';

        installPhase = ''
          mkdir -p $out/include
          cp ./src/ak.hpp $out/include/ak.hpp
          
          mkdir -p $doc/share/doc/api
          cp -R ./build/doc $doc/share/doc/api
          
        ''; 
      };

      libak-examples-echo = pkgs.llvmPackages.stdenv.mkDerivation {
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

      # Allow building only these vendored deps with our overlayed flags
      

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
            ] ++ [ak_gtest ak_gbenchmark];
            
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
              echo "clangd path  : ${pkgs.clang-tools}/bin/clangd"
              echo "gtest inc    : ${pkgs.gtest.dev}/include"
              echo "gtest lib    : ${pkgs.gtest}/lib"
              echo "gbenchmark inc    : ${pkgs.gbenchmark}/include"
              echo "gbenchmark lib    : ${pkgs.gbenchmark}/lib"
              export PKG_CONFIG_PATH="${pkgs.gtest.dev}/lib/pkgconfig:$PKG_CONFIG_PATH"
              export PKG_CONFIG_PATH="${pkgs.gbenchmark}/lib/pkgconfig:$PKG_CONFIG_PATH"
            '';
          };
        };
  };
}
