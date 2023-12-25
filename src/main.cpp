#include <condition_variable>
#include <mutex>
#include <thread>

#include "traffic_manager.h"

static std::vector<std::thread> threads;
static volatile bool All_finished = false;
static std::condition_variable cv;
static std::mutex* mtxs;
static volatile bool* thread_ready;
static volatile bool* task_ready;
static std::atomic_uint64_t pkt_i, pkt_j;

void run_one_cycle(std::vector<Packet*>& vecmess, System* s);
void release_links(std::vector<Packet*>& packets, System* s);
void update_packets(std::vector<Packet*>& packets, System* s);

void wait_for_task(std::vector<Packet*>& packets, System* s, int id) {
  std::unique_lock<std::mutex> lk(mtxs[id]);
  thread_ready[id] = true;
  cv.wait(lk);
  while (!All_finished) {
    update_packets(packets, s);
    task_ready[id] = false;
    while (!task_ready[id] && !All_finished) cv.wait(lk);
  }
}

void release_links(std::vector<Packet*>& packets, System* system) {
  uint64_t i = pkt_i.load();
  uint64_t j = pkt_j.load();
  uint64_t vecsize = packets.size();
  while (i < vecsize) {
    if (pkt_i.compare_exchange_strong(i, i + 1)) {
      Packet*& pkt = packets[i];
      if (pkt->releaselink_ == true) {
        pkt->tail_trace().buffer->release_link(*pkt);
        pkt->releaselink_ = false;
      }
      if (pkt->finished_) {
        delete pkt;
      } else {
        if (pkt_j.compare_exchange_strong(j, j + 1)) {
          packets[j] = pkt;
          j = pkt_j.load();
        }
      }
      i = pkt_i.load();
    }
  }
}

void update_packets(std::vector<Packet*>& packets, System* system) {
  uint64_t i = pkt_i.load();
  uint64_t vecsize = packets.size();
  while (i < vecsize) {
    if (pkt_i.compare_exchange_strong(i, i + 1)) {
      system->update(*packets[i]);
      i = pkt_i.load();
    }
  }
}

void run_one_cycle(std::vector<Packet*>& vec_pkts, System* system) {
  // iterate through all messages, update link status
  // single thread, fisrt come first serve
  uint64_t j;
  uint64_t curj = 0;
  pkt_i.store(0);
  pkt_j.store(0);
  uint64_t vecsize = vec_pkts.size();
  for (j = 0; j < vecsize; ++j) {
    Packet*& mess = vec_pkts[j];
    if (mess->releaselink_ == true) {
      mess->tail_trace().buffer->release_link(*mess);
      mess->releaselink_ = false;
    }
    if (mess->finished_) {
      delete mess;
    } else {
      vec_pkts[curj] = mess;
      curj++;
    }
  }
   vec_pkts.resize(curj);
  // vecsize = vecmess.size();
  vec_pkts.resize(pkt_j.load());

  pkt_i.store(0);
  if (vecsize < param->threads * 10 || param->threads < 2) {
    update_packets(vec_pkts, system);
  } else {
    for (int i = 0; i < param->threads; ++i) {
      task_ready[i] = true;
    }
    for (int i = 0; i < param->threads; ++i) {
      mtxs[i].unlock();
    }
    cv.notify_all();
    // wait all threads finish
    for (int i = 0; i < param->threads; ++i) {
      while (task_ready[i])
        ;
      mtxs[i].lock();
    }
  }
}

int main() {
  param = new Parameters();
  network = System::New(param->topology);
  TM = new TrafficManager();
  srand(1);

  uint64_t timeout_limit = 10000;
  float maximum_receiving_rate = 0;
  std::vector<Packet*> all_packets;

  // Multi-threads
  if (param->threads > 1) {
    mtxs = new std::mutex[param->threads];
    thread_ready = new bool[param->threads];
    task_ready = new bool[param->threads];
    pkt_i.store(0);
    for (int i = 0; i < param->threads; ++i) {
      thread_ready[i] = false;
      threads.push_back(std::thread(wait_for_task, std::ref(all_packets), network, i));
    }
    for (int i = 0; i < param->threads; ++i) {  // wait for all threads ready
      while (!thread_ready[i])
        ;
      mtxs[i].lock();
    }
  }

  while (true) {
    TM->injection_rate_ += param->injection_increment;

    //  warm up
    for (uint64_t i = 0; i < param->simulation_time / 10; i++) {
      TM->genMes(all_packets, i);
      run_one_cycle(all_packets, network);
    }

    TM->reset();
    for (uint64_t i = 0; i < param->simulation_time && TM->message_timeout_ < timeout_limit; i++) {
      TM->genMes(all_packets, i);
      run_one_cycle(all_packets, network);
    }
    TM->print_info();
    if (TM->receiving_rate() > maximum_receiving_rate)
      maximum_receiving_rate = TM->receiving_rate();

    // Saturated
    if (TM->message_arrived_ < (TM->message_timeout_ + all_packets.size()) * 5) {
      std::cout << std::endl
                << "Saturation point!" << std::endl
                << "Maximum average receiving traffic: " << maximum_receiving_rate
                << " flits/(node*cycle)" << std::endl;
#ifdef _DEBUG
      for (uint64_t i = 0; i < param->simulation_time * 2; i++) {  // try to drain
        run_one_cycle(all_packets, network);
      }
      if (all_packets.size() != 0)
        std::cerr << "Deadlock!" << std::endl;
      else
        std::cerr << "No deadlock!" << std::endl;
#endif  // DEBUG
      break;
    }
    for (auto mess : all_packets) delete mess;
    all_packets.clear();
    network->reset();
    srand(1);
  }
  if (param->threads > 1) {
    All_finished = true;
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