#include "traffic_manager.h"

#include "boost/dynamic_bitset.hpp"
#include "boost/random.hpp"
boost::mt19937 gen;

TrafficManager::TrafficManager() {
  injection_rate_ = 0;
  traffic_ = param->traffic;
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
  // for (int i = 0; i < network->chips_[0]->number_nodes_; i++) {
  //   for (int j = 0; j < network->chips_[0]->number_nodes_; j++) {
  //     traffic_map_[{i, j}] = 0;
  //   }
  // }
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
}

void TrafficManager::print_statistics() {
  std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - time_;
  float average_internal_hops = ((float)TM->total_internal_hops_ / TM->message_arrived_);
  float average_parallel_hops = ((float)TM->total_parallel_hops_ / TM->message_arrived_);
  float average_serial_hops = ((float)TM->total_serial_hops_ / TM->message_arrived_);
  std::cout << std::endl
            << "Time elapsed: " << elapsed_seconds.count() << "s" << std::endl
            << "Injection rate:" << injection_rate_ << " flits/(node*cycle)"
            << "    Injected:" << all_message_num_ << "    Arrived:  " << message_arrived_
            << "    Timeout:  " << message_timeout_ << std::endl
            << "Average latency: " << ((float)TM->total_cycles_ / TM->message_arrived_)
            << "  Average receiving rate: " << receiving_rate() << std::endl
            << "Internal Hops: " << average_internal_hops
            << "   Parallel Hops: " << average_parallel_hops
            << "   Serial Hops: " << average_serial_hops << std::endl;
  output_ << injection_rate_ << "," << ((float)total_cycles_ / message_arrived_) << ","
          << receiving_rate() << std::endl;
}

void TrafficManager::genMes(std::vector<Packet*>& packets, uint64_t cyc) {
  if (traffic_ == "all_to_all") {
    all_to_all_mess(packets);
    return;
  } else if (traffic_ == "netrace") {
    netrace(packets, cyc);
    return;
  }
  double message_per_cycle = injection_rate_ * network->num_cores_ / param->packet_length;
  for (pkt_for_injection_ += message_per_cycle; pkt_for_injection_ > 1; pkt_for_injection_--) {
    Packet* mess;
    if (traffic_ == "test")
      mess = new Packet(NodeID(0, 0), NodeID(3, 0), message_length_);
    else if (traffic_ == "uniform")
      mess = uniform_mess();
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
      mess = uniform_mess();
    packets.push_back(mess);
    all_message_num_++;
  }
}

