#pragma once
#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// Router Microarchitectures
enum class Router {
  OneStage,
  TwoStage,
  ThreeStage,
};

enum class Topology {
  SingleChipMesh,
  DragonflySW,
  DragonflyChiplet
};

enum class Traffic {
  test,
  uniform,
  hotspot,
  bitcomplement,
  bittranspose,
  bitreverse,
  bitshuffle,
  adversarial,
  dc_traces,
  all_to_all
};

struct Channel {
  Channel(int link_width = 0, int link_latency = 0) : width(link_width), latency(link_latency) {}
  int width;  // Link (bandwidth) can allocated at flit (1 flit/cycle) granularity.
  int latency;
};

const Channel on_chip_channel(1,1);
const Channel off_chip_channel(1,4);

struct Parameters {
 public:
  explicit Parameters(const std::string& filename = "");

  int buffer_size;     // flits
  int IF_buffer_size;  // flits
  int vc_number;
  Router microarchitecture;
  int processing_time;     // cycles
  int routing_time;        // cycles
  int vc_allocating_time;  // cycles
  int sw_allocating_time;  // cycles

  // Topology
  Topology topology;
  // represent different meanings in different topologies
  int radix;
  int scale;
  bool misrouting; // param for dragonfly

  // Simulation Parameters
  uint64_t simulation_time;
  float injection_increment;
  Traffic traffic;
  int packet_length;  // # of flits
  int timeout_threshold;
  int threads;
};

class TrafficManager;
class System;

extern TrafficManager* TM;
extern System* network;
extern Parameters* param;
