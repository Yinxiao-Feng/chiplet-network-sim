#include "traffic_manager.h"

#include "boost/dynamic_bitset.hpp"

TrafficManager::TrafficManager() {
  injection_rate_ = 0;
  traffic_ = param->traffic;
  traffic_scale_ = param->traffic_scale;
  if (traffic_scale_ == 0) traffic_scale_ = network->num_cores_;
  message_length_ = param->packet_length;
  if (traffic_ == "sd_trace") {
    trace_.open(param->trace_file, std::fstream::in);
    std::cout << "Trace file is read!" << std::endl;
    std::string head;
    std::getline(trace_, head);
  } else if (traffic_ == "netrace") {
    CTX = new nt_context_t();
    nt_open_trfile(CTX, param->netrace_file.c_str());
    nt_disable_dependencies(CTX);
    nt_print_trheader(CTX);
  }
  output_.open(param->output_file, std::fstream::out);
  log_.open(param->log_file, std::fstream::out);

  pkt_for_injection_ = 0;
  // statistics
  time_ = std::chrono::system_clock::now();
  all_message_num_.store(0);
  message_arrived_.store(0);
  message_timeout_.store(0);
  total_cycles_.store(0);
  total_internal_hops_.store(0);
  total_parallel_hops_.store(0);
  total_serial_hops_.store(0);
  total_other_hops_.store(0);
#ifdef DEBUG
  for (auto& chip : network->chips_) {
    for (auto& node : chip->nodes_) {
      for (auto& buf : node->in_buffers_) {
        traffic_map_[buf].store(0);
      }
    }
  }
#endif  // DEBUG
}

TrafficManager::~TrafficManager() {
  if (traffic_ == "sd_trace") {
    trace_.close();
  } else if (traffic_ == "netrace") {
    delete CTX;
  }
  output_.close();
  log_.close();
}

void TrafficManager::reset() {
  pkt_for_injection_ = 0;
  time_ = std::chrono::system_clock::now();
  all_message_num_.store(0);
  message_arrived_.store(0);
  message_timeout_.store(0);
  total_cycles_.store(0);
  total_internal_hops_.store(0);
  total_parallel_hops_.store(0);
  total_serial_hops_.store(0);
  total_other_hops_.store(0);
}

void TrafficManager::print_statistics() {
  std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - time_;
  double average_internal_hops = ((double)TM->total_internal_hops_ / TM->message_arrived_);
  double average_parallel_hops = ((double)TM->total_parallel_hops_ / TM->message_arrived_);
  double average_serial_hops = ((double)TM->total_serial_hops_ / TM->message_arrived_);
  double average_other_hops = ((double)TM->total_other_hops_ / TM->message_arrived_);
  std::cout << std::endl
            << "Time elapsed: " << elapsed_seconds.count() << "s" << std::endl
            << "Injection rate:" << injection_rate_ << " flits/(node*cycle)"
            << "    Injected:" << all_message_num_ << "    Arrived:  " << message_arrived_
            << "    Timeout:  " << message_timeout_ << std::endl
            << "Average latency: " << ((double)TM->total_cycles_ / TM->message_arrived_)
            << "  Average receiving rate: " << receiving_rate() << std::endl
            << "Internal Hops: " << average_internal_hops
            << "   Parallel Hops: " << average_parallel_hops
            << "   Serial Hops: " << average_serial_hops << "   Other Hops: " << average_other_hops
            << std::endl;
  output_ << injection_rate_ << "," << ((double)total_cycles_ / message_arrived_) << ","
          << receiving_rate() << std::endl;
#ifdef DEBUG
  // int max = 0;
  // Buffer* max_buf = nullptr;
  // for (auto& i : traffic_map_) {
  //   if (i.second > max) {
  //     max = i.second;
  //     max_buf = i.first;
  //   }
  // }
  // std::cout << "Max traffic: " << max << " at " << max_buf->node_->id_ << std::endl;
#endif  // DEBUG
}

void TrafficManager::genMes(std::vector<Packet*>& packets, uint64_t cyc) {
  if (traffic_ == "all_to_all") {
    all_to_all_mess(packets);
    return;
  } else if (traffic_ == "all_to_all_bi") {
    all_to_all_bi_mess(packets);
    return;
  } else if (traffic_ == "netrace") {
    netrace(packets, cyc);
    return;
  }
  for (pkt_for_injection_ += message_per_cycle(); pkt_for_injection_ >= 1; pkt_for_injection_--) {
    Packet* mess;
    if (traffic_ == "test")
      mess = new Packet(NodeID(0, 0), NodeID(0, 8), message_length_);
    else if (traffic_ == "uniform")
      mess = uniform_mess();
    else if (traffic_ == "intra_group_uniform")
      mess = intra_group_uniform_mess();
    else if (traffic_ == "hotspot")
      mess = hotspot_mess();
    else if (traffic_ == "bitcomplement")
      mess = bitcomplement_mess();
    else if (traffic_ == "bitreverse")
      mess = bitreverse_mess();
    else if (traffic_ == "bitshuffle")
      mess = bitshuffle_mess();
    else if (traffic_ == "bittranspose")
      mess = bittranspose_mess();
    else if (traffic_ == "adversarial")
      mess = adversarial_mess();
    else if (traffic_ == "sd_traces")
      mess = sd_trace_mess();
    else
      std::cerr << "Unknown traffic pattern!" << std::endl;
    packets.push_back(mess);
    all_message_num_++;
  }
}