Packet* TrafficManager::uniform_mess() {
  int src, dest;
  int core_per_chip = network->chips_[0]->number_cores_;
  int core_number = network->num_cores_;
  while (true) {
    src = gen() % core_number;
    dest = gen() % core_number;
    if (dest != src) break;
  }
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

Packet* TrafficManager::hotspot_mess() {
  int src, dest;
  int core_number = network->num_cores_;
  int core_per_chip = network->chips_[0]->number_cores_;
  int WG1, WG2;
  while (true) {
    WG1 = (gen() % 4 * 10) % 37;
    WG2 = (gen() % 4 * 10) % 37;
    src = (WG1 * 144 + gen() % 144) % core_number;
    dest = (WG2 * 144 + gen() % 144) % core_number;
    if (src != dest) break;
  }
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

Packet* TrafficManager::bitcomplement_mess() {
  int src, dest;
  // int core_number = network->num_cores_;
  int core_number = network->chips_[0]->number_cores_ * 9;
  int bits = floor(log2(core_number));
  int core_per_chip = network->chips_[0]->number_cores_;
  while (true) {
    src = gen() % core_number;
    boost::dynamic_bitset<> src_binary(bits, src);
    boost::dynamic_bitset<> dest_binary = ~src_binary;
    dest = dest_binary.to_ulong();
    if (dest != src) break;
  }
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

Packet* TrafficManager::bitreverse_mess() {
  int src, dest;
  // int core_number = network->num_cores_;
  int core_number = network->chips_[0]->number_cores_ * 9;
  int bits = floor(log2(core_number));
  int core_per_chip = network->chips_[0]->number_cores_;
  while (true) {
    src = gen() % core_number;
    boost::dynamic_bitset<> src_binary(bits, src);
    boost::dynamic_bitset<> dest_binary(bits);
    for (int i = 0; i < bits; ++i) {
      dest_binary[i] = src_binary[bits - 1 - i];
    }
    dest = dest_binary.to_ulong();
    if (dest != src) break;
  }
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

Packet* TrafficManager::bitshuffle_mess() {
  int src, dest;
  int core_number = network->chips_[0]->number_cores_ * 9;
  // int core_number = network->num_cores_;
  int bits = floor(log2(core_number));
  int core_per_chip = network->chips_[0]->number_cores_;
  while (true) {
    src = gen() % core_number;
    boost::dynamic_bitset<> src_binary(bits, src);
    bool last_bit = src_binary[bits - 1];
    boost::dynamic_bitset<> dest_binary = src_binary << 1;
    dest_binary[0] = last_bit;
    dest = dest_binary.to_ulong();
    if (dest != src) break;
  }
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

Packet* TrafficManager::bittranspose_mess() {
  int src, dest;
  int core_number = network->chips_[0]->number_cores_ * 9;
  // int core_number = network->num_cores_;
  int bits = floor(log2(core_number));
  int core_per_chip = network->chips_[0]->number_cores_;
  while (true) {
    src = gen() % core_number;
    boost::dynamic_bitset<> src_binary(bits, src);
    boost::dynamic_bitset<> dest_binary(bits);
    for (int i = 0; i < bits; ++i) {
      dest_binary[i] = src_binary[(i + bits / 2) % bits];
    }
    dest = dest_binary.to_ulong();
    if (dest != src) break;
  }
  return new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                    NodeID(dest % core_per_chip, dest / core_per_chip), message_length_);
}

Packet* TrafficManager::adversarial_mess() {
  int src, dest;
  int core_number = network->num_cores_;
  int core_per_chip = network->chips_[0]->number_cores_;
  src = gen() % core_number;
  int src_group = src / 36;
  dest = ((src_group + 1) * 36 + gen() % 36) % core_number;
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
  double message_per_cycle = injection_rate_ * network->num_cores_ / param->packet_length;
  int core_number = network->num_cores_;
  int core_per_chip = network->chips_[0]->number_cores_;
  int src, dest1, dest2;
  for (pkt_for_injection_ += message_per_cycle; pkt_for_injection_ > core_number * 2;
       pkt_for_injection_ -= core_number * 2) {
    for (int n = 0; n < core_number; n++) {
      src = n;
      dest1 = (n - 1 + core_number) % core_number;
      dest2 = (n + 1) % core_number;
      Packet* mess =
          new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                     NodeID(dest1 % core_per_chip, dest1 / core_per_chip), message_length_);
      packets.push_back(mess);
      mess = new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                        NodeID(dest2 % core_per_chip, dest2 / core_per_chip), message_length_);
      packets.push_back(mess);
      all_message_num_ += 2;
      // all_message_num_ ++;
    }
  }
}

void TrafficManager::netrace(std::vector<Packet*>& vecmess, uint64_t cyc) {
  int src, dest;
  static int core_per_chip = network->chips_[0]->number_cores_;
  static nt_packet_t* trace_packet = nullptr;
  if (cyc > CTX->input_trheader->num_cycles)
    return;
  else if ((cyc+1) % 100000000 == 0) {
    print_statistics();
  }
  while ((CTX->latest_active_packet_cycle == cyc)) {
    trace_packet = nt_read_packet(CTX);
    if (trace_packet==nullptr || nt_get_packet_size(trace_packet) == -1) {
      nt_packet_free(trace_packet);
      continue;
    }
    else if (all_message_num_ % 100000 ==0 ) nt_print_packet(trace_packet);
    src = trace_packet->src;
    dest = trace_packet->dst;
    if (src != dest) {
      int packet_length = nt_get_packet_size(trace_packet) / 8;  // 8B Bus width
      Packet* packet =
          new Packet(NodeID(src % core_per_chip, src / core_per_chip),
                     NodeID(dest % core_per_chip, dest / core_per_chip), packet_length);
      vecmess.push_back(packet);
      all_message_num_++;
    }
    // Get another packet from trace
    nt_packet_free(trace_packet);
  }
}
