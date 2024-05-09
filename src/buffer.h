#pragma once
#include <atomic>
#include <queue>

#include "node.h"

class Packet;
class Buffer;

struct VCInfo {
  VCInfo(Buffer* buffer_ = nullptr, int vc_ = 0, NodeID id_ = NodeID());
  NodeID id;
  Buffer* buffer;  // point to the buffer occupying, such as bufferxneg......
  int vc;          // which VC channel

  inline bool operator==(const VCInfo& vcb) const { return (id == vcb.id && vc == vcb.vc); }
  Packet* head_packet() const;
};

class Buffer {
 public:
  Buffer();
  Buffer(Node* node, int vc_num, int buffer_size, Channel channel);
  ~Buffer();

  bool allocate_buffer(int vcb, int n);  // Return true if there is enough free buffer.
  void release_buffer(int vcb, int n);
  bool allocate_link(Packet&);
  void release_link(Packet&);
  inline Packet* head_packet(int vcb) { return vc_head_packet[vcb].load(); }
  inline bool is_empty(int vcb) { return vc_head_packet[vcb].load() == nullptr; }
  void clear_buffer();

  // Point to the node where the buffer is located.
  Node* node_;

  int buffer_size_;  // for each VC, measured by flits number
  int vc_num_;       // virtual channel number

  Channel channel_;

 private:
  // single thread: first come first serve
  // multi thread: atomic operation ensures correctness
  // current state of the physical link
  std::atomic_bool link_used_;
  // current free buffer for each VC, measured by flit number
  std::atomic_int* vc_buffer_;
  // record the packet order for each VC, only the head packet can be sent
  std::queue<Packet*>* vc_queue_;
  std::atomic<Packet*>* vc_head_packet;
};