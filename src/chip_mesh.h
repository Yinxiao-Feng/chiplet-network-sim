#pragma once
#include "chip.h"
#include "node_mesh.h"

class ChipMesh : public Chip {
public:
    ChipMesh(int k_node, int vc_num, int buffer_size);
	~ChipMesh();
	void set_chip(System* system, int chip_id) override;
	inline NodeMesh* get_node(int node_id) override {
		return static_cast<NodeMesh*>(nodes_[node_id]);
	}
	inline NodeMesh* get_node(NodeID id) override {
		return static_cast<NodeMesh*>(nodes_[id.node_id]);
	}

	int k_node_;
};
