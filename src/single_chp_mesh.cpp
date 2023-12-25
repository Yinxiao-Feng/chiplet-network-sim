#include "single_chip_mesh.h"

SingleChipMesh::SingleChipMesh() {
  // topology parameters
  num_chips_ = 1;
  k_node_ = param->scale;
  num_nodes_ = k_node_ * k_node_;
  num_cores_ = num_nodes_;
  algorithm_ = Algorithm::XY;
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
  switch (algorithm_) {
    case SingleChipMesh::Algorithm::XY:
      XY_routing(s);
      break;
    default:
      XY_routing(s);
  }
}

void SingleChipMesh::XY_routing(Packet &s) {
  NodeMesh *current_node = get_node(s.head_trace().id);
  NodeMesh *destination_node = get_node(s.destination_);

  int curx = current_node->x_;
  int cury = current_node->y_;
  int dstx = destination_node->x_;
  int dsty = destination_node->y_;
  int xdis = dstx - curx;  // x offset
  int ydis = dsty - cury;  // y offset

  // Adaptive Routing Channels
  // if (xdis < 0)
  //   s.candidate_channels_.push_back(VCInfo(current_node->xneg_link_buffer_, 1));
  // else if (xdis > 0)
  //   s.candidate_channels_.push_back(VCInfo(current_node->xpos_link_buffer_, 1));
  // if (ydis < 0)
  //   s.candidate_channels_.push_back(VCInfo(current_node->yneg_link_buffer_, 1));
  // else if (ydis > 0)
  //   s.candidate_channels_.push_back(VCInfo(current_node->ypos_link_buffer_, 1));

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
  return;
}