Packet* TrafficManager::uniform_mess() {
  int src, dest;
  int core_per_chip = network->chips_[0]->number_cores_;
  while (true) {
    src = gen() % traffic_scale_;
    dest = gen() % traffic_scale_;
    if (dest != src) break;
  }
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

Packet* TrafficManager::intra_group_uniform_mess() {
  int src, dest;
  int core_per_chip = network->chips_[0]->number_cores_;
  while (true) {
    src = gen() % traffic_scale_;
    dest = gen() % traffic_scale_;
    if (dest != src) break;
  }
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

Packet* TrafficManager::hotspot_mess() {
  int src, dest;
  int core_per_chip = network->chips_[0]->number_cores_;
  int node_per_WG = traffic_scale_ / 4;
  int WG1, WG2;
  while (true) {
    WG1 = gen() % 4 * 10;
    WG2 = gen() % 4 * 10;
    src = (WG1 * node_per_WG + gen() % node_per_WG) % traffic_scale_;
    dest = (WG2 * node_per_WG + gen() % node_per_WG) % traffic_scale_;
    if (src != dest) break;
  }
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

Packet* TrafficManager::bitcomplement_mess() {
  int src, dest;
  int bits = (int)floor(log2(traffic_scale_));
  int core_per_chip = network->chips_[0]->number_cores_;
  while (true) {
    src = gen() % traffic_scale_;
    boost::dynamic_bitset<> src_binary(bits, src);
    boost::dynamic_bitset<> dest_binary = ~src_binary;
    dest = dest_binary.to_ulong() % traffic_scale_;
    if (dest != src) break;
  }
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

Packet* TrafficManager::bitreverse_mess() {
  int src, dest;
  int bits = (int)floor(log2(traffic_scale_));
  int core_per_chip = network->chips_[0]->number_cores_;
  while (true) {
    src = gen() % traffic_scale_;
    boost::dynamic_bitset<> src_binary(bits, src);
    boost::dynamic_bitset<> dest_binary(bits);
    for (int i = 0; i < bits; ++i) {
      dest_binary[i] = src_binary[bits - 1 - i];
    }
    dest = dest_binary.to_ulong() % traffic_scale_;
    if (dest != src) break;
  }
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

Packet* TrafficManager::bitshuffle_mess() {
  int src, dest;
  int bits = (int)floor(log2(traffic_scale_));
  int core_per_chip = network->chips_[0]->number_cores_;
  while (true) {
    src = gen() % traffic_scale_;
    boost::dynamic_bitset<> src_binary(bits, src);
    bool last_bit = src_binary[bits - 1];
    boost::dynamic_bitset<> dest_binary = src_binary << 1;
    dest_binary[0] = last_bit;
    dest = dest_binary.to_ulong() % traffic_scale_;
    if (dest != src) break;
  }
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

Packet* TrafficManager::bittranspose_mess() {
  int src, dest;
  int bits = (int)floor(log2(traffic_scale_));
  int core_per_chip = network->chips_[0]->number_cores_;
  while (true) {
    src = gen() % traffic_scale_;
    boost::dynamic_bitset<> src_binary(bits, src);
    boost::dynamic_bitset<> dest_binary(bits);
    for (int i = 0; i < bits; ++i) {
      dest_binary[i] = src_binary[(i + bits / 2) % bits];
    }
    dest = dest_binary.to_ulong() % traffic_scale_;
    if (dest != src) break;
  }
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

Packet* TrafficManager::adversarial_mess() {
  int src, dest;
  int core_per_chip = network->chips_[0]->number_cores_;
  int node_per_WG = traffic_scale_ / 41;
  src = gen() % traffic_scale_;
  int src_group = src / node_per_WG;
  dest = ((src_group + 1) * node_per_WG + gen() % node_per_WG) % traffic_scale_;
  assert(src != dest);
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

Packet* TrafficManager::sd_trace_mess() {
  int src, dest;
  int core_number = network->num_cores_;
  // int core_per_chip = (KNode - 2) * (KNode - 2);
  int core_per_chip = network->chips_[0]->number_cores_;
  while (true) {
    std::string word;
    std::getline(trace_, word, ',');
    std::getline(trace_, word, ',');
    src = std::stoi(word) * 4 + gen() % 4;
    std::getline(trace_, word);
    dest = std::stoi(word) * 4 + gen() % 4;
    if (dest != src) break;
  }
  // return new Message(NodeID(src, src / core_per_chip),
  //                    NodeID(dest, dest / core_per_chip));
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

void TrafficManager::all_to_all_mess(std::vector<Packet*>& packets) {
  int core_per_chip = network->chips_[0]->number_cores_;
  for (pkt_for_injection_ += message_per_cycle(); pkt_for_injection_ > traffic_scale_;
       pkt_for_injection_ -= traffic_scale_) {
    for (int src = 0; src < traffic_scale_; src++) {
      int dest1;
      if (param->topology == "DragonflyChiplet") {
        if (src % 16 == 0 || src % 16 == 1 || src % 16 == 4 || src % 16 == 5)
          dest1 = (src + 2) % traffic_scale_;
        else if (src % 16 == 2 || src % 16 == 3 || src % 16 == 6 || src % 16 == 7)
          dest1 = (src + 8) % traffic_scale_;
        else if (src % 16 == 8 || src % 16 == 9 || src % 16 == 12 || src % 16 == 13)
          dest1 = (src + 8) % traffic_scale_;
        else if (src % 16 == 10 || src % 16 == 11 || src % 16 == 14 || src % 16 == 15)
          dest1 = (src - 2) % traffic_scale_;
      }
      else if (param->topology == "DragonflySW") {
		dest1 = (src + 1) % traffic_scale_;
	  }
      Packet* mess = new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                                NodeID(dest1 % core_per_chip, dest1 / core_per_chip), message_length_);
      packets.push_back(mess);
      all_message_num_ += 1;
    }
  }
}

void TrafficManager::all_to_all_bi_mess(std::vector<Packet*>& packets) {
  int core_per_chip = network->chips_[0]->number_cores_;
  for (pkt_for_injection_ += message_per_cycle(); pkt_for_injection_ > traffic_scale_ * 2;
       pkt_for_injection_ -= traffic_scale_ * 2) {
    for (int src = 0; src < traffic_scale_; src++) {
      int dest1, dest2;
      if (param->topology == "DragonflyChiplet") {
        if (src % 16 == 0 || src % 16 == 1 || src % 16 == 4 || src % 16 == 5) {
          dest1 = (src + 2) % traffic_scale_;
          dest2 = (src - 8 + traffic_scale_) % traffic_scale_;
        } else if (src % 16 == 2 || src % 16 == 3 || src % 16 == 6 || src % 16 == 7) {
          dest1 = (src + 8) % traffic_scale_;
          dest2 = (src - 2) % traffic_scale_;
        } else if (src % 16 == 8 || src % 16 == 9 || src % 16 == 12 || src % 16 == 13) {
          dest1 = (src + 8) % traffic_scale_;
          dest2 = (src + 2) % traffic_scale_;
        } else if (src % 16 == 10 || src % 16 == 11 || src % 16 == 14 || src % 16 == 15) {
          dest1 = (src - 2) % traffic_scale_;
		  dest2 = (src - 8) % traffic_scale_;
        }
      } else if (param->topology == "DragonflySW") {
        dest1 = (src + 1) % traffic_scale_;
        dest2  = (src - 1 + traffic_scale_) % traffic_scale_;
      }
      Packet* mess = new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                                NodeID(dest1 % core_per_chip, dest1 / core_per_chip), message_length_);
      packets.push_back(mess);
      mess = new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                        NodeID(dest2 % core_per_chip, dest2 / core_per_chip), message_length_);
      packets.push_back(mess);
      all_message_num_ += 2;
    }
  }
}

void TrafficManager::netrace(std::vector<Packet*>& vecmess, uint64_t cyc) {
  int src, dest;
  static int core_per_chip = network->chips_[0]->number_cores_;
  static nt_packet_t* trace_packet = nullptr;
  if (cyc > CTX->input_trheader->num_cycles)
    return;
  else if ((cyc + 1) % 100000000 == 0) {
    print_statistics();
  }
  while ((CTX->latest_active_packet_cycle == cyc)) {
    trace_packet = nt_read_packet(CTX);
    if (trace_packet == nullptr)
      return;
    else if (nt_get_packet_size(trace_packet) == -1) {
      nt_packet_free(trace_packet);
      continue;
    } else if (all_message_num_ % 10000000 == 0)
      nt_print_packet(trace_packet);
    src = trace_packet->src;
    dest = trace_packet->dst;
    if (src != dest) {
      int packet_length = ceil((double)nt_get_packet_size(trace_packet) / 16);  // 16B Bus width
      // Packet* packet =
      //     new Packet(NodeID(src % core_per_chip, src / core_per_chip),
      //                NodeID(dest % core_per_chip, dest / core_per_chip), packet_length);
      Packet* packet = new Packet(network->id2nodeid(src), network->id2nodeid(dest), packet_length);
      vecmess.push_back(packet);
      all_message_num_++;
    }
    // Get another packet from trace
    nt_packet_free(trace_packet);
  }
}
