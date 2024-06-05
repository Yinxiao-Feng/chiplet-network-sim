#include "chip_mesh.h"

NodeMesh::NodeMesh(int k_node, int vc_num, int buffer_size)
    : Node(4, vc_num, buffer_size),
      xneg_in_buffer_(in_buffers_[0]),
      xpos_in_buffer_(in_buffers_[1]),
      yneg_in_buffer_(in_buffers_[2]),
      ypos_in_buffer_(in_buffers_[3]),
      xneg_link_node_(link_nodes_[0]),
      xpos_link_node_(link_nodes_[1]),
      yneg_link_node_(link_nodes_[2]),
      ypos_link_node_(link_nodes_[3]),
      xneg_link_buffer_(link_buffers_[0]),
      xpos_link_buffer_(link_buffers_[1]),
      yneg_link_buffer_(link_buffers_[2]),
      ypos_link_buffer_(link_buffers_[3]) {
  x_ = 0;
  y_ = 0;
  k_node_ = k_node;
}

void NodeMesh::set_node(Chip* chip, NodeID id) {
  chip_ = chip;
  id_ = id;
  x_ = id.node_id % k_node_;
  y_ = id.node_id / k_node_;
}

ChipMesh::ChipMesh(int k_node, int vc_num, int buffer_size) {
  k_node_ = k_node;
  number_nodes_ = k_node_ * k_node_;
  number_cores_ = number_nodes_;
  // port_number_ = Knode_ * 4 - 4;
  nodes_.reserve(number_nodes_);
  chip_coordinate_.resize(2);
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