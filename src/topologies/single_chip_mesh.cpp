#include "single_chip_mesh.h"

SingleChipMesh::SingleChipMesh() {
  read_config();
  num_chips_ = 1;
  num_nodes_ = k_node_ * k_node_;
  num_cores_ = num_nodes_;
  chips_.push_back(new ChipMesh(k_node_, param->vc_number, param->buffer_size));
  chips_[0]->set_chip(this, 0);
}

SingleChipMesh::~SingleChipMesh() {
  delete chips_[0];
  chips_.clear();
}

void SingleChipMesh::read_config() {
  k_node_ = param->params_ptree.get<int>("Network.scale", 4);
  algorithm_ = param->params_ptree.get<std::string>("Network.routing_algorithm", "XY");
  if (algorithm_ == "NFR_adaptive") assert(param->vc_number >= 2);
  printf("Single Chip 2D-mesh, %ix%i\n", k_node_, k_node_);
}

void SingleChipMesh::routing_algorithm(Packet &s) const {
  if (algorithm_ == "XY")
    XY_routing(s);
  else if (algorithm_ == "NFR")
    NFR_routing(s);
  else if (algorithm_ == "NFR_adaptive")
    NFR_adaptive_routing(s);
  else
    std::cerr << "Unknown routing algorithm: " << algorithm_ << std::endl;
}

void SingleChipMesh::XY_routing(Packet &s) const {
  NodeMesh *current_node = get_node(s.head_trace().id);
  NodeMesh *destination_node = get_node(s.destination_);

  int cur_x = current_node->x_;
  int cur_y = current_node->y_;
  int dest_x = destination_node->x_;
  int dest_y = destination_node->y_;
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

void SingleChipMesh::NFR_routing(Packet &s) const {
  NodeMesh *current_node = get_node(s.head_trace().id);
  NodeMesh *destination_node = get_node(s.destination_);

  int cur_x = current_node->x_;
  int cur_y = current_node->y_;
  int dest_x = destination_node->x_;
  int dest_y = destination_node->y_;
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

void SingleChipMesh::NFR_adaptive_routing(Packet &s) const {
  NodeMesh *current_node = get_node(s.head_trace().id);
  NodeMesh *destination_node = get_node(s.destination_);

  int cur_x = current_node->x_;
  int cur_y = current_node->y_;
  int dest_x = destination_node->x_;
  int dest_y = destination_node->y_;
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