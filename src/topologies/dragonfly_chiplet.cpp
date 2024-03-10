#include "dragonfly_chiplet.h"

NodeInCG::NodeInCG(int k_chiplet, int vc_num, int buffer_size, Channel internal_channel,
                   Channel external_channel)
    : Node(5, vc_num, buffer_size),
      node_id_in_cg_(id_.node_id),
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
  cgroup_ = nullptr;
  k_chiplet_ = k_chiplet;
  x_ = 0;
  y_ = 0;
  for (int i = 0; i < 4; i++) {
    in_buffers_[i]->channel_ = internal_channel;
  }
  in_buffers_[4]->channel_ = external_channel;
}

void NodeInCG::set_node(Chip* cgroup, NodeID id) {
  assert(cgroup != nullptr);
  Node::set_node(cgroup, id);
  cgroup_ = dynamic_cast<CGroup*>(chip_);
  x_ = id.node_id % k_chiplet_;
  y_ = id.node_id / k_chiplet_;
}

CGroup::CGroup(int k_chiplet, int cgroup_radix, int vc_num, int buffer_size,
               Channel internal_channel, Channel external_channel)
    : num_chiplets_(number_nodes_), cgroup_id_(chip_id_) {
  k_node_ = k_chiplet;
  cgroup_radix_ = cgroup_radix;
  num_chiplets_ = k_chiplet * k_chiplet;
  number_cores_ = num_chiplets_;
  wgroup_id_ = 0;
  dragonfly_ = nullptr;
  nodes_.reserve(num_chiplets_);
  for (int i = 0; i < num_chiplets_; i++) {
    nodes_.push_back(
        new NodeInCG(k_chiplet, vc_num, buffer_size, internal_channel, external_channel));
  }
}

CGroup::~CGroup() {
  for (auto chiplet : nodes_) delete chiplet;
  nodes_.clear();
}

void CGroup::set_chip(System* dragonfly, int cgroup_id) {
  Chip::set_chip(dragonfly, cgroup_id);
  dragonfly_ = dynamic_cast<DragonflyChiplet*>(system_);
  wgroup_id_ = cgroup_id / dragonfly_->cgroup_per_wgroup_;
  // 2D-mesh
  for (int node_id = 0; node_id < number_nodes_; node_id++) {
    NodeInCG* node = get_node(node_id);
    if (node->x_ != 0) {
      node->xneg_link_node_ = NodeID(node_id - 1, cgroup_id);
      node->xneg_link_buffer_ = get_node(node->xneg_link_node_)->xpos_in_buffer_;
    }
    if (node->x_ != k_node_ - 1) {
      node->xpos_link_node_ = NodeID(node_id + 1, cgroup_id);
      node->xpos_link_buffer_ = get_node(node->xpos_link_node_)->xneg_in_buffer_;
    }
    if (node->y_ != 0) {
      node->yneg_link_node_ = NodeID(node_id - k_node_, cgroup_id);
      node->yneg_link_buffer_ = get_node(node->yneg_link_node_)->ypos_in_buffer_;
    }
    if (node->y_ != k_node_ - 1) {
      node->ypos_link_node_ = NodeID(node_id + k_node_, cgroup_id);
      node->ypos_link_buffer_ = get_node(node->ypos_link_node_)->yneg_in_buffer_;
    }
  }
}

