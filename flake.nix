{
  description = "Flake for making and developing volumetric videos";

  # Flake inputs
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs"; # also valid: "nixpkgs"
    utils.url = "github:numtide/flake-utils";
  };

  # Flake outputs
  outputs = { self, nixpkgs, utils }:
    utils.lib.eachSystem [
      "x86_64-linux" # 64-bit Intel/AMD Linux
      "aarch64-linux" # 64-bit ARM Linux
      # "x86_64-darwin" # 64-bit Intel macOS
      # "aarch64-darwin" # 64-bit ARM macOS
    ] (system:
      let
        # Helper to provide system-specific attributes
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        # Development environment output
        devShells.default =
          # let
          #   python = pkgs.python310;
          # in
          pkgs.mkShell {
            # The Nix packages provided in the environment
            packages = with pkgs; [
              lldb
              clang-tools
              openvdb
              boost
              tbb
              blender
              fish
              nodejs
              nodePackages.vscode-langservers-extracted
              nodePackages.typescript-language-server
              inotify-tools
              fiji
              # (python.withPackages (ps: with ps; [
              #   virtualenv
              #   pip
              #   python-lsp-server
              # ]))
            ];
            shellHook = ''
              exec fish
            '';
          };
      
        packages.vdb2raw =
          let
            binName = "combiner";
            cppDependencies = with pkgs; [ gcc openvdb tbb boost ];
          in
          pkgs.stdenv.mkDerivation {
            name = "combiner";
            src = self;
            buildInputs = cppDependencies;
            buildPhase = "c++ -std=c++17 -o ${binName} ${./main.cpp} -lboost_system -lopenvdb -ltbb";
            installPhase = ''
              mkdir -p $out/bin
              cp ${binName} $out/bin/
            '';
        };
        

        packages.default = pkgs.stdenv.mkDerivation {
          name = "make-video";
          src = ./.;
          nativeBuildInputs = [
            pkgs.makeBinaryWrapper
          ];
          buildInputs = with pkgs; [
            blender
            nodejs
            self.packages.${system}.vdb2raw
          ];
          installPhase = ''
            mkdir -p $out/bin
            cp blender_scene2vdb.py $out/bin
            cp raw2bvp.js $out/bin
            cp make-video $out/bin
            wrapProgram $out/bin/make-video \
              --prefix PATH : ${pkgs.lib.makeBinPath [ pkgs.blender pkgs.nodejs self.packages.${system}.vdb2raw ]}
          '';
        };
    });
}