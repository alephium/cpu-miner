{ lib , stdenv , fetchFromGitHub , libuv }:

stdenv.mkDerivation rec {

  pname = "alephium-cpu-miner";
  version = "0.0.1";

  src = ./.;

  blake3 = fetchFromGitHub {
    owner = "BLAKE3-team";
    repo = "BLAKE3";
    rev = "master";
    sha256 = "0jig5g43qby5z3b2zb0vddr7g1khacrmhgvqwcvfcfl1da7y7cdv";
  };

  buildInputs = [ libuv ];

  phases = [ "unpackPhase" "buildPhase" "installPhase" ];

  buildPhase = ''
    mkdir BLAKE3
    cp -r ${blake3}/c BLAKE3
    make all
  '';

  installPhase = ''
    mkdir -p $out
    cp -r ./bin $out
  '';

  meta = with lib; {
    description = "Alephium CPU miner";
    homepage = "https://github.com/alephium/cpu-miner";
  };
}
