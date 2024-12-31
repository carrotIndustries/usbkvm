{
  inputs = {
    nixpkgs = {
      url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    };
  };
  outputs = { self, nixpkgs, ... }: {
    packages = nixpkgs.lib.genAttrs [ "x86_64-linux" "aarch64-linux" ] (system: let
      pkgs = import nixpkgs {
        inherit system;
      };
    in rec {
      usbkvm-udev = pkgs.runCommand "usbkvm-udev" {} ''
        mkdir -p $out/lib/udev/rules.d
        cp ${./app}/70-usbkvm.rules $out/lib/udev/rules.d
      '';
    });

    hydraJobs = {
      inherit (self)
        packages;
    };
  };
}
