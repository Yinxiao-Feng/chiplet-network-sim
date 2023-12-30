#include "system.h"

#include "dragonfly_chiplet.h"
#include "dragonfly_sw.h"
#include "single_chip_mesh.h"
#include "traffic_manager.h"

System::System() {
  num_chips_ = 0;
  num_nodes_ = 0;
  num_cores_ = 0;

  // router parameters
  routing_time_ = param->routing_time;
  vc_allocating_time_ = param->vc_allocating_time;
  sw_allocating_time_ = param->sw_allocating_time;
  microarchitecture_ = param->microarchitecture;

  // simulation parameters
  timeout_time_ = param->timeout_threshold;
}

System* System::New(std::string topology) {
  System* sys_ptr;
  if (topology == "SingleChipMesh")
    sys_ptr = new SingleChipMesh;
  else if (topology == "DragonflySW")
    sys_ptr = new DragonflySW;
  else if (topology == "DragonflyChiplet")
    sys_ptr = new DragonflyChiplet;
  else {
    std::cerr << "No such a topology!" << std::endl;
    return nullptr;
  }
  return sys_ptr;
}

void System::reset() {
  for (auto chip : chips_) {
    chip->clear_all();
  }
}

void System::onestage(Packet& p) {
  if (p.candidate_channels_.empty()) routing(p);
  if (!p.candidate_channels_.empty() && p.next_vc_.buffer == nullptr)  // VC Allocating Stage
    vc_allocate(p);
  if (p.next_vc_.buffer != nullptr && p.switch_allocated_ == false)  // Switch Allocating Stage
    switch_allocate(p);
}

void System::twostage(Packet& s) {
  if (s.candidate_channels_.empty()) routing(s);
  if (!s.candidate_channels_.empty() && s.next_vc_.buffer == nullptr)  // VC Allocating Stage
    vc_allocate(s);
  else if (s.next_vc_.buffer != nullptr && s.switch_allocated_ == false)  // Switch Allocating Stage
    switch_allocate(s);
}

void System::Threestage(Packet& s) {
  if (s.candidate_channels_.empty())  // Routing Stage
    routing(s);
  else if (!s.candidate_channels_.empty() && s.next_vc_.buffer == nullptr)  // VC Allocating Stage
    vc_allocate(s);
  else if (s.next_vc_.buffer != nullptr && s.switch_allocated_ == false)  // Switch Allocating Stage
    switch_allocate(s);
}

void System::routing(Packet& p) {
  assert(p.candidate_channels_.empty());
  if (p.routing_timer_ > 0) {
    p.routing_timer_--;
    return;
  }
  routing_algorithm(p);
  assert(!p.candidate_channels_.empty());
}

void System::vc_allocate(Packet& p) {
  // Latency for vc_allocate
  if (p.VA_timer_ > 0) {
    p.VA_timer_--;
    return;
  }
  for (auto& vc : p.candidate_channels_) {  // packet switching
    if (vc.id == p.destination_) {
      // immediately consumed upon reaching the destination, no vc is occupied
      p.next_vc_ = vc;
      return;
    } else if (vc.buffer->allocate_buffer(vc.vc, p.length_)) {
      // allocating sucess
      p.next_vc_ = vc;
      return;
    }
  }
  p.VA_timer_ = vc_allocating_time_;
}

void System::switch_allocate(Packet& p) {
  VCInfo current_vc = p.head_trace();
  if (current_vc.buffer == nullptr ||           // packet is at the source
      current_vc.head_packet() == &p) {         // the packet is at the front of the queue
    if (p.next_vc_.buffer->allocate_link(p)) {  // wait for link
      if (p.SA_timer_ <= 0) {                   // allocating link (bandwidth)
        // s.next_vc_.buffer->link_used_ = true;
        p.switch_allocated_ = true;
      } else
        p.SA_timer_--;
    } else {  // no available links
      p.SA_timer_ = sw_allocating_time_;
    }
  }
  // not the head of the queue, wait in the queue
}

