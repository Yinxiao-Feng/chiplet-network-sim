#pragma once
#include "chip.h"
#include "packet.h"

class TrafficManager;

class System {
 public:
  System();
  static System* New(const std::string&);
  virtual ~System() {}
  virtual void reset();
  virtual void read_config() = 0;
  void update(Packet& s);
  void onestage(Packet& s);
  void twostage(Packet& s);
  void Threestage(Packet& s);
  void routing(Packet& s) const;
  virtual void routing_algorithm(Packet& s) const = 0;
  void vc_allocate(Packet& s) const;
  void switch_allocate(Packet& s);
  virtual NodeID id2nodeid(int id) const {
    int node_id = id % chips_[0]->number_cores_;
    int chip_id = id / chips_[0]->number_cores_;
    return NodeID(node_id, chip_id);
  }
  virtual inline Chip* get_chip(int chip_id) const { return chips_[chip_id]; }
  virtual inline Chip* get_chip(NodeID id) const { return chips_[id.chip_id]; }
  virtual inline Node* get_node(NodeID id) const {
    return chips_[id.chip_id]->get_node(id.node_id);
  }

  int num_chips_;
  int num_nodes_;  // some nodes are only for routing, no packets injected
  int num_cores_;

  // router parameters
  int routing_time_;
  int vc_allocating_time_;
  std::string router_stages_;

  // simulation parameters
  int timeout_time_;

  friend TrafficManager;

 protected:
  std::vector<Chip*> chips_;
};
