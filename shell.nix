with import <nixpkgs> {};
let
  cpu-miner = pkgs.callPackage ./cpu-miner.nix {};
in
pkgs.mkShell {
  name = "cpu-miner";
  buildInputs = [ cpu-miner ];
}
