{
  description = "Flake for the PiiXel Engine";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      nixpkgs,
      flake-utils,
      ...
    }:

    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        # packages = {};

        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            cmake

            raylib
            entt
            nlohmann_json
            imgui
            box2d

            libGL
            libxcursor
            libxrandr
            xorg.libXi
            xorg.libXinerama
          ];

          shellHook = ''
            # export C_INCLUDE_PATH="${pkgs.raylib}/include:$C_INCLUDE_PATH"

            engineClangdConfig="$PWD/engine/.clangd"
            echo "Created $engineClangdConfig"
            echo -en "CompileFlags:\n  Add: [ -Wall, -Wextra, -I$PWD/engine/include, -I${pkgs.raylib}/include ]" > "$engineClangdConfig"

            gameClangdConfig="$PWD/games/MyFirstGame/.clangd"
            echo "Created $gameClangdConfig"
            echo -en "CompileFlags:\n  Add: [ -Wall, -Wextra, -I$PWD/engine/include, -I$PWD/games/MyFirstGame/include ]" > "$gameClangdConfig"
          '';
        };
      }
    );

}
