#include "multiple_chip_mesh.h"

MultiChipMesh::MultiChipMesh() {
  // topology parameters
  read_config();
  num_chips_ = k_chip_ * k_chip_;
  num_nodes_ = k_node_ * k_node_ * num_chips_;
  num_cores_ = num_nodes_;
  for (int chip_id = 0; chip_id < num_chips_; chip_id++) {
    chips_.push_back(new ChipMesh(k_node_, param->vc_number, param->buffer_size));
    get_chip(chip_id)->set_chip(this, chip_id);
    get_chip(chip_id)->chip_coordinate_[0] = chip_id % k_chip_;
    get_chip(chip_id)->chip_coordinate_[1] = chip_id / k_chip_;
  }
  connect_chiplets();
}

MultiChipMesh::~MultiChipMesh() {
  for (auto chiplet : chips_) delete chiplet;
  chips_.clear();
}

void MultiChipMesh::read_config() {
  k_node_ = param->params_ptree.get<int>("Network.k_node", 4);
  k_chip_ = param->params_ptree.get<int>("Network.k_chip", 2);
  algorithm_ = param->params_ptree.get<std::string>("Network.routing_algorithm", "XY");
  if (algorithm_ == "NFR_adaptive") assert(param->vc_number >= 2);
  d2d_IF_ = param->params_ptree.get<std::string>("Network.d2d_IF", "off_chip_parallel");
  printf("Multi Chip 2D-mesh, %ix%i chiplets, each chiplet %ix%i nodes\n", k_chip_, k_chip_,
         k_node_, k_node_);
}

void MultiChipMesh::connect_chiplets() {
  for (int chip_id = 0; chip_id < num_chips_; ++chip_id) {
    ChipMesh* chip = get_chip(chip_id);
    int chip_x = chip->chip_coordinate_[0];
    int chip_y = chip->chip_coordinate_[1];
    if (chip_x != 0) {
      for (int y = 0; y < k_node_; ++y) {
        NodeMesh* node = chip->get_node(y * k_node_);
        node->xneg_link_node_ = NodeID(y * k_node_ + k_node_ - 1, chip_id - 1);
        node->xneg_link_buffer_ = get_node(node->xneg_link_node_)->xpos_in_buffer_;
        if (d2d_IF_ == "off_chip_parallel")
          node->xneg_in_buffer_->channel_ = off_chip_parallel_channel;
        else if (d2d_IF_ == "off_chip_serial")
          node->xneg_in_buffer_->channel_ = off_chip_serial_channel;
        else
          std::cerr << "Unknown d2d interface: " << d2d_IF_ << std::endl;
      }
    }
    if (chip_x != k_chip_ - 1) {
      for (int y = 0; y < k_node_; ++y) {
        NodeMesh* node = chip->get_node(y * k_node_ + k_node_ - 1);
        node->xpos_link_node_ = NodeID(y * k_node_, chip_id + 1);
        node->xpos_link_buffer_ = get_node(node->xpos_link_node_)->xneg_in_buffer_;
        if (d2d_IF_ == "off_chip_parallel")
          node->xpos_in_buffer_->channel_ = off_chip_parallel_channel;
        else if (d2d_IF_ == "off_chip_serial")
          node->xpos_in_buffer_->channel_ = off_chip_serial_channel;
        else
          std::cerr << "Unknown d2d interface: " << d2d_IF_ << std::endl;
      }
    }
    if (chip_y != 0) {
      for (int x = 0; x < k_node_; ++x) {
        NodeMesh* node = chip->get_node(x);
        node->yneg_link_node_ = NodeID(x + (k_node_ - 1) * k_node_, chip_id - k_chip_);
        node->yneg_link_buffer_ = get_node(node->yneg_link_node_)->ypos_in_buffer_;
        if (d2d_IF_ == "off_chip_parallel")
          node->yneg_in_buffer_->channel_ = off_chip_parallel_channel;
        else if (d2d_IF_ == "off_chip_serial")
          node->yneg_in_buffer_->channel_ = off_chip_serial_channel;
        else
          std::cerr << "Unknown d2d interface: " << d2d_IF_ << std::endl;
      }
    }
    if (chip_y != k_chip_ - 1) {
      for (int x = 0; x < k_node_; ++x) {
        NodeMesh* node = chip->get_node(x + (k_node_ - 1) * k_node_);
        node->ypos_link_node_ = NodeID(x, chip_id + k_chip_);
        node->ypos_link_buffer_ = get_node(node->ypos_link_node_)->yneg_in_buffer_;
        if (d2d_IF_ == "off_chip_parallel")
          node->ypos_in_buffer_->channel_ = off_chip_parallel_channel;
        else if (d2d_IF_ == "off_chip_serial")
          node->ypos_in_buffer_->channel_ = off_chip_serial_channel;
        else
          std::cerr << "Unknown d2d interface: " << d2d_IF_ << std::endl;
      }
    }
  }
}

