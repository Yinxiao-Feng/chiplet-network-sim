#pragma once
#include "chip_mesh.h"
#include "system.h"

class MultiChipTorus : public System {
 public:
  MultiChipTorus();
  ~MultiChipTorus();

  void read_config() override;

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
  void clue_routing(Packet& s) const;

  std::string algorithm_;

  int k_node_;
  int k_chip_;
};