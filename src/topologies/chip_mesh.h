#pragma once
#include "chip.h"
#include "node.h"

class Chip;

class NodeMesh : public Node {
 public:
  NodeMesh(int k_node, int vc_num, int buffer_size);

  void set_node(Chip* chip, NodeID id) override;

  // Chip* chip_;  // point to the chip where the node is located
  int x_, y_;   // coodinate with the chip
  int k_node_;  // number of nodes in a row/column

  // Input buffers for the on-chip 2D-mesh
  Buffer*& xneg_in_buffer_;
  Buffer*& xpos_in_buffer_;
  Buffer*& yneg_in_buffer_;
  Buffer*& ypos_in_buffer_;

  // ID of the node to which the output port goes.
  NodeID& xneg_link_node_;
  NodeID& xpos_link_node_;
  NodeID& yneg_link_node_;
  NodeID& ypos_link_node_;

  // Point to input buffer connected to the output port.
  Buffer*& xneg_link_buffer_;
  Buffer*& xpos_link_buffer_;
  Buffer*& yneg_link_buffer_;
  Buffer*& ypos_link_buffer_;
};

class ChipMesh : public Chip {
 public:
  ChipMesh(int k_node, int vc_num, int buffer_size);
  ~ChipMesh();
  void set_chip(System* system, int chip_id) override;
  inline NodeMesh* get_node(int node_id) const override {
    return static_cast<NodeMesh*>(nodes_[node_id]);
  }
  inline NodeMesh* get_node(NodeID id) const override {
    return static_cast<NodeMesh*>(nodes_[id.node_id]);
  }

  int k_node_;
  std::vector<int> chip_coordinate_;
};
