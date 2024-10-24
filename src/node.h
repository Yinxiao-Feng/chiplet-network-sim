#pragma once

#include "config.h"

class Buffer;
class Chip;

struct NodeID {
  explicit NodeID(int nodeid = -1, int chipid = -1) {
    node_id = nodeid;
    chip_id = chipid;
    // coordinate = std::vector<int>();
  }
  int node_id;
  int chip_id;
  // std::vector<int> coordinate;
  inline bool operator==(const NodeID& id) const {
    return (node_id == id.node_id && chip_id == id.chip_id);
  };
  inline bool operator!=(const NodeID& id) const {
    return (node_id != id.node_id || chip_id != id.chip_id);
  }
  friend std::ostream& operator<<(std::ostream& s, const NodeID& id);
};

struct Port {
  NodeID& node_id;
  Buffer*& in_buffer;
  NodeID& link_node;
  Buffer*& link_buffer;
  static void connect(Port& p1, Port& p2) {
    p1.link_node = p2.node_id;
    p1.link_buffer = p2.in_buffer;
    p2.link_node = p1.node_id;
    p2.link_buffer = p1.in_buffer;
  }
};

class Node {
 public:
  Node(int radix, int vc_num, int buffer_size, Channel channel = on_chip_channel);
  ~Node();

  virtual void set_node(Chip* chip, NodeID id);
  void reset();

  NodeID id_;
  Chip* chip_;
  int radix_;

  // Input buffers for the on-chip 2D-mesh
  std::vector<Buffer*> in_buffers_;

  // ID of the node to which the output port goes.
  std::vector<NodeID> link_nodes_;

  // Point to input buffer connected to the output port.
  std::vector<Buffer*> link_buffers_;

  std::vector<Port> ports_;
};
