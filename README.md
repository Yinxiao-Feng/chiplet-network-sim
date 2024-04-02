# ChipletNetworkSim

### Features
- cross-platform

### Dependencies
- boost library
- netrace (optional, modified from https://github.com/booksim/netrace)
	- bzip2

### Recommended Usage: Visual Studio 2022
- windows or linux (via ssh or wsl)
- open the directory as a cmake project

### Command Line for Ubuntu 22.04
```
sudo apt install cmake ninja-build build-essential libboost-all-dev libbz2-dev
cd chiplet-network-sim
cmake --preset Linux-Release
cd builds/Linux-Release/
cmake --build .
./ChipletNetworkSim ../../input/dragonfly/global-chiplet-16.ini
```