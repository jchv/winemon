final: prev:
let
  inherit (final) pkgs;
in
{
  wineman = pkgs.callPackage ./package.nix { };
}
