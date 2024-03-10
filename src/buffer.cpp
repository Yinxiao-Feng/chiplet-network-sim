#include "buffer.h"

#include "packet.h"

VCInfo::VCInfo(Buffer* buffer_, int vc_, NodeID id_) {
  buffer = buffer_;
  vcb = vc_;
  if (buffer_ != nullptr)
    id = buffer->node_->id_;
  else
    id = id_;
}

Packet* VCInfo::head_packet() const { return buffer->head_packet(vcb); }

Buffer::Buffer() {
  node_ = nullptr;
  buffer_size_ = 0;
  vc_num_ = 0;
  in_link_used_.store(false);
  sw_link_used_.store(false);
  vc_buffer_ = nullptr;
  vc_queue_ = nullptr;
  vc_head_packet = nullptr;
}

Buffer::Buffer(Node* node, int vc_num, int buffer_size, Channel channel) {
  node_ = node;
  buffer_size_ = buffer_size;
  vc_num_ = vc_num;
  channel_ = channel;
  in_link_used_.store(false);
  sw_link_used_.store(false);
  vc_buffer_ = new std::atomic_int[vc_num_];
  vc_queue_ = new std::queue<Packet*>[vc_num_];
  vc_head_packet = new std::atomic<Packet*>[vc_num_];
  for (int i = 0; i < vc_num_; ++i) {
    // vc_buffer_.push_back(buffer_size);
    vc_buffer_[i].store(buffer_size);
    vc_queue_[i] = std::queue<Packet*>();
    vc_head_packet[i].store(nullptr);
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
    // buffer is allocate by other threads (packets), try again
  }
}

void Buffer::release_buffer(int vcb, int n) {
  int buffer = vc_buffer_[vcb].load();
  while (!vc_buffer_[vcb].compare_exchange_weak(buffer, buffer + n))
    ;
  assert(vc_buffer_[vcb].load() <= buffer_size_);
}

bool Buffer::allocate_in_link(Packet& p) {
  int vcb = p.next_vc_.vcb;
  bool link_used_state = in_link_used_.load();
  if (link_used_state)
    return false;
  else if (in_link_used_.compare_exchange_strong(link_used_state, true)) {
    // link is allocated by this thread (packet)
    if (node_->id_ != p.destination_) {
      push_pkt(&p, vcb);
    }
    return true;
  } else  // allocation failed, link is allocated by other threads(packets)
    return false;
}

void Buffer::release_in_link(Packet& p) {
  assert(in_link_used_);
  // release head in former buffer
  if (p.leaving_vc_.buffer != nullptr) {  // not the source node
    assert(p.leaving_vc_.head_packet() == &p);
    p.leaving_vc_.buffer->pop_pkt(p.leaving_vc_.vcb);
  }
  // release link
  in_link_used_.store(false);
}

bool Buffer::allocate_sw_link() {
  bool link_used_state = sw_link_used_.load();
  if (link_used_state)
	return false;
  else if (sw_link_used_.compare_exchange_strong(link_used_state, true)) {
	// link is allocated by this thread (packet)
	return true;
  } else  // allocation failed, link is allocated by other threads(packets)
	return false;
}

void Buffer::release_sw_link() {
  assert(sw_link_used_);
  sw_link_used_.store(false);
}

void Buffer::clear_buffer() {
  in_link_used_.store(false);
  sw_link_used_.store(false);
  for (int i = 0; i < vc_num_; ++i) {
    vc_buffer_[i].store(buffer_size_);
    while (!vc_queue_[i].empty()) vc_queue_[i].pop();
    vc_head_packet[i].store(nullptr);
  }
}
