#include "buffer.h"

#include "packet.h"

Buffer::Buffer() {
  node_ = nullptr;
  buffer_size_ = 0;
  vc_num_ = 0;
  link_used_.store(false);
  vc_buffer_ = nullptr;
  vc_queue_ = nullptr;
}

Buffer::Buffer(Node* node, int vc_num, int buffer_size, Channel channel) {
  node_ = node;
  buffer_size_ = buffer_size;
  vc_num_ = vc_num;
  channel_ = channel;
  link_used_.store(false);
  vc_buffer_ = new std::atomic_int[vc_num_];
  vc_queue_ = new std::queue<Packet*>[vc_num_];
  for (int i = 0; i < vc_num_; ++i) {
    // vc_buffer_.push_back(buffer_size);
    vc_buffer_[i].store(buffer_size);
    vc_queue_[i] = std::queue<Packet*>();
  }
}

Buffer::~Buffer() {
  delete[] vc_buffer_;
  delete[] vc_queue_;
}

bool Buffer::allocate_buffer(int vcb, int n) {
  int buffer = vc_buffer_[vcb].load();
  while (true) {
    if (buffer < n)
      return false;
    else if (vc_buffer_[vcb].compare_exchange_weak(buffer, buffer - n))
      return true;
  }
}

void Buffer::release_buffer(int vcb, int n) {
  int buffer = vc_buffer_[vcb].load();
  while (!vc_buffer_[vcb].compare_exchange_weak(buffer, buffer + n)) assert(buffer < buffer_size_);
}

bool Buffer::allocate_link(Packet& p) {
  VCInfo current_vc = p.head_trace();
  bool link_used = link_used_.load();
  if (link_used)
    return false;
  else if (link_used_.compare_exchange_strong(link_used, true)) {  // link_used == false
    if (node_->id_ != p.destination_) {
      vc_queue_[p.next_vc_.vc].push(&p);
    }
    return true;
  } else  // allocation failed, link is allocated by other threads(packets)
    return false;
}

void Buffer::release_link(Packet& p) {
  bool link_used = true;
  if (link_used_.compare_exchange_strong(link_used, false)) {
    VCInfo releasing_vc = p.leaving_vc_;
    if (releasing_vc.buffer != nullptr) {
      assert(releasing_vc.head_packet() == &p);
      releasing_vc.buffer->vc_queue_[releasing_vc.vc].pop();
    }
  };
  return;
}

void Buffer::clear_buffer() {
  link_used_.store(false);
  for (int i = 0; i < vc_num_; ++i) {
    vc_buffer_[i].store(buffer_size_);
    while (!vc_queue_[i].empty()) vc_queue_[i].pop();
  }
}