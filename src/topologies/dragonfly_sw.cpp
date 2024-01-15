#include "dragonfly_sw.h"

ChipSwitch::ChipSwitch(int sw_radix, int num_core, int vc_num, int buffer_size, Channel ch) {
  switch_radix_ = sw_radix;
  number_cores_ = num_core;
  number_nodes_ = number_cores_ + 1;
  group_id_ = 0;
  nodes_.reserve(number_nodes_);
  for (int i = 0; i < number_cores_; i++) {
    nodes_.push_back(new Node(1, vc_num, buffer_size, ch));
  }
  nodes_.push_back(new Node(switch_radix_, vc_num, buffer_size, ch));
}

ChipSwitch::~ChipSwitch() {
  for (auto node : nodes_) delete node;
  nodes_.clear();
}

void ChipSwitch::set_chip(System* dragonfly, int switch_id) {
  Chip::set_chip(dragonfly, switch_id);
  group_id_ = switch_id / static_cast<DragonflySW*>(dragonfly)->sw_per_group_;
  Node* sw = get_node(number_cores_);
  NodeID sw_id = NodeID(number_cores_, switch_id);
  // connect cores to switch
  for (int i = 0; i < number_cores_; i++) {
    Node* core = get_node(i);
    NodeID core_id = NodeID(i, switch_id);
    core->link_nodes_[0] = sw_id;
    core->link_buffers_[0] = sw->in_buffers_[i];
    sw->link_nodes_[i] = core_id;
    sw->link_buffers_[i] = core->in_buffers_[0];
  }
}

DragonflySW::DragonflySW() : num_switch_(num_chips_), switches_(chips_) {
  read_config();
  cores_per_sw_ = sw_radix_ / 4;
  l_ports_per_sw_ = sw_radix_ / 2 - 1;
  if (fully_use_ports_) {
    g_ports_per_sw_ = sw_radix_ - cores_per_sw_ - l_ports_per_sw_;
  } else {
    g_ports_per_sw_ = sw_radix_ - cores_per_sw_ - l_ports_per_sw_ - 1;
  }
  sw_per_group_ = l_ports_per_sw_ + 1;
  g_ports_per_group_ = g_ports_per_sw_ * sw_per_group_;
  num_group_ = g_ports_per_group_ + 1;
  num_switch_ = num_group_ * sw_per_group_;
  num_cores_ = num_switch_ * cores_per_sw_;
  num_nodes_ = num_switch_ * (cores_per_sw_ + 1);
  assert(param->vc_number >= 2);
  std::cout << "n_per_s:" << cores_per_sw_ << " s_per_g:" << sw_per_group_ << " g:" << num_group_
            << " num_cores:" << num_cores_ << std::endl;
  switches_.reserve(num_switch_);
  for (int sw_id = 0; sw_id < num_switch_; sw_id++) {
    switches_.push_back(new ChipSwitch(sw_radix_, cores_per_sw_, param->vc_number,
                                       param->buffer_size, physical_channel_));
    switches_[sw_id]->set_chip(this, sw_id);
  }
  connect_local();
  connect_global();
}

DragonflySW::~DragonflySW() {
  for (auto sw : switches_) delete sw;
  switches_.clear();
}

void DragonflySW::read_config() {
  sw_radix_ = param->params_ptree.get<int>("Network.sw_radix", 16);
  algorithm_ = param->params_ptree.get<std::string>("Network.routing_algorithm", "MIN");
  fully_use_ports_ = param->params_ptree.get<bool>("Network.fully_use_ports", false);
  int latency = param->params_ptree.get<int>("Network.channel_latency", 4);
  physical_channel_ = Channel(1, latency);
}

void DragonflySW::connect_local() {
  for (int group_id = 0; group_id < num_group_; group_id++) {  // For each group
    // step 1:
    for (int i = 0; i < (sw_per_group_ - 1); i++) {
      Port port1 = get_port(group_id * sw_per_group_ + i, sw_radix_ - 1);
      Port port2 = get_port(group_id * sw_per_group_ + i + 1, cores_per_sw_);
      Port::connect(port1, port2);
      if (group_id == 0) {
        local_link_map_.insert({{i, i + 1}, sw_radix_ - 1});
        local_link_map_.insert({{i + 1, i}, cores_per_sw_});
      }
    }
    // step 2:
    for (int i = 0; i < (sw_per_group_ - 2); i++) {
      for (int j = i + 2; j < sw_per_group_; j++) {
        // int highest_port = sw_radix_ - (j - i);
        // int lowest_port = cores_per_sw_ + (i + 1);
        Port port1 = get_port(group_id * sw_per_group_ + i, sw_radix_ - (j - i));
        Port port2 = get_port(group_id * sw_per_group_ + j, cores_per_sw_ + (i + 1));
        Port::connect(port1, port2);
        if (group_id == 0) {
          local_link_map_.insert({{i, j}, sw_radix_ - (j - i)});
          local_link_map_.insert({{j, i}, cores_per_sw_ + (i + 1)});
        }
      }
    }
  }
}

