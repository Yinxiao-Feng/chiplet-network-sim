#pragma once
#include "chip_mesh.h"
#include "system.h"

class SingleChipMesh : public System {
  enum class Algorithm { XY, NFR };

 public:
  SingleChipMesh();
  ~SingleChipMesh();

  inline NodeMesh* get_node(NodeID id) const override {
    return dynamic_cast<NodeMesh*>(System::get_node(id));
  }

  void routing_algorithm(Packet& s) override;
  void XY_routing(Packet& s);

  Algorithm algorithm_;

  int k_node_;
};
