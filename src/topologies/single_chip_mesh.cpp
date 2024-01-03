#include "single_chip_mesh.h"

SingleChipMesh::SingleChipMesh() {
  // topology parameters
  num_chips_ = 1;
  k_node_ = param->scale;
  num_nodes_ = k_node_ * k_node_;
  num_cores_ = num_nodes_;
  algorithm_ = param->routing_algorithm;
  chips_.push_back(new ChipMesh(k_node_, param->vc_number, param->buffer_size));
  for (int chip_id = 0; chip_id < num_chips_; chip_id++) {
    chips_[chip_id]->set_chip(this, chip_id);
  }
}

SingleChipMesh::~SingleChipMesh() {
  delete chips_[0];
  chips_.clear();
}

void SingleChipMesh::routing_algorithm(Packet &s) {
  if (algorithm_ == "XY")
    XY_routing(s);
  else if (algorithm_ == "NFR")
    NFR_routing(s);
  else
    std::cerr << "Unknown routing algorithm: " << algorithm_ << std::endl;
}

void SingleChipMesh::XY_routing(Packet &s) const {
  NodeMesh *current_node = get_node(s.head_trace().id);
  NodeMesh *destination_node = get_node(s.destination_);

  int curx = current_node->x_;
  int cury = current_node->y_;
  int dstx = destination_node->x_;
  int dsty = destination_node->y_;
  int xdis = dstx - curx;  // x offset
  int ydis = dsty - cury;  // y offset

  if (xdis < 0)  // first x
    for (int i = 0; i < current_node->xneg_link_buffer_->vc_num_; i++)
      s.candidate_channels_.push_back(VCInfo(current_node->xneg_link_buffer_, i));
  else if (xdis > 0)
    for (int i = 0; i < current_node->xpos_link_buffer_->vc_num_; i++)
      s.candidate_channels_.push_back(VCInfo(current_node->xpos_link_buffer_, i));
  else if (xdis == 0) {
    if (ydis < 0)  // then y
      for (int i = 0; i < current_node->yneg_link_buffer_->vc_num_; i++)
        s.candidate_channels_.push_back(VCInfo(current_node->yneg_link_buffer_, i));
    else if (ydis > 0)
      for (int i = 0; i < current_node->ypos_link_buffer_->vc_num_; i++)
        s.candidate_channels_.push_back(VCInfo(current_node->ypos_link_buffer_, i));
  }
}

void SingleChipMesh::NFR_routing(Packet &s) const {
  NodeMesh *current_node = get_node(s.head_trace().id);
  NodeMesh *destination_node = get_node(s.destination_);

  int curx = current_node->x_;
  int cury = current_node->y_;
  int dstx = destination_node->x_;
  int dsty = destination_node->y_;
  int xdis = dstx - curx;  // x offset
  int ydis = dsty - cury;  // y offset

  // Baseline routing: negative-first
  if (xdis < 0 || ydis < 0) {
    if (xdis < 0)
      for (int i = 0; i < current_node->xneg_link_buffer_->vc_num_; i++)
        s.candidate_channels_.push_back(VCInfo(current_node->xneg_link_buffer_, i));
    if (ydis < 0)
      for (int i = 0; i < current_node->yneg_link_buffer_->vc_num_; i++)
        s.candidate_channels_.push_back(VCInfo(current_node->yneg_link_buffer_, i));
  } else {
    if (xdis > 0)
      for (int i = 0; i < current_node->xpos_link_buffer_->vc_num_; i++)
        s.candidate_channels_.push_back(VCInfo(current_node->xpos_link_buffer_, i));
    if (ydis > 0)
      for (int i = 0; i < current_node->ypos_link_buffer_->vc_num_; i++)
        s.candidate_channels_.push_back(VCInfo(current_node->ypos_link_buffer_, i));
  }
}