void DragonflySW::connect_global() {
  // step 1:
  for (int i = 0; i < (num_group_ - 1); i++) {
    int sw_id_in_group_1, sw_id_in_group_2;
    int port_id_1, port_id_2;
    std::tie(sw_id_in_group_1, port_id_1) = global_port_id_to_port_id(g_ports_per_group_ - 1);
    std::tie(sw_id_in_group_2, port_id_2) = global_port_id_to_port_id(0);
    Port port1 = get_port(i * sw_per_group_ + sw_id_in_group_1, port_id_1);
    Port port2 = get_port((i + 1) * sw_per_group_ + sw_id_in_group_2, port_id_2);
    Port::connect(port1, port2);
    global_link_map_.insert({std::make_pair(i, i + 1), port1});
    global_link_map_.insert({std::make_pair(i + 1, i), port2});
  }
  // step 2:
  for (int i = 0; i < (num_group_ - 2); i++) {
    for (int j = i + 2; j < num_group_; j++) {
      int sw_id_in_group_1, sw_id_in_group_2;
      int port_id_1, port_id_2;
      std::tie(sw_id_in_group_1, port_id_1) =
          global_port_id_to_port_id(g_ports_per_group_ - (j - i));
      std::tie(sw_id_in_group_2, port_id_2) = global_port_id_to_port_id(i + 1);
      Port port1 = get_port(i * sw_per_group_ + sw_id_in_group_1, port_id_1);
      Port port2 = get_port(j * sw_per_group_ + sw_id_in_group_2, port_id_2);
      Port::connect(port1, port2);
      global_link_map_.insert({std::make_pair(i, j), port1});
      global_link_map_.insert({std::make_pair(j, i), port2});
    }
  }
}

void DragonflySW::routing_algorithm(Packet& s) const {
  if (algorithm_ == "MIN")
    MIN_routing(s);
  else
    std::cerr << "Unknown routing algorithm: " << algorithm_ << std::endl;
}

void DragonflySW::MIN_routing(Packet& s) const {
  Node* current = get_node(s.head_trace().id);
  Node* destination = get_node(s.destination_);

  ChipSwitch* current_sw = get_switch(s.head_trace().id);
  ChipSwitch* dest_sw = get_switch(s.destination_);

  if (current->id_.node_id != cores_per_sw_) {  // current node is core
    VCInfo vc(current->link_buffers_[0], 0);
    s.candidate_channels_.push_back(vc);
    vc = VCInfo(current->link_buffers_[0], 1);
    s.candidate_channels_.push_back(vc);
  }
  // current node is switch
  else if (current_sw->chip_id_ == dest_sw->chip_id_) {  // within the switch
    VCInfo vc(current->link_buffers_[destination->id_.node_id], 0);
    s.candidate_channels_.push_back(vc);
  } else if (current_sw->group_id_ == dest_sw->group_id_) {  // within the group
    int current_sw_id_in_group = current_sw->chip_id_ % sw_per_group_;
    int dest_sw_id_in_group = dest_sw->chip_id_ % sw_per_group_;
    int port_id = local_link_map_.at(std::make_pair(current_sw_id_in_group, dest_sw_id_in_group));
    VCInfo vc(current->link_buffers_[port_id], 1);
    s.candidate_channels_.push_back(vc);
  } else {  // global
    int current_group_id = current_sw->group_id_;
    int dest_group_id = dest_sw->group_id_;
    // mis-routing
    // int source_group_id = get_switch(s.source_)->group_id_;
    // if (current_group_id == source_group_id) {
    //  int sw_id_in_group = current_sw->chip_id_ % sw_per_group_;
    //  int lowest_global_port_id = cores_per_sw_ + sw_id_in_group;
    //  VCInfo vc(current->link_buffers_[lowest_global_port_id + s.source_.node_id], 0);
    //  s.candidate_channels_.push_back(vc);
    //  return;
    //}
    // std::cout << current_group_id << " " << dest_group_id << std::endl;
    Port global_port = global_link_map_.at(std::make_pair(current_group_id, dest_group_id));
    if (current->id_ == global_port.node_id) {  // the global link is at current switch
      VCInfo vc(global_port.link_buffer, 0);
      s.candidate_channels_.push_back(vc);
    } else {
      ChipSwitch* global_sw = static_cast<ChipSwitch*>(get_chip(global_port.node_id.chip_id));
      int current_sw_id_in_group = current_sw->chip_id_ % sw_per_group_;
      int global_sw_id_in_group = global_sw->chip_id_ % sw_per_group_;
      int port_id =
          local_link_map_.at(std::make_pair(current_sw_id_in_group, global_sw_id_in_group));
      VCInfo vc(current->link_buffers_[port_id], 0);
      s.candidate_channels_.push_back(vc);
    }
  }
}

std::pair<int, int> DragonflySW::global_port_id_to_port_id(int global_port_id) {
  int sw_id_in_group = global_port_id / g_ports_per_sw_;
  // port id for the lowest global port in the switch
  int lowest_id = cores_per_sw_ + sw_id_in_group;
  int port_id = lowest_id + global_port_id % g_ports_per_sw_;
  return std::make_pair(sw_id_in_group, port_id);
}
