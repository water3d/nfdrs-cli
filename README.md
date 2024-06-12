This is the Docker version of the commandline calculator for NFDRS Version 4.0 (2016)
Written By: Stuart Brittan and Matt Jolly
Build modifications (Dockerless) by Nick Santos June 2024
Version: 4.0 (23 Oct 2021)

# Shell Script
Run the following in the folder you wish to build the binary into:
```bash
git clone https://github.com/water3d/nfdrs-cli
cd nfdrs-cli
chmod +x build.sh
./build.sh
```

Afterward, the executable NFDRScliV4 will be in the current directory

# Docker
Build it like this (may have to use 'sudo'):
docker build -t nfdrscli:4.0 .

Run it like this:
docker run --rm -e INCFG=/in/CCaccia.cfg -v D:\In:/in -v D:\Out:/out nfdrscli:4.0

This will give you an interactive shell with the container:
docker run -it -e INCFG=/in/CCaccia.cfg -v D:\In:/in -v D:\Out:/out nfdrscli:4.0 bash