void MultiChipMesh::routing_algorithm(Packet& s) const {
  if (algorithm_ == "XY")
    XY_routing(s);
  else if (algorithm_ == "NFR")
    NFR_routing(s);
  else if (algorithm_ == "NFR_adaptive")
    NFR_adaptive_routing(s);
  else
    std::cerr << "Unknown routing algorithm: " << algorithm_ << std::endl;
}

void MultiChipMesh::XY_routing(Packet& s) const {
  ChipMesh* current_chip = get_chip(s.head_trace().id);
  ChipMesh* destination_chip = get_chip(s.destination_);
  NodeMesh* current_node = get_node(s.head_trace().id);
  NodeMesh* destination_node = get_node(s.destination_);

  int cur_x = current_chip->chip_coordinate_[0] * k_node_ + current_node->x_;
  int cur_y = current_chip->chip_coordinate_[1] * k_node_ + current_node->y_;
  int dest_x = destination_chip->chip_coordinate_[0] * k_node_ + destination_node->x_;
  int dest_y = destination_chip->chip_coordinate_[1] * k_node_ + destination_node->y_;
  int dis_x = dest_x - cur_x;  // x offset
  int dis_y = dest_y - cur_y;  // y offset

  if (dis_x < 0)  // first x
    for (int i = 0; i < current_node->xneg_link_buffer_->vc_num_; i++)
      s.candidate_channels_.push_back(VCInfo(current_node->xneg_link_buffer_, i));
  else if (dis_x > 0)
    for (int i = 0; i < current_node->xpos_link_buffer_->vc_num_; i++)
      s.candidate_channels_.push_back(VCInfo(current_node->xpos_link_buffer_, i));
  else if (dis_x == 0) {
    if (dis_y < 0)  // then y
      for (int i = 0; i < current_node->yneg_link_buffer_->vc_num_; i++)
        s.candidate_channels_.push_back(VCInfo(current_node->yneg_link_buffer_, i));
    else if (dis_y > 0)
      for (int i = 0; i < current_node->ypos_link_buffer_->vc_num_; i++)
        s.candidate_channels_.push_back(VCInfo(current_node->ypos_link_buffer_, i));
  }
}