DragonflyChiplet::DragonflyChiplet() : num_cgroup_(num_chips_), cgroups_(chips_) {
  read_config();
  num_nodes_per_cg_ = k_node_in_CG_ * k_node_in_CG_;
  // cgroup_radix_ = 3 * chiplets_per_cg_;
  cgroup_radix_ = (4 * k_node_in_CG_ - 4);
  // ext_ports_per_chiplet_ = cgroup_radix_ / (4 * k_chiplet_ - 4);
  l_ports_per_cg_ = cgroup_radix_ / 3 * 2 - 1;
  // l_ports_per_cg_ = cgroup_radix_ / 3 * 2;
  g_ports_per_cg_ = cgroup_radix_ - l_ports_per_cg_;
  cgroup_per_wgroup_ = l_ports_per_cg_ + 1;
  g_ports_per_wg_ = g_ports_per_cg_ * cgroup_per_wgroup_;
  num_wgroup_ = g_ports_per_wg_ + 1;
  num_cgroup_ = num_wgroup_ * cgroup_per_wgroup_;
  num_cores_ = num_cgroup_ * num_nodes_per_cg_;
  num_nodes_ = num_cores_;
  std::cout << "n_per_s:" << num_nodes_per_cg_ << " s_per_g:" << cgroup_per_wgroup_
            << " g:" << num_wgroup_ << std::endl;
  cgroups_.reserve(num_cgroup_);
  for (int cgroup_id = 0; cgroup_id < num_cgroup_; cgroup_id++) {
    cgroups_.push_back(new CGroup(k_node_in_CG_, cgroup_radix_, param->vc_number,
                                  param->buffer_size, internal_channel_, external_channel_));
    cgroups_[cgroup_id]->set_chip(this, cgroup_id);
  }
  // build the map form ext_port_id to node_id
  int port_id = 0;
  for (int i = 0; i < k_node_in_CG_; i++) {  // bottom
    port_node_map_.insert({port_id, i});
    port_id += 1;
  }
  for (int i = 0; i < k_node_in_CG_ - 2; i++) {  // two side
    int left_node = (i + 1) * k_node_in_CG_;
    int right_node = (i + 1) * k_node_in_CG_ + k_node_in_CG_ - 1;
    // left side
    port_node_map_.insert({port_id, left_node});
    port_node_map_.insert({port_id + 1, right_node});
    port_id += 2;
  }
  for (int i = 0; i < k_node_in_CG_; i++) {  // top
    port_node_map_.insert({port_id, (k_node_in_CG_ - 1) * k_node_in_CG_ + i});
    port_id += 1;
  }
  connect_local();
  connect_global();
}

DragonflyChiplet::~DragonflyChiplet() {
  for (auto cgroup : cgroups_) delete cgroup;
  cgroups_.clear();
}

void DragonflyChiplet::read_config() {
  k_node_in_CG_ = param->params_ptree.get<int>("Network.k_node", 4);
  algorithm_ = param->params_ptree.get<std::string>("Network.routing_algorithm", "MIN");
  int internal_bandiwdth = param->params_ptree.get<int>("Network.internal_bandwidth", 1);
  int external_latency = param->params_ptree.get<int>("Network.external_latency", 4);
  internal_channel_ = Channel(internal_bandiwdth, 1);
  external_channel_ = Channel(1, external_latency);
  mis_routing = param->params_ptree.get<bool>("Network.mis_routing", false);
}

void DragonflyChiplet::connect_local() {
  for (int wgroup_id = 0; wgroup_id < num_wgroup_; wgroup_id++) {
    for (int i = 0; i < (cgroup_per_wgroup_ - 1); i++) {
      int node_id_1 = port_node_map_.at(cgroup_radix_ - 1);
      int node_id_2 = port_node_map_.at(0);
      Port port1 = get_port(wgroup_id * cgroup_per_wgroup_ + i, node_id_1);
      Port port2 = get_port(wgroup_id * cgroup_per_wgroup_ + i + 1, node_id_2);
      Port::connect(port1, port2);
      if (wgroup_id == 0) {
        local_link_map_.insert({{i, i + 1}, node_id_1});
        local_link_map_.insert({{i + 1, i}, node_id_2});
      }
    }
    for (int i = 0; i < cgroup_per_wgroup_ - 2; i++) {
      for (int j = i + 2; j < cgroup_per_wgroup_; j++) {
        int node_id_1 = port_node_map_.at(cgroup_radix_ - (j - i));
        int node_id_2 = port_node_map_.at(i + 1);
        Port port1 = get_port(wgroup_id * cgroup_per_wgroup_ + i, node_id_1);
        Port port2 = get_port(wgroup_id * cgroup_per_wgroup_ + j, node_id_2);
        Port::connect(port1, port2);
        if (wgroup_id == 0) {
          local_link_map_.insert({{i, j}, node_id_1});
          local_link_map_.insert({{j, i}, node_id_2});
        }
      }
    }
  }
}

