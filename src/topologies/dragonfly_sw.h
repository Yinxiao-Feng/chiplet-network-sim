#pragma once
#include "system.h"

//struct Port {
//  NodeID switch_id;
//  int port_id;
//};

class ChipSwitch : public Chip {
 public:
  ChipSwitch(int sw_radix, int num_core, int vc_num, int buffer_size);
  ~ChipSwitch();

  void set_chip(System* dragonfly, int switch_id) override;

  int switch_radix_;
  //int num_cores_;
  int group_id_;

  // Nodes 0 - (num_cores_-1): cores
  // Node num_cores_: switch
};

class DragonflySW : public System {
 public:
  DragonflySW();
  ~DragonflySW();

  void connect_local();
  void connect_global();

  void routing_algorithm(Packet& s) override;
  void MIN_routing(Packet& s);

  inline ChipSwitch* get_switch(NodeID id) {
    return static_cast<ChipSwitch*>(get_chip(id.chip_id));
  }
  inline Port get_port(int switch_id, int port_id) {
    Node* sw = get_node(NodeID(cores_per_sw_, switch_id));
    return sw->ports_[port_id];
  }

  //Port get_global_port(int group_id, int global_port_id);
  // <switch_id_in_group, port_id>
  std::pair<int, int> global_port_id_to_port_id(int global_port_id);

  std::string algorithm_;

  int sw_radix_;
  int cores_per_sw_;
  int l_ports_per_sw_;
  int g_ports_per_sw_;
  int g_ports_per_group_;
  int sw_per_group_;
  int num_group_;
  int& num_switch_;

  // <src_sw_id_in_group, dest_sw_id_in_group> -> port_id
  std::map<std::pair<int, int>, int> local_link_map_;
  // <src_group_id, dest_group_id> -> port
  std::map<std::pair<int, int>, Port> global_link_map_;

  std::vector<Chip*>& switches_;
};
