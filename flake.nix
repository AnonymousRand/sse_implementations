{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = inputs:
    let
      pkgs = inputs.nixpkgs.legacyPackages."x86_64-linux";
    in
    {
      devShells."x86_64-linux".default = pkgs.mkShell {
        packages = [
          pkgs.cmake
          pkgs.conan
          pkgs.gcc
        ];
      };
    };
}
