{
  lib,
  config,
  pkgs,
  ...
}:
{
  options = {
    programs.winemon = {
      enable = lib.mkEnableOption "Winemon, Wine instance monitor";
    };
  };

  config = lib.mkIf config.programs.winemon.enable {
    nixpkgs.overlays = [ (import ./overlay.nix) ];

    environment.systemPackages = [ pkgs.winemon ];
  };
}
