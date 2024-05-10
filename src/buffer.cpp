#include "buffer.h"

#include "packet.h"

Buffer::Buffer() {
  node_ = nullptr;
  buffer_size_ = 0;
  vc_num_ = 0;
  link_used_.store(false);
  vc_buffer_ = nullptr;
  vc_queue_ = nullptr;
  vc_head_packet = nullptr;
}

Buffer::Buffer(Node* node, int vc_num, int buffer_size, Channel channel) {
  node_ = node;
  buffer_size_ = buffer_size;
  vc_num_ = vc_num;
  channel_ = channel;
  channel_.latency = param->on_chip_latency;
  link_used_.store(false);
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
    if (node_->id_ != p.destination_) {  // only one thread win the link allocation
      if (vc_head_packet[p.next_vc_.vc] != nullptr)
        vc_queue_[p.next_vc_.vc].push(&p);
      else
        vc_head_packet[p.next_vc_.vc].store(&p);
    }
    return true;
  } else  // allocation failed, link is allocated by other threads(packets)
    return false;
}

void Buffer::release_link(Packet& p) {
  assert(link_used_);
  link_used_.store(false);
  // release head in former buffer
  Buffer* buffer = p.leaving_vc_.buffer;
  int vc = p.leaving_vc_.vc;
  if (buffer != nullptr) {
    assert(p.leaving_vc_.head_packet() == &p);
    if (!buffer->vc_queue_[vc].empty()) {
      buffer->vc_head_packet[vc].store(buffer->vc_queue_[vc].front());
      buffer->vc_queue_[vc].pop();
    } else
      buffer->vc_head_packet[vc].store(nullptr);
    // releasing_vc.buffer->vc_queue_[releasing_vc.vc].pop();
  }
  // p.leaving_vc_.release_head();
}

void Buffer::clear_buffer() {
  link_used_.store(false);
  for (int i = 0; i < vc_num_; ++i) {
    vc_buffer_[i].store(buffer_size_);
    while (!vc_queue_[i].empty()) vc_queue_[i].pop();
    vc_head_packet[i].store(nullptr);
  }
}

VCInfo::VCInfo(Buffer* buffer_, int vc_, NodeID id_) {
  buffer = buffer_;
  vc = vc_;
  if (buffer_ != nullptr)
    id = buffer->node_->id_;
  else
    id = id_;
}

Packet* VCInfo::head_packet() const { return buffer->head_packet(vc); }

// void VCInfo::release_head() const {
//   {
//     if (buffer != nullptr) {
//       if (!buffer->vc_queue_[vc].empty()) {
//         buffer->vc_head_packet[vc].store(buffer->vc_queue_[vc].front());
//         buffer->vc_queue_[vc].pop();
//       } else
//         buffer->vc_head_packet[vc].store(nullptr);
//     }
//   };
// }
