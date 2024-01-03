#include "node_mesh.h"
#include "chip_mesh.h"

NodeMesh::NodeMesh(int k_node, int vc_num, int buffer_size)
    : Node(4, vc_num, buffer_size), xneg_in_buffer_(in_buffers_[0]),
      xpos_in_buffer_(in_buffers_[1]),
      yneg_in_buffer_(in_buffers_[2]), ypos_in_buffer_(in_buffers_[3]),
      xneg_link_node_(link_nodes_[0]), xpos_link_node_(link_nodes_[1]),
      yneg_link_node_(link_nodes_[2]), ypos_link_node_(link_nodes_[3]),
      xneg_link_buffer_(link_buffers_[0]), xpos_link_buffer_(link_buffers_[1]),
      yneg_link_buffer_(link_buffers_[2]), ypos_link_buffer_(link_buffers_[3]),
      xneg_port_(ports_[0]),
      xpos_port_(ports_[1]),
      yneg_port_(ports_[2]),
      ypos_port_(ports_[3]) {
    x_ = 0;
    y_ = 0;
    k_node_ = k_node;
}

void NodeMesh::set_node(Chip *chip, NodeID id)
{
    chip_ = chip;
    id_ = id;
    x_ = id.node_id % k_node_;
    y_ = id.node_id / k_node_;
}
