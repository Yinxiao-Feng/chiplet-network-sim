#pragma once
#include <chrono>
#include <fstream>

#include "system.h"
extern "C" {
#include "netrace.h"
}

class TrafficManager {
 public:
  TrafficManager();
  ~TrafficManager();
  void reset();
  void genMes(std::vector<Packet*>& packets, uint64_t cyc = 0);
  Packet* uniform_mess();
  Packet* intra_group_mess();
  Packet* hotspot_mess();
  Packet* bitcomplement_mess();
  Packet* bitreverse_mess();
  Packet* bitshuffle_mess();
  Packet* bittranspose_mess();
  Packet* adversarial_mess();
  Packet* sd_trace_mess();
  void all_to_all_mess(std::vector<Packet*>& packets);
  void netrace(std::vector<Packet*>& packets, uint64_t cyc);
  inline float receiving_rate() const {
    return injection_rate_ * ((float)TM->message_arrived_ / TM->all_message_num_);
  };

  void print_statistics();

  std::fstream trace_;
  nt_context_t* CTX;
  std::fstream output_;
  std::fstream log_;

  float injection_rate_;
  std::string traffic_;
  int message_length_;

  std::unordered_map<Buffer*, std::atomic_uint64_t> traffic_map_;
  float pkt_for_injection_;
  // atomic statistics, modified by all threds
  std::chrono::system_clock::time_point time_;
  std::atomic_uint64_t all_message_num_;
  std::atomic_uint64_t message_arrived_;
  std::atomic_uint64_t message_timeout_;
  std::atomic_uint64_t total_cycles_;
  std::atomic_uint64_t total_internal_hops_;
  std::atomic_uint64_t total_parallel_hops_;
  std::atomic_uint64_t total_serial_hops_;
  std::atomic_uint64_t total_other_hops_;
};
