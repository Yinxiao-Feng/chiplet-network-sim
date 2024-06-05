#pragma once
#include "chip_mesh.h"
#include "system.h"

class MultiChipMesh : public System {
 public:
  MultiChipMesh();
  ~MultiChipMesh();

  void read_config() override;

  inline NodeID id2nodeid(int id) const override {
    int K = k_node_ * k_chip_;
    int x = id % K;
    int y = id / K;
    int node_id = x % k_node_ + (y % k_node_) * k_node_;
    int chip_id = x / k_node_ + (y / k_node_) * k_chip_;
    return NodeID(node_id, chip_id);
  }
  inline NodeMesh* get_node(NodeID id) const override {
    return dynamic_cast<NodeMesh*>(System::get_node(id));
  }
  inline ChipMesh* get_chip(int chip_id) const override {
    return dynamic_cast<ChipMesh*>(chips_[chip_id]);
  }
  inline ChipMesh* get_chip(NodeID id) const override {
    return dynamic_cast<ChipMesh*>(chips_[id.chip_id]);
  }
  void connect_chiplets();

  void routing_algorithm(Packet& s) const override;
  void XY_routing(Packet& s) const;
  void NFR_routing(Packet& s) const;
  void NFR_adaptive_routing(Packet& s) const;

  std::string algorithm_;

  int k_node_;
  int k_chip_;

  std::string d2d_IF_;
};