{
  inputs = {
    nixpkgs.url = "github:StarGate01/nixpkgs/gpshell-python";
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
              (pkgs.python3Packages.pyglobalplatform.overrideAttrs (oldAttrs: {
                src = pkgs.fetchFromGitHub {
                  owner = "StarGate01";
                  repo = "pyglobalplatform";
                  rev = "master";
                  sha256 = "sha256-mJG1oLU62aP2tBzvYIDQxtKpc/nkFNQAhuuDI1UB5Yo=";
                };
              }))
            ]))
          ];
        };
    };
}
