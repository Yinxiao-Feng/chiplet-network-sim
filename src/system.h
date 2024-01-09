#pragma once
#include "chip.h"
#include "packet.h"

class System {
 public:
  System();
  static System* New(const std::string&);
  virtual ~System() {}
  virtual void reset();
  // Message* genMes();
  void update(Packet& s);
  void onestage(Packet& s);
  void twostage(Packet& s);
  void Threestage(Packet& s);
  void routing(Packet& s);
  virtual void routing_algorithm(Packet& s) = 0;
  void vc_allocate(Packet& s) const;
  void switch_allocate(Packet& s);
  // void transmit_head(Packet& s);
  virtual inline Chip* get_chip(int chip_id) const { return chips_[chip_id]; }
  virtual inline Chip* get_chip(NodeID id) const { return chips_[id.chip_id]; }
  virtual inline Node* get_node(NodeID id) const {
    return chips_[id.chip_id]->get_node(id.node_id);
  }

  int num_chips_;
  int num_nodes_;  // some nodes are only for routing, no packets injected
  int num_cores_;

  std::vector<Chip*> chips_;

  // router parameters
  int routing_time_;
  int vc_allocating_time_;
  int sw_allocating_time_;
  std::string router_stages_;

  // simulation parameters
  int timeout_time_;
};
