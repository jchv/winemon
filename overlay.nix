final: prev:
let
  inherit (final) pkgs;
in
{
  winemon = pkgs.callPackage ./package.nix { };
}