void System::update(Packet& p) {
  // A packet cannot be sent to itself
  assert(p.link_timer_ > 0 || p.destination_ != p.tail_trace().id);

  // Processing at source node before transmission (Packetization, injection, etc.)
  if (p.head_trace().id == p.source_ && p.process_timer_ > 0) {
    p.process_timer_--;
    if (p.process_timer_ == 0) {
      // injected
      p.routing_timer_ = routing_time_;
      p.VA_timer_ = vc_allocating_time_;
      p.SA_timer_ = sw_allocating_time_;
    }
    return;
  }

  p.trans_timer_++;
  if (p.wait_timer_ == timeout_time_)  // timeout
    TM->message_timeout_++;

  // Routing -> VC allocating -> Switch allocating -> Transmission
  // switch_allocated_ is the final credit for message forwarding.
  if (p.link_timer_ == 0) {                     // reach the input buffer
    if (p.head_trace().id != p.destination_) {  // not reach destination
      if (microarchitecture_ == "OneStage") {
        onestage(p);
      } else if (microarchitecture_ == "TwoStage") {
        twostage(p);
      } else if (microarchitecture_ == "ThreeStage") {
        Threestage(p);
      } else {
        onestage(p);
      }

      /*switch (microarchitecture_) {
        case Router::OneStage:
          onestage(p);
          break;
        case Router::TwoStage:
          twostage(p);
          break;
        case Router::ThreeStage:
          Threestage(p);
          break;
        default:
          onestage(p);
          break;
      }*/
      if (!p.switch_allocated_) p.wait_timer_++;
    }
  } else {
    p.link_timer_--;
  }

  VCInfo temp1, temp2;
  int i = 0;

  if (p.switch_allocated_) {
    temp1 = p.next_vc_;
    p.wait_timer_ = 0;
    p.link_timer_ = p.next_vc_.buffer->channel_.latency;
    //TM->traffic_map_[{p.head_trace().id.node_id, temp1.id.node_id}]++;
    p.internal_hops_ += 1;
    p.routing_timer_ = routing_time_;
    p.VA_timer_ = vc_allocating_time_;
    p.SA_timer_ = sw_allocating_time_;
    p.candidate_channels_.clear();
    p.next_vc_ = VCInfo();
    p.switch_allocated_ = false;
  } else {
    temp1 = p.head_trace();
    // find the flit that fall behind the head flit
    while (i < p.length_ && p.flit_trace_[i].id == temp1.id) i++;
  }

  if (i < p.length_) {
    temp2 = p.flit_trace_[i];
    int k = temp1.buffer->channel_.width;  // linkwidth
    int j = 0;
    while (i < p.length_) {
      if (p.flit_trace_[i].id == temp2.id && j < k) {
        assert(p.flit_trace_[i].id != temp1.id);
        p.flit_trace_[i] = temp1;
        j++;
      } else {
        if (p.flit_trace_[i].id != temp2.id) {
          temp1 = temp2;
          temp2 = p.flit_trace_[i];
          k = temp1.buffer->channel_.width;
          j = 0;
          assert(p.flit_trace_[i].id != temp1.id);
          p.flit_trace_[i] = temp1;
          j++;
        } else {
          assert(j == k);
        }
      }
      i++;
    }
    // If last flit shift, realease link
    if (temp2.id != p.tail_trace().id) {
      p.releaselink_ = true;
      p.releasebuffer_ = temp2;
      if (temp2.buffer != nullptr) {
        temp2.buffer->release_buffer(temp2.vc, p.length_);
      }
    }
  }
  // If the last flit reach destination, delete message
  if (p.link_timer_ == 0 && p.tail_trace().id == p.destination_) {
    p.finished_ = true;
    TM->message_arrived_++;
    TM->total_cycles_ += p.trans_timer_;
    TM->total_parallel_hops_ += p.parallel_hops_;
    TM->total_serial_hops_ += p.serial_hops_;
    TM->total_internal_hops_ += p.internal_hops_;
    return;
  }
}
