#pragma once
#include "node.h"

class System;

class Chip {
 public:
  Chip();
  ~Chip();

  virtual void set_chip(System* system, int chip_id_);
  void clear_all();

  virtual inline Node* get_node(int node_id) { return nodes_[node_id]; }
  virtual inline Node* get_node(NodeID id) { return nodes_[id.node_id]; }

  System* system_;  // Point to the upper level group

  int chip_id_;
  std::vector<int> chip_coordinate_;
  int number_nodes_;
  int number_cores_;

  std::vector<Node*> nodes_;
};
