#include "multiple_chip_torus.h"

MultiChipTorus::MultiChipTorus() {
  read_config();
  num_chips_ = k_chip_ * k_chip_;
  num_nodes_ = k_node_ * k_node_ * num_chips_;
  num_cores_ = num_nodes_;
  for (int chip_id = 0; chip_id < num_chips_; chip_id++) {
    chips_.push_back(new ChipMesh(k_node_, param->vc_number, param->buffer_size));
    chips_[chip_id]->set_chip(this, chip_id);
    get_chip(chip_id)->chip_coordinate_[0] = chip_id % k_chip_;
    get_chip(chip_id)->chip_coordinate_[1] = chip_id / k_chip_;
  }
  connect_chiplets();
}

MultiChipTorus::~MultiChipTorus() {
  for (auto chiplet : chips_) delete chiplet;
  chips_.clear();
}

void MultiChipTorus::read_config() {
  k_node_ = param->params_ptree.get<int>("Network.k_node", 4);
  k_chip_ = param->params_ptree.get<int>("Network.k_chip", 2);
  algorithm_ = param->params_ptree.get<std::string>("Network.routing_algorithm", "CLUE");
}

void MultiChipTorus::connect_chiplets() {
  for (int chip_id = 0; chip_id < num_chips_; ++chip_id) {
    ChipMesh* chip = get_chip(chip_id);
    int chip_x = chip->chip_coordinate_[0];
    int chip_y = chip->chip_coordinate_[1];
    NodeMesh* node;
    int link_node_id, link_chip_id;
    for (int y = 0; y < k_node_; ++y) {
      node = chip->get_node(y * k_node_);
      link_node_id = y * k_node_ + k_node_ - 1;
      link_chip_id = chip_y * k_chip_ + (chip_id - 1 + k_chip_) % k_chip_;
      node->xneg_link_node_ = NodeID(link_node_id, link_chip_id);
      node->xneg_link_buffer_ = get_node(node->xneg_link_node_)->xpos_in_buffer_;
      if (chip_x == 0)
        node->xneg_in_buffer_->channel_ = off_chip_serial_channel;
      else
        node->xneg_in_buffer_->channel_ = off_chip_parallel_channel;
    }
    for (int y = 0; y < k_node_; ++y) {
      node = chip->get_node(y * k_node_ + k_node_ - 1);
      link_node_id = y * k_node_;
      link_chip_id = chip_y * k_chip_ + (chip_id + 1) % k_chip_;
      node->xpos_link_node_ = NodeID(link_node_id, link_chip_id);
      node->xpos_link_buffer_ = get_node(node->xpos_link_node_)->xneg_in_buffer_;
      if (chip_x == k_chip_ - 1)
        node->xpos_in_buffer_->channel_ = off_chip_serial_channel;
      else
        node->xpos_in_buffer_->channel_ = off_chip_parallel_channel;
    }
    for (int x = 0; x < k_node_; ++x) {
      node = chip->get_node(x);
      link_node_id = x + (k_node_ - 1) * k_node_;
      link_chip_id = (chip_y - 1 + k_chip_) % k_chip_ * k_chip_ + chip_x;
      node->yneg_link_node_ = NodeID(link_node_id, link_chip_id);
      node->yneg_link_buffer_ = get_node(node->yneg_link_node_)->ypos_in_buffer_;
      if (chip_y == 0)
        node->yneg_in_buffer_->channel_ = off_chip_serial_channel;
      else
        node->yneg_in_buffer_->channel_ = off_chip_parallel_channel;
    }
    for (int x = 0; x < k_node_; ++x) {
      node = chip->get_node(x + (k_node_ - 1) * k_node_);
      link_node_id = x;
      link_chip_id = (chip_y + 1) % k_chip_ * k_chip_ + chip_x;
      node->ypos_link_node_ = NodeID(link_node_id, link_chip_id);
      node->ypos_link_buffer_ = get_node(node->ypos_link_node_)->yneg_in_buffer_;
      if (chip_y == k_chip_ - 1)
        node->ypos_in_buffer_->channel_ = off_chip_serial_channel;
      else
        node->ypos_in_buffer_->channel_ = off_chip_parallel_channel;
    }
  }
}

void MultiChipTorus::routing_algorithm(Packet& s) const {
  if (algorithm_ == "CLUE")
    clue_routing(s);
  else
    std::cerr << "Unknown routing algorithm: " << algorithm_ << std::endl;
}

void MultiChipTorus::clue_routing(Packet& s) const {
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

  // VC-0 Adaptive Routing
  int K = k_node_ * k_chip_;
  if (-K / 2 <= dis_x && dis_x < 0 || dis_x > K / 2)
    s.candidate_channels_.push_back(VCInfo(current_node->xneg_link_buffer_, 0));
  else if (0 < dis_x && dis_x <= K / 2 || dis_x < -K / 2)
    s.candidate_channels_.push_back(VCInfo(current_node->xpos_link_buffer_, 0));

  if (-K / 2 <= dis_y && dis_y < 0 || dis_y > K / 2)
    s.candidate_channels_.push_back(VCInfo(current_node->yneg_link_buffer_, 0));
  else if (0 < dis_y && dis_y <= K / 2 || dis_y < -K / 2)
    s.candidate_channels_.push_back(VCInfo(current_node->ypos_link_buffer_, 0));

  if (abs(dis_x) <= K / 2 && abs(dis_y) <= K / 2) {
    // negative-first
    if (dis_x < 0 || dis_y < 0) {
      if (dis_x < 0) s.candidate_channels_.push_back(VCInfo(current_node->xneg_link_buffer_, 1));
      if (dis_y < 0) s.candidate_channels_.push_back(VCInfo(current_node->yneg_link_buffer_, 1));
    } else {
      if (dis_x > 0) s.candidate_channels_.push_back(VCInfo(current_node->xpos_link_buffer_, 1));
      if (dis_y > 0) s.candidate_channels_.push_back(VCInfo(current_node->ypos_link_buffer_, 1));
    }
  } else if (abs(dis_x) > K / 2) {
    if (cur_x == 0)
      s.candidate_channels_.push_back(VCInfo(current_node->xneg_link_buffer_, 1));
    else if (cur_x == K - 1)
      s.candidate_channels_.push_back(VCInfo(current_node->xpos_link_buffer_, 1));
  } else if (abs(dis_y) > K / 2) {
    if (cur_y == 0)
      s.candidate_channels_.push_back(VCInfo(current_node->yneg_link_buffer_, 1));
    else if (cur_y == K - 1)
      s.candidate_channels_.push_back(VCInfo(current_node->ypos_link_buffer_, 1));
  }
}
