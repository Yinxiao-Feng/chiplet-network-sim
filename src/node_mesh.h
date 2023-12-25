#pragma once
#include "node.h"

class Chip;

class NodeMesh : public Node {
public:
    NodeMesh(int k_node, int vc_num, int buffer_size);

	void set_node(Chip* chip, NodeID id) override;

	//Chip* chip_;  // point to the chip where the node is located
	int x_, y_;   // coodinate with the chip
	int k_node_;   // number of nodes in a row/column

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

	Port& xneg_port_;
	Port& xpos_port_;
	Port& yneg_port_;
	Port& ypos_port_;
};

