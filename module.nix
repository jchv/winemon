{
  lib,
  config,
  pkgs,
  ...
}:
{
  options = {
    programs.wineman = {
      enable = lib.mkEnableOption "Wineman monitor program";
    };
  };

  config = lib.mkIf config.programs.wineman.enable {
    nixpkgs.overlays = [ (import ./overlay.nix) ];

    environment.systemPackages = [ pkgs.wineman ];
  };
}
