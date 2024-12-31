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
      usbkvm-fw = pkgs.stdenv.mkDerivation rec {
        pname = "usbkvm-fw";
        version = "0.0.1";

        src = nixpkgs.lib.cleanSource ./.;

        sourceRoot = "${src.name}/fw/usbkvm";

        nativeBuildInputs = with pkgs; [
          gcc-arm-embedded
          python3
        ];

        postPatch = ''
          patchShebangs --build write_header.py
        '';

        installPhase = ''
          mkdir -p $out
          cp -r build/* $out
        '';
      };

      mslib = pkgs.buildGoModule {
        pname = "mslib";
        version = "0.0.1";

        src = ./. + "/app/ms-tools";

        buildInputs = with pkgs; [
          hidapi
        ];

        buildPhase = ''
          mkdir -p $out
          go build -C lib -o $out -buildmode=c-archive mslib.go
        '';

        vendorHash = "sha256-imHpsos7RDpATSZFWRxug67F7VgjRTT1SkLt7cWk6tU=";
      };

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
