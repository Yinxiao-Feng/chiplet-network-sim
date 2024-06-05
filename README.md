# Artifact Evaluation Instruction
Artifact name: Chiplet Network Simulator (CNSim)
## Source Code
The source code of CNSim is in the directory "chiplet-network-sim".
Besides our artifacts, we provide a brief instruction to compare with one of the baseline [BookSim2](https://github.com/booksim/booksim2), which can be downloaded by
```
git clone https://github.com/booksim/booksim2.git
```
The configuration files for the BookSim2 are in the directory "booksim_runfiles".

The traces can be downloaded from https://www.cs.utexas.edu/~netrace/, and the trace processing codes are modified from https://github.com/booksim/netrace

## Environment Setup 
CNSim is a C/C++ program developed by Visual Studio 2022 and is verified on both Windows and Linux platforms. Since the Booksim2 natively runs on Linux, we recommend using an Ubuntu 22.04 machine (at least 8C/16T) in the evaluation.
### Dependencies 
```
sudo apt install cmake ninja-build build-essential libboost-all-dev libbz2-dev
sudo apt install flex bison
```
### Build and Basic Test
#### BookSim2
```
cd booksim2/src
make -j8
./booksim ../runfiles/meshconfig
```
#### CNSim
```
cd chiplet-network-sim
mkdir output
mkdir -p input/netrace
wget -O input/netrace/blackscholes_64c_simsmall.tra.bz2 https://www.cs.utexas.edu/~netrace/download/blackscholes_64c_simsmall.tra.bz2
cmake --preset Linux-Release
cd builds/Linux-Release/
cmake --build .
./ChipletNetworkSim ../../../cnsim_runfiles/test.ini
./ChipletNetworkSim ../../input/hetero-link/single_chip_mesh_8x8.ini
```

## Evaluation
### Basic function: the latency-injection curves (Figure 6)
#### Boosim2
The "injection rate (packets/cycle/core)" parameter is at the last line of configuration files.
```
~/booksim2/src/booksim ~/booksim_runfiles/fig6_4x4 
~/booksim2/src/booksim ~/booksim_runfiles/fig6_8x8
```
The output is shown in the terminal ("Packet latency average")

**Expected result:**
 - 33.67 for 4x4 at 0.1 packets/cycle/node (0.5 flits/cycle/node)
 - 38.98 for 8x8 at 0.01 packets/cycle/node (0.05 flits/cycle/node)

#### CNSim
```
cd chiplet-network-sim/builds/Linux-Release
./ChipletNetworkSim ../../../cnsim_runfiles/fig6_4x4.ini
./ChipletNetworkSim ../../../cnsim_runfiles/fig6_8x8.ini
```
The outputs (log.txt and output.csv) are in the subdirectory "output".

**Expected result:**
 - 11.2963 average latency for 4x4 at 0.05 flits/cycle/node and 0.911569 maximum average receiving traffic
 - 17.1353 average latency for 8x8 at 0.05 flits/cycle/node and 0.460206 maximum average receiving traffic

### Core contribution: simulation speed
It takes hours to simulate 100K cycles for the large-scale Dragonfly of 16K nodes; therefore, we simulate 1K cycles in the Artifact Evaluation.
#### Boosim2 (Figure 7)
```
~/booksim2/src/booksim ~/booksim_runfiles/fig7a
~/booksim2/src/booksim ~/booksim_runfiles/fig7b
```
The output is shown in the terminal ("Total run time")
Boosim2 will run several rounds to converge: "Time taken is **n** cycles". Therefore, the run time for 100K cycles is "Total run time" / **(n/100K)**.

**Expected result:**
- 2D-mesh (16 nodes), 100K cycles, 0.5 injection rate: ~3300ms
- Dragonfly (16K nodes), 1K cycles, 0.5 injection rate: ~165s

#### CNSim (Figure 7)
The "threads" parameter can be changed in the configuration files.
```
cd chiplet-network-sim/builds/Linux-Release
./ChipletNetworkSim ../../../cnsim_runfiles/fig7a.ini
./ChipletNetworkSim ../../../cnsim_runfiles/fig7b.ini
```
**Multi-thread speedups are highly CPU-dependent. The number of CPU cores is supposed to be greater than the number of running threads**

**Expected result (on an 8C/16T platform):**
 - 2D-mesh (16 nodes), 100K cycles, 1 thread, 0.5 injection rate: ~158ms
 - Dragonfly (16K nodes), 1K cycles, **1 thread**, 0.5 injection rate: ~50s
 - Dragonfly (16K nodes), 1K cycles, **8 threads**, 0.5 injection rate: ~24s

#### Concurrency and inconsistency (Figure 9 and Figure 10)
The "threads" and "issue width" parameters can be changed in the configuration files. **Results are CPU/environment-dependent, and the thread number should be no more than the core number**
```
./ChipletNetworkSim ../../../cnsim_runfiles/fig9_10_1K
./ChipletNetworkSim ../../../cnsim_runfiles/fig9_10_16K
```
**Expected result**
 - Noticeable speedup with hyper-threading turned on (Figure 9)
 - Negligible average latency deviations (<1%) (Figure 10)

### Memory consumption (Figure 8)
May be very slow.
```
sudo apt install valgrind
valgrind --tool=massif ~/booksim2/src/booksim ~/booksim_runfiles/fig7b
cd chiplet-network-sim/builds/Linux-Release
valgrind --tool=massif ./ChipletNetworkSim ../../../cnsim_runfiles/fig7b.ini
```

### Using CNSim (Figure 12,13,15-18)
CNSim can be used to evaluate various networks. The topology (system) source codes can be found in the subdirectory "src/topologies", and the configuration files can be found in the subdirectory "input". The simulation results are not the main claims of this paper. We are showing that CNSim can be used to do these things:
 - Simulating various topologies, routing algorithms, and microarchitectures.
 - Simulating different traffic patterns and real world traces
 - Simulating large-scale networks (> 16K nodes)
 - Evaluating heterogeneous link bandwidth and latency

#### Hetero-IF (Figure 12)
```
./ChipletNetworkSim ../../../cnsim_runfiles/fig12_1.ini
./ChipletNetworkSim ../../../cnsim_runfiles/fig12_2.ini
./ChipletNetworkSim ../../../cnsim_runfiles/fig12_3.ini
./ChipletNetworkSim ../../../cnsim_runfiles/fig12_4.ini
```

**Expected result**
 - Low load Latency, Maximum average receiving traffic
   - 1-Chip Mesh: 15.5, 0.9
   - 2x2-Chips B=1 L=2: 16.4, 0.85
   - 2x2-Chips B=2 L=5: 17.8, 1.13
   - 2x2-Chips Torus: 14.3, 1.23

#### Hetero-IF + Traces (Figure 13)
All the other traces can be downloaded from https://www.cs.utexas.edu/~netrace/ and can be changed in the configuration file: "netrace_file".
```
./ChipletNetworkSim ../../../cnsim_runfiles/fig13_1.ini
./ChipletNetworkSim ../../../cnsim_runfiles/fig13_2.ini
./ChipletNetworkSim ../../../cnsim_runfiles/fig13_3.ini
./ChipletNetworkSim ../../../cnsim_runfiles/fig13_4.ini
./ChipletNetworkSim ../../../cnsim_runfiles/fig13_5.ini
```

The result is in log.txt (last block).

**Expected result (Black-Scholes)**
 - Average latency
   - 1-Chip Mesh XY: 75.9
   - 2x2-Chips (B=2, L=4) XY: 79.7
   - 4x4-Chips (B=2, L=4) XY: 84.1
   - 2x2-Chips (B=2, L=4) NFR_Adaptive: 28.7
   - 4x4-Chip Torus Hetro-Link CLUE: 21.8

#### Dragonfly (Figure 17)
The Large-Scale Dragonfly simulation can be slow (hours).
```
./ChipletNetworkSim ../../../cnsim_runfiles/fig17_1.ini
./ChipletNetworkSim ../../../cnsim_runfiles/fig17_2.ini
./ChipletNetworkSim ../../../cnsim_runfiles/fig17_3.ini
./ChipletNetworkSim ../../../cnsim_runfiles/fig17_4.ini
```
**Expected result**
 - flits/chip/cycle = flits/node/cycle * node/chip
 - Low load Latency, Maximum average receiving traffic
   - (a) radix-16 switch: 44.8, 0.693
   - (a) radix-16 chiplet: 46.4, 0.69 (0.1736 * 4)
   - (b) radix-32 switch: 95.4, 0.60
   - (b) radix-32 chiplet: 99.0, 0.48 (0.0789 * 49/8)