void DragonflyChiplet::connect_global() {
  // step 1:
  for (int i = 0; i < (num_wgroup_ - 1); i++) {
    int cg_id_in_wg_1, cg_id_in_wg_2;
    int node_id_1, node_id_2;
    std::tie(cg_id_in_wg_1, node_id_1) = global_port_id_to_port_id(g_ports_per_wg_ - 1);
    std::tie(cg_id_in_wg_2, node_id_2) = global_port_id_to_port_id(0);
    Port port1 = get_port(i * cgroup_per_wgroup_ + cg_id_in_wg_1, node_id_1);
    Port port2 = get_port((i + 1) * cgroup_per_wgroup_ + cg_id_in_wg_2, node_id_2);
    Port::connect(port1, port2);
    global_link_map_.insert({{i, i + 1}, port1});
    global_link_map_.insert({{i + 1, i}, port2});
  }
  // step 2:
  for (int i = 0; i < (num_wgroup_ - 2); i++) {
    for (int j = i + 2; j < num_wgroup_; j++) {
      int cg_id_in_wg_1, cg_id_in_wg_2;
      int node_id_1, node_id_2;
      std::tie(cg_id_in_wg_1, node_id_1) = global_port_id_to_port_id(g_ports_per_wg_ - (j - i));
      std::tie(cg_id_in_wg_2, node_id_2) = global_port_id_to_port_id(i + 1);
      Port port1 = get_port(i * cgroup_per_wgroup_ + cg_id_in_wg_1, node_id_1);
      Port port2 = get_port(j * cgroup_per_wgroup_ + cg_id_in_wg_2, node_id_2);
      Port::connect(port1, port2);
      global_link_map_.insert({{i, j}, port1});
      global_link_map_.insert({{j, i}, port2});
    }
  }
}

void DragonflyChiplet::routing_algorithm(Packet& s) const {
  if (algorithm_ == "MIN")
    MIN_routing(s);
  else
    std::cerr << "Unknown routing algorithm: " << algorithm_ << std::endl;
}

void DragonflyChiplet::MIN_routing(Packet& s) const {
  NodeInCG* current = get_node(s.head_trace().id);
  NodeInCG* destination = get_node(s.destination_);

  CGroup* current_cgroup = current->cgroup_;
  CGroup* dest_cgroup = destination->cgroup_;

  int current_cg_id_in_wg = current_cgroup->cgroup_id_ % cgroup_per_wgroup_;
  int dest_cg_id_in_wg = dest_cgroup->cgroup_id_ % cgroup_per_wgroup_;

  if (current_cgroup->cgroup_id_ == dest_cgroup->cgroup_id_) {  // within the C-Group
    XY_routing(s, destination->id_);
  } else if (current_cgroup->wgroup_id_ == dest_cgroup->wgroup_id_) {  // within the W-Group
    int node_id = local_link_map_.at({current_cg_id_in_wg, dest_cg_id_in_wg});
    Port local_port = get_port(current_cgroup->cgroup_id_, node_id);
    if (node_id == current->node_id_in_cg_) {  // ext_port is at current chiplet
      VCInfo vc(local_port.link_buffer, 1);
      s.candidate_channels_.push_back(vc);
    } else {  // ext_port is at an other chiplet
      XY_routing(s, local_port.node_id);
    }
  } else {  // Global
    int current_wg_id = current_cgroup->wgroup_id_;
    int dest_wg_id = dest_cgroup->wgroup_id_;
    // mis-routing
    if (mis_routing) {
      CGroup* source_cg = get_node(s.source_)->cgroup_;
      int source_wg_id = source_cg->wgroup_id_;
      int src_cg_id_in_wgroup = source_cg->cgroup_id_ % cgroup_per_wgroup_;
      // port id for the lowest global port of the C-group: cg_id_in_wgroup
      if (current_wg_id == source_wg_id) {
        int leave_node_id =
            port_node_map_.at(src_cg_id_in_wgroup + s.source_.node_id % g_ports_per_cg_);
        Port misrouting_global_port = get_port(current_cgroup->cgroup_id_, leave_node_id);
        if (current->node_id_in_cg_ == leave_node_id) {
          VCInfo vc(misrouting_global_port.link_buffer, 0);
          s.candidate_channels_.push_back(vc);
        } else {
          XY_routing(s, NodeID(misrouting_global_port.node_id));
          return;
        }
        // if (current_wg_id == source_wg_id) {
        //   dest_wg_id = (dest_wg_id + s.source_.chip_id * 64 + s.source_.node_id) %
        // num_wgroup_;
        //   if (dest_wg_id == current_wg_id) dest_wg_id = (dest_wg_id + 1) %
        // num_wgroup_;
        // }
      }
    }
    Port global_port = global_link_map_.at({current_wg_id, dest_wg_id});
    if (global_port.node_id.node_id == current->node_id_in_cg_) {
      VCInfo vc(global_port.link_buffer, 0);
      s.candidate_channels_.push_back(vc);
    } else {  // the global_port is at an other chiplet
      CGroup* global_cgroup = get_cgroup(global_port.node_id);
      // the global_port is at current C-Group
      if (current_cgroup->cgroup_id_ == global_cgroup->cgroup_id_) {
        XY_routing(s, global_port.node_id);
      } else {  // the global_port is at an other C-Group, go a local link first
        // the cgroup_id_in_wgroup of the global_port
        int g_port_cg_id_in_wg = global_cgroup->cgroup_id_ % cgroup_per_wgroup_;
        int node_id = local_link_map_.at({current_cg_id_in_wg, g_port_cg_id_in_wg});
        Port local_port = get_port(current_cgroup->cgroup_id_, node_id);
        // local_port is at current chiplet
        if (node_id == current->node_id_in_cg_) {
          VCInfo vc(local_port.link_buffer, 0);
          s.candidate_channels_.push_back(vc);
        } else {  // local_port is at an other chiplet
          XY_routing(s, local_port.node_id);
        }
      }
    }
  }
}

