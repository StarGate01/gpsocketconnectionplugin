{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      pkgs = nixpkgs.legacyPackages.x86_64-linux;

      pyglobalplatform = (pkgs.python3Packages.buildPythonPackage {
        pname = "pyglobalplatform";
        version = "1.0.0";

        src = pkgs.fetchFromGitHub {
          owner = "StarGate01";
          repo = "pyglobalplatform";
          rev = "master";
          sha256 = "sha256-XGSpRsrqHZL4266dbhD0GihSx8qQ9x5uJdy3JFgq/BE=";
        };

        pyproject = true;

        nativeBuildInputs = with pkgs; [
          pkg-config
          swig
        ];

        buildInputs = with pkgs; [
          globalplatform
          pcsclite
        ];

        propagatedBuildInputs = with pkgs.python3Packages; [
          setuptools
        ];

        pythonImportsCheck = [ "globalplatform" ];
      });
    in
    {
      devShell.x86_64-linux =
        pkgs.mkShell {
          shellHook = ''
            export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(pwd)/install/lib"
          '';

          buildInputs = with pkgs; [
            gdb
            gcc
            pkg-config
            cmake
            gnumake
            globalplatform
            pcsclite
            (python3.withPackages (ps: with ps; [
              pyscard
              pyglobalplatform
            ]))
          ];
        };
    };
}
