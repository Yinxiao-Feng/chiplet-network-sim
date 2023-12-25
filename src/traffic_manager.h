#pragma once
#include <chrono>
#include <fstream>

#include "system.h"

class TrafficManager {
 public:
  TrafficManager();
  ~TrafficManager();
  void reset();
  void genMes(std::vector<Packet*>& packets, uint64_t cyc = 0);
  Packet* uniform_mess();
  Packet* hotspot_mess();
  Packet* bitcomplement_mess();
  Packet* bitreverse_mess();
  Packet* bitshuffle_mess();
  Packet* bittranspose_mess();
  Packet* adversarial_mess();
  Packet* dc_trace_mess();
  void all_to_all_mess(std::vector<Packet*>& packets);
  inline float receiving_rate() {
    return injection_rate_ * ((float)TM->message_arrived_ / TM->all_message_num_);
  };

  void print_info();

  std::fstream trace_;
  std::fstream output_;

  float injection_rate_;
  Traffic traffic_;
  int message_length_;

  std::map<std::pair<int, int>, uint64_t> traffic_map_;
  float pkt_for_injection_;
  // statistics
  std::chrono::system_clock::time_point time_;
  std::atomic_int64_t all_message_num_;
  std::atomic_int64_t message_arrived_;
  std::atomic_int64_t message_timeout_;
  std::atomic_int64_t total_cycles_;
  std::atomic_int64_t total_internal_hops_;
  std::atomic_int64_t total_parallel_hops_;
  std::atomic_int64_t total_serial_hops_;
};