void MultiChipMesh::NFR_routing(Packet& s) const {
  ChipMesh* current_chip = get_chip(s.head_trace().id);
  ChipMesh* destination_chip = get_chip(s.destination_);
  NodeMesh* current_node = get_node(s.head_trace().id);
  NodeMesh* destination_node = get_node(s.destination_);

  int cur_x = current_chip->chip_coordinate_[0] * k_node_ + current_node->x_;
  int cur_y = current_chip->chip_coordinate_[1] * k_node_ + current_node->y_;
  int dest_x = destination_chip->chip_coordinate_[0] * k_node_ + destination_node->x_;
  int dest_y = destination_chip->chip_coordinate_[1] * k_node_ + destination_node->y_;
  int dis_x = dest_x - cur_x;  // x offset
  int dis_y = dest_y - cur_y;  // y offset

  // Baseline routing: negative-first
  if (dis_x < 0 || dis_y < 0) {
    if (dis_x < 0)
      for (int i = 0; i < current_node->xneg_link_buffer_->vc_num_; i++)
        s.candidate_channels_.push_back(VCInfo(current_node->xneg_link_buffer_, i));
    if (dis_y < 0)
      for (int i = 0; i < current_node->yneg_link_buffer_->vc_num_; i++)
        s.candidate_channels_.push_back(VCInfo(current_node->yneg_link_buffer_, i));
  } else {
    if (dis_x > 0)
      for (int i = 0; i < current_node->xpos_link_buffer_->vc_num_; i++)
        s.candidate_channels_.push_back(VCInfo(current_node->xpos_link_buffer_, i));
    if (dis_y > 0)
      for (int i = 0; i < current_node->ypos_link_buffer_->vc_num_; i++)
        s.candidate_channels_.push_back(VCInfo(current_node->ypos_link_buffer_, i));
  }
}

void MultiChipMesh::NFR_adaptive_routing(Packet& s) const {
  ChipMesh* current_chip = get_chip(s.head_trace().id);
  ChipMesh* destination_chip = get_chip(s.destination_);
  NodeMesh* current_node = get_node(s.head_trace().id);
  NodeMesh* destination_node = get_node(s.destination_);

  int cur_x = current_chip->chip_coordinate_[0] * k_node_ + current_node->x_;
  int cur_y = current_chip->chip_coordinate_[1] * k_node_ + current_node->y_;
  int dest_x = destination_chip->chip_coordinate_[0] * k_node_ + destination_node->x_;
  int dest_y = destination_chip->chip_coordinate_[1] * k_node_ + destination_node->y_;
  int dis_x = dest_x - cur_x;  // x offset
  int dis_y = dest_y - cur_y;  // y offset

  // Adaptive Routing Channels
  if (dis_x < 0)
    for (int i = 0; i < current_node->xneg_link_buffer_->vc_num_ - 1; i++)
      s.candidate_channels_.push_back(VCInfo(current_node->xneg_link_buffer_, i));
  else if (dis_x > 0)
    for (int i = 0; i < current_node->xpos_link_buffer_->vc_num_ - 1; i++)
      s.candidate_channels_.push_back(VCInfo(current_node->xpos_link_buffer_, i));
  if (dis_y < 0)
    for (int i = 0; i < current_node->yneg_link_buffer_->vc_num_ - 1; i++)
      s.candidate_channels_.push_back(VCInfo(current_node->yneg_link_buffer_, i));
  else if (dis_y > 0)
    for (int i = 0; i < current_node->ypos_link_buffer_->vc_num_ - 1; i++)
      s.candidate_channels_.push_back(VCInfo(current_node->ypos_link_buffer_, i));

  // Baseline routing: negative-first
  if (dis_x < 0 || dis_y < 0) {
    if (dis_x < 0)
      s.candidate_channels_.push_back(
          VCInfo(current_node->xneg_link_buffer_, current_node->xneg_link_buffer_->vc_num_ - 1));
    if (dis_y < 0)
      s.candidate_channels_.push_back(
          VCInfo(current_node->yneg_link_buffer_, current_node->yneg_link_buffer_->vc_num_ - 1));
  } else {
    if (dis_x > 0)
      s.candidate_channels_.push_back(
          VCInfo(current_node->xpos_link_buffer_, current_node->xpos_link_buffer_->vc_num_ - 1));
    if (dis_y > 0)
      s.candidate_channels_.push_back(
          VCInfo(current_node->ypos_link_buffer_, current_node->ypos_link_buffer_->vc_num_ - 1));
  }
}
