{
  description = "A grayscale to normal map converter";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs, ... }:
    let
      forAllSystems = nixpkgs.lib.genAttrs nixpkgs.lib.systems.flakeExposed;
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = pkgs.stdenv.mkDerivation {
            pname = "gtn";
            version = "unstable";
            src = self;

            preBuild = ''
              ln -s ${pkgs.stb}/include/stb/stb_image.h .
              ln -s ${pkgs.stb}/include/stb/stb_image_write.h .
            '';

            installFlags = [
              "PREFIX=$(out)"
            ];

            meta.mainProgram = "gtn";
          };
        }
      );

      overlays.default = final: prev: {
        gtn = self.packages.${final.system}.default;
      };
    };
}
