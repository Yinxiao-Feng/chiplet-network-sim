#include "config.h"

Parameters *param;
TrafficManager *TM;
System *network;

Parameters::Parameters(const std::string &filename) {
  topology = Topology::DragonflySW;
  // topology = Topology::SingleChipMesh;
  // topology = Topology::DragonflyChiplet;

  radix = 32;
  scale = 4;
  misrouting = false;

  buffer_size = 256;
  IF_buffer_size = 16;
  vc_number = 2;

  processing_time = 1;
  microarchitecture = Router::OneStage;
  // 0: each stage finish in one cycle, no extra delay
  routing_time = 0;
  vc_allocating_time = 0;
  sw_allocating_time = 0;

  injection_increment = 0.1;
  simulation_time = 1000;
  traffic = Traffic::uniform;
  packet_length = 1;
  timeout_threshold = 200;
  threads = 0;
}
