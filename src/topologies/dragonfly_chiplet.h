#pragma once
#include "system.h"

class CGroup;
class DragonflyChiplet;

class NodeInCG : public Node {
 public:
  NodeInCG(int k_chiplet, int vc_num, int buffer_size, Channel internal_channel,
           Channel external_channel);
  void set_node(Chip* cgroup, NodeID id) override;
  CGroup* cgroup_;
  int& node_id_in_cg_;
  int x_, y_;      // coodinate with the chip
  int k_chiplet_;  // 2D-mesh of k_chiplet_ * k_chiplet_

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
};

class CGroup : public Chip {
 public:
  CGroup(int k_chiplet, int cgroup_radix, int vc_num, int buffer_size, Channel internal_channel,
         Channel external_channel);
  ~CGroup();

  void set_chip(System* dragonfly, int Cgroup_id) override;
  inline NodeInCG* get_node(int chiplet_id) {
    return static_cast<NodeInCG*>(Chip::get_node(NodeID(chiplet_id)));
  }
  inline NodeInCG* get_node(NodeID id) { return static_cast<NodeInCG*>(Chip::get_node(id)); }

  DragonflyChiplet* dragonfly_;
  int& num_chiplets_;
  int k_node_;
  int cgroup_radix_;
  int& cgroup_id_;
  int wgroup_id_;
};

class DragonflyChiplet : public System {
 public:
  // port_id in C-Group
  struct PortID {
    PortID() {}
    PortID(int port_id_, int chiplet_id_) {
      port_id = port_id_;
      chiplet_id = chiplet_id_;
    }
    int port_id;
    int chiplet_id;  // node_id
  };
  DragonflyChiplet();
  ~DragonflyChiplet();

  void read_config() override;

  void connect_local();
  void connect_global();

  void routing_algorithm(Packet& s) const override;
  void MIN_routing(Packet& s) const;
  void XY_routing(Packet& s, NodeID dest) const;

  inline NodeInCG* get_node(NodeID id) const {
    return static_cast<NodeInCG*>(System::get_node(id));
  }
  inline CGroup* get_cgroup(NodeID id) const { return static_cast<CGroup*>(get_chip(id.chip_id)); }
  inline Port get_port(int cgroup_id, int node_id) const {
    Node* chiplet = get_node(NodeID(node_id, cgroup_id));
    return chiplet->ports_[4];
  }
  // inline PortID get_port_id(int port_id) {
  //   int i = port_id / cgroup_radix_;
  //   int chiplet_id;
  //   if (i < k_chiplet_) {
  //     chiplet_id = i;
  //   } else if (i >= k_chiplet_ && i < (3*k_chiplet_ -4))

  //}

  // PortID local_port_id_to_port_id(int local_port_id);
  //   <cgroup_id_w_group, node_id>
  std::pair<int, int> global_port_id_to_port_id(int global_port_id);

  std::string algorithm_;

  int k_node_in_CG_;
  Channel internal_channel_;
  Channel external_channel_;
  int cgroup_radix_;
  int num_nodes_per_cg_;
  int l_ports_per_cg_;
  int g_ports_per_cg_;
  int g_ports_per_wg_;
  int cgroup_per_wgroup_;
  int num_wgroup_;
  int& num_cgroup_;

  // <port_id, node_id>
  std::map<int, int> port_node_map_;
  // <src_cgroup_id_in_wgroup, dest_cgroup_id_in_wgroup> -> node_id
  std::map<std::pair<int, int>, int> local_link_map_;
  // <src_group_id, dest_group_id> -> port
  std::map<std::pair<int, int>, Port> global_link_map_;

  std::vector<Chip*>& cgroups_;
};
