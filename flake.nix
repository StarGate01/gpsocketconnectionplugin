{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      pkgs = nixpkgs.legacyPackages.x86_64-linux;
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
              (buildPythonPackage {
                pname = "pyglobalplatform";
                version = "1.0.0";

                src = fetchFromGitHub {
                  owner = "StarGate01";
                  repo = "pyglobalplatform";
                  rev = "master";
                  sha256 = "sha256-XGSpRsrqHZL4266dbhD0GihSx8qQ9x5uJdy3JFgq/BE=";
                };

                pyproject = true;

                nativeBuildInputs = [
                  pkg-config
                  swig
                ];

                buildInputs = [
                  globalplatform
                  pcsclite
                ];

                propagatedBuildInputs = [
                  setuptools
                ];

                pythonImportsCheck = [ "globalplatform" ];
              })
            ]))
          ];
        };
    };
}
