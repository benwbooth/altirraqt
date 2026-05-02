{
  description = "Cross-platform Qt port workspace for Altirra";

  inputs = {
    nixpkgs.url = "nixpkgs";
  };

  outputs = { self, nixpkgs }:
    let
      lib = nixpkgs.lib;
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forAllSystems = f:
        lib.genAttrs systems (system:
          f (import nixpkgs { inherit system; }));

      # MADS — Tomasz Biela's 6502/65816 cross-assembler. Used to build
      # Avery Lee's BSD-licensed Atari kernel/BASIC ROMs from source.
      madsFor = pkgs: pkgs.stdenv.mkDerivation {
        pname = "mads";
        version = "2.1.8";
        src = pkgs.fetchFromGitHub {
          owner = "tebe6502";
          repo = "Mad-Assembler";
          rev = "0151c6abaf38d9fc62dcb19ffa156f926ec7a64f";
          hash = "sha256-UP/KxK3DOYLt/G36dM3pby3SDoaPUL/MzhmeMGefy/M=";
        };
        nativeBuildInputs = [ pkgs.fpc ];
        dontConfigure = true;
        buildPhase = ''
          fpc -MDelphi -O3 mads.pas
        '';
        installPhase = ''
          install -Dm755 mads $out/bin/mads
        '';
        meta = with pkgs.lib; {
          description = "Mad-Assembler — 6502/65816 cross-assembler used by Altirra";
          homepage = "https://github.com/tebe6502/Mad-Assembler";
          license = licenses.bsd2;
          mainProgram = "mads";
        };
      };
      cleanSource = lib.cleanSourceWith {
        src = ./.;
        filter = path: type:
          let
            relPath = lib.removePrefix "${toString ./.}/" (toString path);
            baseName = builtins.baseNameOf (toString path);
          in
            !(relPath == "build"
              || lib.hasPrefix "build/" relPath
              || relPath == ".direnv"
              || lib.hasPrefix ".direnv/" relPath
              || relPath == "result"
              || lib.hasPrefix "cmake-build-" baseName
              || relPath == "src/.vs"
              || lib.hasPrefix "src/.vs/" relPath);
      };
    in {
      packages = forAllSystems (pkgs:
        let
          qtRuntimeInputs = with pkgs; [
            qt6.qtbase
            qt6.qtmultimedia
            zlib
          ] ++ lib.optionals pkgs.stdenv.isLinux [
            qt6.qtwayland
          ];
        in {
          default = pkgs.stdenv.mkDerivation {
            pname = "altirraqt";
            version = "0.1.0";
            src = cleanSource;

            nativeBuildInputs = with pkgs; [
              cmake
              ninja
              pkg-config
              qt6.wrapQtAppsHook
            ];

            buildInputs = qtRuntimeInputs;

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
            ];

            meta = with pkgs.lib; {
              description = "Qt front-end scaffold for a cross-platform Altirra port";
              license = licenses.gpl2Plus;
              mainProgram = "altirraqt";
              platforms = platforms.linux ++ platforms.darwin;
            };
          };
        });

      devShells = forAllSystems (pkgs:
        let
          qtRuntimeInputs = with pkgs; [
            qt6.qtbase
            qt6.qtmultimedia
            zlib
          ] ++ lib.optionals pkgs.stdenv.isLinux [
            qt6.qtwayland
          ];

          devTools = with pkgs; [
            cmake
            ninja
            pkg-config
            git
            clang-tools
            llvmPackages.clang
            llvmPackages.libcxx
            (madsFor pkgs)
          ] ++ lib.optionals pkgs.stdenv.isLinux [
            gdb
          ] ++ lib.optionals pkgs.stdenv.isDarwin [
            lldb
          ];
        in {
          default = pkgs.mkShell {
            packages = devTools ++ qtRuntimeInputs;

            FLAKE_INPUTS = builtins.concatStringsSep ":" [ "${nixpkgs}" ];

            shellHook = ''
              export CMAKE_GENERATOR=Ninja
              export CMAKE_EXPORT_COMPILE_COMMANDS=ON
              export CC=clang
              export CXX=clang++
              ${lib.optionalString pkgs.stdenv.isLinux ''
                export QT_QPA_PLATFORM="wayland;xcb"
              ''}

              echo "altirraqt dev shell ready"
              echo "Configure: cmake -S . -B build"
              echo "Build:     cmake --build build"
            '';
          };
        });

      formatter = forAllSystems (pkgs: pkgs.nixfmt-rfc-style);
    };
}
