#pragma once
#include "node.h"


class Chip {
 public:
  Chip();
  ~Chip();

  virtual void set_chip(System* system, int chip_id_);
  void reset();

  virtual inline Node* get_node(int node_id) const { return nodes_[node_id]; }
  virtual inline Node* get_node(NodeID id) const  { return nodes_[id.node_id]; }

  System* system_;  // Point to the upper level group

  int chip_id_;
  std::vector<int> chip_coordinate_;
  int number_nodes_;
  int number_cores_;

  friend TrafficManager;
 protected:
  std::vector<Node*> nodes_;
};