void DragonflyChiplet::XY_routing(Packet& s, NodeID dest) const {
  NodeInCG* current_node = get_node(s.head_trace().id);
  NodeInCG* destination_node = get_node(dest);

  int curx = current_node->x_;
  int cury = current_node->y_;
  int dstx = destination_node->x_;
  int dsty = destination_node->y_;
  int xdis = dstx - curx;  // x offset
  int ydis = dsty - cury;  // y offset

  // Adaptive Routing Channels
  if (xdis < 0)
    s.candidate_channels_.push_back(VCInfo(current_node->xneg_link_buffer_, 1));
  else if (xdis > 0)
    s.candidate_channels_.push_back(VCInfo(current_node->xpos_link_buffer_, 1));
  if (ydis < 0)
    s.candidate_channels_.push_back(VCInfo(current_node->yneg_link_buffer_, 1));
  else if (ydis > 0)
    s.candidate_channels_.push_back(VCInfo(current_node->ypos_link_buffer_, 1));

  if (xdis < 0)  // first x
    s.candidate_channels_.push_back(VCInfo(current_node->xneg_link_buffer_, 0));
  else if (xdis > 0)
    s.candidate_channels_.push_back(VCInfo(current_node->xpos_link_buffer_, 0));
  else if (xdis == 0) {
    if (ydis < 0)  // then y
      s.candidate_channels_.push_back(VCInfo(current_node->yneg_link_buffer_, 0));
    else if (ydis > 0)
      s.candidate_channels_.push_back(VCInfo(current_node->ypos_link_buffer_, 0));
  }
}

// DragonflyChiplet::PortID DragonflyChiplet::local_port_id_to_port_id(int local_port_id)
// {
//   return PortID();
// }

std::pair<int, int> DragonflyChiplet::global_port_id_to_port_id(int global_port_id) {
  int cgroup_id_in_wgroup = global_port_id / g_ports_per_cg_;
  // port id for the lowest global port of the C-group: cgroup_id_in_wgroup
  int node_id = port_node_map_.at(cgroup_id_in_wgroup + global_port_id % g_ports_per_cg_);
  return std::make_pair(cgroup_id_in_wgroup, node_id);
}
