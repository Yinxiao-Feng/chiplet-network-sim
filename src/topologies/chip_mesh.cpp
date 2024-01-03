#include "chip_mesh.h"

ChipMesh::ChipMesh(int k_node, int vc_num, int buffer_size)
{
    k_node_ = k_node;
    number_nodes_ = k_node_ * k_node_;
    number_cores_ = number_nodes_;
    //port_number_ = Knode_ * 4 - 4;
    nodes_.reserve(number_nodes_);
    for (int node_id = 0; node_id < number_nodes_; node_id++) {
        nodes_.push_back(new NodeMesh(k_node_, vc_num, buffer_size));
    }
}

ChipMesh::~ChipMesh() {
	for (auto node : nodes_) {
		delete node;
	}
	nodes_.clear();
}

void ChipMesh::set_chip(System* system, int chip_id) {
    Chip::set_chip(system, chip_id);
    for (int node_id = 0; node_id < number_nodes_; node_id++) {
        NodeMesh* node = get_node(node_id);
        if (node->x_ != 0) {
            node->xneg_link_node_ = NodeID(node_id - 1, chip_id);
            node->xneg_link_buffer_ = get_node(node->xneg_link_node_)->xpos_in_buffer_;
        }
        if (node->x_ != k_node_ - 1) {
            node->xpos_link_node_ = NodeID(node_id + 1, chip_id);
            node->xpos_link_buffer_ = get_node(node->xpos_link_node_)->xneg_in_buffer_;
        }
        if (node->y_ != 0) {
            node->yneg_link_node_ = NodeID(node_id - k_node_, chip_id);
            node->yneg_link_buffer_ = get_node(node->yneg_link_node_)->ypos_in_buffer_;
        }
        if (node->y_ != k_node_ - 1) {
            node->ypos_link_node_ = NodeID(node_id + k_node_, chip_id);
            node->ypos_link_buffer_ = get_node(node->ypos_link_node_)->yneg_in_buffer_;
        }
    }
}