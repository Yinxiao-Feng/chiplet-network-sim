#pragma once
#include <boost/optional.hpp>
#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

// Router Microarchitectures
// enum class Router {
//  OneStage,
//  TwoStage,
//  ThreeStage,
//};

const std::vector<std::string> router_stage_nums = {"OneStage", "TwoStage", "ThreeStage"};
const std::vector<std::string> topologies = {"SingleChipMesh", "DragonflySW", "DragonflyChiplet"};
const std::vector<std::string> traffic_patterns = {
    "test",       "uniform",     "hotspot",   "bitcomplement", "bittranspose", "bitreverse",
    "bitshuffle", "adversarial", "dc_traces", "netrace",       "all_to_all"};

// enum class Topology { SingleChipMesh, DragonflySW, DragonflyChiplet };
//
// enum class Traffic {
//   test,
//   uniform,
//   hotspot,
//   bitcomplement,
//   bittranspose,
//   bitreverse,
//   bitshuffle,
//   adversarial,
//   dc_traces,
//   all_to_all
// };

struct Channel {
  Channel(int link_width = 0, int link_latency = 0) : width(link_width), latency(link_latency) {}
  int width;  // Link (bandwidth) can allocated at flit (1 flit/cycle) granularity.
  int latency;
};

const Channel on_chip_channel(1, 1);
const Channel off_chip_channel(1, 4);

struct Parameters {
 public:
  explicit Parameters(const std::string& config_file = "");

  int buffer_size;     // flits
  int IF_buffer_size;  // flits
  int vc_number;
  std::string microarchitecture;
  int processing_time;     // cycles
  int routing_time;        // cycles
  int vc_allocating_time;  // cycles
  int sw_allocating_time;  // cycles

  // Topology
  std::string topology;
  // represent different meanings in different topologies
  int radix;
  int scale;
  bool misrouting;  // param for dragonfly

  // Simulation Parameters
  uint64_t simulation_time;
  float injection_increment;
  std::string traffic;
  int packet_length;  // # of flits
  int timeout_threshold;
  int timeout_limit;
  int threads;

  // I/O Files
  std::string trace_file, netrace_file, output_file, log_file;
};

class TrafficManager;
class System;

extern TrafficManager* TM;
extern System* network;
extern Parameters* param;
