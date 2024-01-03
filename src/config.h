#pragma once
#include <boost/optional.hpp>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

const std::vector<std::string> router_stage_nums = {"OneStage", "TwoStage", "ThreeStage"};
const std::vector<std::string> topologies = {"SingleChipMesh", "DragonflySW", "DragonflyChiplet"};
const std::vector<std::string> traffic_patterns = {
    "test",       "uniform",     "hotspot",  "bitcomplement", "bittranspose", "bitreverse",
    "bitshuffle", "adversarial", "sd_trace", "netrace",       "all_to_all"};

struct Channel {
  Channel(int link_width = 0, int link_latency = 0) : width(link_width), latency(link_latency) {}
  int width;  // Link (bandwidth) can allocated at flit (1 flit/cycle) granularity.
  int latency;
};

const Channel on_chip_channel(1, 0);
const Channel off_chip_channel(1, 4);

struct Parameters {
 public:
  explicit Parameters(const std::string& config_file = "");
  std::string config_file_path;

  // Network parameters
  std::string topology;
  // represent different meanings in different topologies
  int radix;
  int scale;
  std::string routing_algorithm;
  int buffer_size;     // flits
  int IF_buffer_size;  // flits
  int vc_number;
  std::string router_stages;
  int processing_time;     // cycles
  int routing_time;        // cycles
  int vc_allocating_time;  // cycles
  int sw_allocating_time;  // cycles

  // Workloads
  std::string traffic;
  int packet_length;  // # of flits

  // Simulation Parameters
  uint64_t simulation_time;
  float injection_increment;
  int timeout_threshold;
  int timeout_limit;
  int threads;

  // I/O Files
  std::string trace_file, netrace_file, output_file, log_file;

  void print_params() const {
    // print all memebers
    std::cout << std::setw(20) << "Config File: " << config_file_path << std::endl;
    std::cout << std::setw(20) << "Topology: " << topology << std::endl;
    std::cout << std::setw(20) << "Routing Algorithm: " << routing_algorithm << std::endl;
    std::cout << std::setw(20) << "Router Stage Num: " << router_stages << std::endl;
    std::cout << std::setw(20) << "Buffer Size: " << buffer_size << std::endl;
    std::cout << std::setw(20) << "IF Buffer Size: " << IF_buffer_size << std::endl;
    std::cout << std::setw(20) << "VC number: " << vc_number << std::endl;
    std::cout << std::setw(20) << "Processing Time: " << processing_time << std::endl;
    std::cout << std::setw(20) << "Routing Time: " << routing_time << std::endl;
    std::cout << std::setw(20) << "VC Allocating Time: " << vc_allocating_time << std::endl;
    std::cout << std::setw(20) << "Traffic: " << traffic << std::endl;
    std::cout << std::setw(20) << "Packet Length: " << packet_length << std::endl;
    std::cout << std::setw(20) << "Simulation Time: " << simulation_time << std::endl;
    std::cout << std::setw(20) << "Inject Increment: " << injection_increment << std::endl;
    std::cout << std::setw(20) << "Timeout Threshold: " << timeout_threshold << std::endl;
    std::cout << std::setw(20) << "Timeout Pkts Limit: " << timeout_limit << std::endl;
    std::cout << std::setw(20) << "Threads Number: " << threads << std::endl;
    std::cout << std::setw(20) << "Trace File: " << trace_file << std::endl;
    std::cout << std::setw(20) << "Netrace File: " << netrace_file << std::endl;
    std::cout << std::setw(20) << "Output File: " << output_file << std::endl;
    std::cout << std::setw(20) << "Log File: " << log_file << std::endl;
  }
};

class TrafficManager;
class System;

extern TrafficManager* TM;
extern System* network;
extern Parameters* param;
