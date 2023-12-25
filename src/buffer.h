#pragma once
#include <atomic>
#include <queue>

#include "config.h"

class Node;
class Packet;

class Buffer {
 public:
  Buffer();
  Buffer(Node* node, int vc_num, int buffer_size, Channel channel);
  ~Buffer();

  bool allocate_buffer(int vcb, int n);  // Return true if there is enough free buffer.
  void release_buffer(int vcb, int n);
  bool allocate_link(Packet&);
  void release_link(Packet&);
  inline Packet* head_packet(int vcb) { return vc_queue_[vcb].front(); }
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
};