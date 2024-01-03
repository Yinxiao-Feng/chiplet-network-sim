#pragma once
#include "chip_mesh.h"
#include "system.h"

class SingleChipMesh : public System {
 public:
  SingleChipMesh();
  ~SingleChipMesh();

  inline NodeMesh* get_node(NodeID id) const override {
    return dynamic_cast<NodeMesh*>(System::get_node(id));
  }

  void routing_algorithm(Packet& s) override;
  void XY_routing(Packet& s) const;
  void NFR_routing(Packet& s) const;

  std::string algorithm_;

  int k_node_;
};
