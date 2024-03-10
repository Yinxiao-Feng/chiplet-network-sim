#pragma once
#include "buffer.h"

class Packet {
 public:
  Packet(NodeID src, NodeID dst, int length);
  friend std::ostream& operator<<(std::ostream& s, Packet*& m) {
    s << "Source:" << m->source_ << " Destination:" << m->destination_ << std::endl;
    return s;
  }
  inline const VCInfo& head_trace() const { return flit_trace_[0]; };
  inline const VCInfo& tail_trace() const { return flit_trace_[length_ - 1]; };

  NodeID source_;
  NodeID destination_;
  // the ith flit now at [i].node and take vc-[i].vcb of [i].buffer
  std::vector<VCInfo> flit_trace_;
  std::vector<VCInfo> candidate_channels_;
  VCInfo next_vc_;  // vc for the next hop
  int interleaving_tag_;
  bool switch_allocated_;

  int length_;
  int process_timer_;  // time to process a message before injecting
  int SA_timer_;       // time cost by switch allocating
  int link_timer_;     // time cost on the link
  int wait_timer_;     // waiting time in one buffer
  int trans_timer_;    // the total time a message consumed
  int internal_hops_;
  int parallel_hops_;
  int serial_hops_;
  int other_hops_;
  bool finished_;      // check message whether arrived
  bool releaselink_;   // if the tail of a message shifts , the physical link
                       // the message occupied should release.
  VCInfo leaving_vc_;  // if the tail of a message shifts , the buffer the
                       // message occupied should release.
};
