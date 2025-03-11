{
  stdenv,
  cmake,
  ninja,
  pkg-config,
  qt6Packages,
}:
stdenv.mkDerivation {
  pname = "winemon";
  version = "1.0.0";
  src = ./.;
  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
    qt6Packages.wrapQtAppsHook
  ];
  buildInputs = [
    qt6Packages.qtbase
  ];
  cmakeFlags = [
    "-GNinja"
  ];
  meta = {
    mainProgram = "winemon";
  };
}
