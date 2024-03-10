#include <condition_variable>
#include <mutex>
#include <thread>

#include "traffic_manager.h"

Parameters* param;
TrafficManager* TM;
System* network;

static std::vector<std::thread> threads;
static volatile bool all_finished = false;
static std::condition_variable cv;
static std::atomic_uint64_t pkt_i;  // atomic counter, shared by all threads
// per thread vavriable
static std::mutex* mtxs;
static volatile bool* thread_ready;
static volatile bool* task_ready;

static void update_packets(std::vector<Packet*>& packets, System* system) {
  uint64_t i = pkt_i.load();
  uint64_t vec_size = packets.size();
  static int issue_width = param->issue_width;
  while (i < vec_size) {
    if (pkt_i.compare_exchange_strong(i, i + issue_width)) {
      uint64_t max_i = std::min(i + issue_width, vec_size);
      do {
        system->update(*packets[i]);
      } while (++i < max_i);
      i = pkt_i.load();
    }
  }
}

static void worker(std::vector<Packet*>& packets, System* s, int id) {
  std::unique_lock<std::mutex> lk(mtxs[id]);
  thread_ready[id] = true;
  cv.wait(lk);
  while (!all_finished) {
    update_packets(packets, s);
    task_ready[id] = false;
    while (!task_ready[id] && !all_finished) cv.wait(lk);
  }
}

static void run_one_cycle(std::vector<Packet*>& vec_pkts, System* system) {
  // iterate through all messages, update link status
  // single thread, fisrt come first serve
  uint64_t j = 0;
  uint64_t vecsize = vec_pkts.size();
  for (auto i = 0; i < vecsize; ++i) {
    Packet*& pkt = vec_pkts[i];
    if (pkt->releaselink_ == true) {
      pkt->tail_trace().buffer->release_in_link(*pkt);
      if (pkt->leaving_vc_.buffer != nullptr) // not leave the source node
        pkt->leaving_vc_.buffer->release_sw_link();
      else {
        assert(pkt->leaving_vc_.id == pkt->source_);
      }
      pkt->releaselink_ = false;
    }
    if (pkt->finished_) {
      delete pkt;
    } else {
      vec_pkts[j] = pkt;
      j++;
    }
  }
  vec_pkts.resize(j);

  pkt_i.store(0);
  if (vec_pkts.size() < param->threads * 1 || param->threads < 2) {
    update_packets(vec_pkts, system);
  } else {
    for (int i = 0; i < param->threads; ++i) {
      task_ready[i] = true;
      mtxs[i].unlock();
    }
    cv.notify_all();
    // wait all threads finish releasing links
    for (int i = 0; i < param->threads; ++i) {
      while (task_ready[i])
        ;
      mtxs[i].lock();
    }
  }
}

int main(int argc, char* argv[]) {
  std::string config_file;
  if (argc > 1) config_file = argv[1];
  param = new Parameters(config_file);
  network = System::New(param->topology);
  TM = new TrafficManager();
  srand(1);

  uint64_t timeout_limit = param->timeout_limit;
  float maximum_receiving_rate = 0;
  std::vector<Packet*> all_packets;

  // Multi-threads initialization
  if (param->threads > 1) {
    mtxs = new std::mutex[param->threads];
    thread_ready = new bool[param->threads];
    task_ready = new bool[param->threads];
    pkt_i.store(0);
    for (auto i = 0; i < param->threads; ++i) {
      thread_ready[i] = false;
      threads.push_back(std::thread(worker, std::ref(all_packets), network, i));
    }
    for (auto i = 0; i < param->threads; ++i) {  // wait for all threads ready
      while (!thread_ready[i])
        ;
      mtxs[i].lock();  // All threads are
    }
    // All threads are waiting for cv.notify_all()
  }

  if (param->traffic == "netrace") {
    TM->injection_rate_ = (float)TM->CTX->input_trheader->num_packets /
                          TM->CTX->input_trheader->num_cycles / network->num_cores_;
    for (uint64_t i = 0; i < TM->CTX->input_trheader->num_cycles + 1000; i++) {
      TM->genMes(all_packets, i);
      run_one_cycle(all_packets, network);
    }
    TM->print_statistics();
    nt_close_trfile(TM->CTX);
  } else
    while (true) {
      TM->injection_rate_ += param->injection_increment;

      //  warm up for 10% of the simulation time
      for (uint64_t i = 0; i < param->simulation_time / 10; i++) {
        TM->genMes(all_packets);
        run_one_cycle(all_packets, network);
      }
      TM->reset();
      for (uint64_t i = 0; i < param->simulation_time && TM->message_timeout_ < timeout_limit;
           i++) {
        TM->genMes(all_packets);
        run_one_cycle(all_packets, network);
      }
      TM->print_statistics();
      if (TM->receiving_rate() > maximum_receiving_rate)
        maximum_receiving_rate = TM->receiving_rate();
      // Saturated
      if (TM->message_arrived_ < (TM->message_timeout_ + all_packets.size()) * 5) {
        std::cout << std::endl
                  << "Saturation point!" << std::endl
                  << "Maximum average receiving traffic: " << maximum_receiving_rate
                  << " flits/(node*cycle)" << std::endl;
#ifdef DEBUG
        for (uint64_t i = 0; i < param->simulation_time * 2; i++) {  // try to drain
          run_one_cycle(all_packets, network);
          if (all_packets.size() == 0) {
            std::cerr << "No deadlock!" << std::endl;
            break;
          }
        }
        if (all_packets.size() != 0) std::cerr << "Possible Deadlock!" << std::endl;
#endif  // DEBUG
        break;
      }
      for (auto pkt : all_packets) delete pkt;
      all_packets.clear();
      network->reset();
      srand(1);
    }
  if (param->threads > 1) {
    all_finished = true;
    for (int i = 0; i < param->threads; ++i) {
      mtxs[i].unlock();
    }
    cv.notify_all();
    for (int i = 0; i < param->threads; ++i) {
      threads[i].join();
    }
    delete[] mtxs;
    delete[] thread_ready;
    delete[] task_ready;
  }
  delete TM;
  delete network;
  delete param;
  return 0;
}