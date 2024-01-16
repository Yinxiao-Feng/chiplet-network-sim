# ChipletNetworkSim

### Features
- cross-platform

### Dependencies
- boost library
- netrace (modified from https://github.com/booksim/netrace)
	- bzip2

### Recommended Usage: Visual Studio 2022
- windows or linux (via ssh or wsl)
- open the directory as a cmake project

### Command Line: Ubuntu
```
sudo apt install cmake ninja-build build-essential libboost-all-dev libbz2-dev
cd chiplet-network-sim
mkdir -p input/netrace
wget -O input/netrace/blackscholes_64c_simsmall.tra.bz2 wget https://www.cs.utexas.edu/~netrace/download/blackscholes_64c_simsmall.tra.bz2
cmake --preset Linux-Release
cd builds/Linux-Release/
cmake --build .
./ChipletNetworkSim ../../input/dragonfly/dragonfly_sw_32.ini
./ChipletNetworkSim ../../input/hetero-link/multiple_chip_torus_8x8.ini
```