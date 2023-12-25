#include "node.h"

std::ostream& operator<<(std::ostream& s, const NodeID& id) {
  s << "NodeID:" << id.node_id << " ChipID:" << id.chip_id;
  return s;
}

Node::Node(int radix, int vc_num, int buffer_size, Channel channel) {
  chip_ = nullptr;
  id_ = NodeID();
  radix_ = radix;
  in_buffers_.resize(radix_);
  link_nodes_.resize(radix_);
  link_buffers_.resize(radix_);
  ports_.reserve(radix_);
  for (int i = 0; i < radix_; i++) {
    in_buffers_[i] = new Buffer(this, vc_num, buffer_size, channel);
    link_nodes_[i] = NodeID();
    link_buffers_[i] = nullptr;
    ports_.push_back(Port{id_, in_buffers_[i], link_nodes_[i], link_buffers_[i]});
  }
}

Node::~Node() {
  for (auto in_buffer : in_buffers_) {
    delete in_buffer;
  }
  in_buffers_.clear();
}

void Node::set_node(Chip* chip, NodeID id) {
  chip_ = chip;
  id_ = id;
}

void Node::clear_all() {
  for (auto in_buffer : in_buffers_) {
    in_buffer->clear_buffer();
  }
}
