# Chiplet Network Sim

Chiplet Network Simulator (CNSim) is a cycle-accurate, packet-parallel simulator for efficiently simulating large-scale chiplet-based networks.

If you use CNSim in your research, we would appreciate a citation of the following paper:

Yinxiao Feng, Yuchen Wei, Dong Xiang, and Kaisheng Ma. Evaluating Chiplet-based Large-Scale Interconnection Networks via Cycle-Accurate Packet-Parallel Simulation. In 2024 USENIX Annual Technical Conference (USENIX ATC '24), 2024.

To reproduce the results in the paper, please use the [`atc24_artifact`](https://github.com/Yinxiao-Feng/chiplet-network-sim/tree/atc24_artifact) branch.

### Features
- Multithreading
- Cycle-accurate
- Highly configurable and customizable topologies, routing algorithms, and microarchitectures

### Dependencies
- Boost (header-only libraries)
- netrace (built in, modified from https://github.com/booksim/netrace)
	- Reading netrace traces requires the `bzip2` command-line tool

### Usage
- Windows
  - Visual Studio 2022
  - Open the directory as a CMake project
- Linux
  - Build and run as shown below

#### Ubuntu 22.04
```
sudo apt install cmake ninja-build build-essential libboost-dev bzip2
cd chiplet-network-sim
mkdir -p output
cmake --preset Release
cd builds/Release/
cmake --build .
./ChipletNetworkSim ../../input/multiple_chip_mesh_4x4.ini
```
Results are written to `output/` by default (see `output_file` and `log_file` in the `[Files]` section of the config file).

### Acknowledgements
- Yuchen Wei, Tsinghua University
- Dong Xiang and Kaisheng Ma, Tsinghua University
- BookSim and Netrace (https://github.com/booksim)
