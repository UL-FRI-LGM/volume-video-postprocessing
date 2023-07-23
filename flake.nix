{
  description = "Environment for developing volumes with blender";

  # Flake inputs
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs"; # also valid: "nixpkgs"
  };

  # Flake outputs
  outputs = { self, nixpkgs }:
    let
      # Systems supported
      allSystems = [
        "x86_64-linux" # 64-bit Intel/AMD Linux
        # "aarch64-linux" # 64-bit ARM Linux
        # "x86_64-darwin" # 64-bit Intel macOS
        # "aarch64-darwin" # 64-bit ARM macOS
      ];

      # Helper to provide system-specific attributes
      forAllSystems = f: nixpkgs.lib.genAttrs allSystems (system: f {
        pkgs = import nixpkgs { inherit system; };
      });
    in
    {
      # Development environment output
      devShells = forAllSystems ({ pkgs }: {
        default =
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
              pkgs.fish
              pkgs.nodejs
              pkgs.nodePackages.vscode-langservers-extracted
              pkgs.nodePackages.typescript-language-server
              # (python.withPackages (ps: with ps; [
              #   virtualenv
              #   pip
              #   python-lsp-server
              # ]))
            ];
            shellHook = ''
              fish
            '';
          };
      });
      packages = forAllSystems ({ pkgs }: {
        default =
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
      });
    };
}