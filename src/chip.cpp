#include "chip.h"
#include "system.h"

Chip::Chip() {
	system_ = nullptr;
	chip_id_ = 0;
	//port_number_ = 0;
	number_nodes_ = 0;
}

Chip::~Chip()
{
	for (auto node : nodes_) {
		delete node;
	}
	nodes_.clear();
}

void Chip::set_chip(System* system, int chip_id)
{
	system_ = system;
	chip_id_ = chip_id;
	for (int node_id = 0; node_id < number_nodes_; node_id++) {
		NodeID id(node_id, chip_id_);
		nodes_[node_id]->set_node(this, id);
	}
}

void Chip::reset() {
	for (auto node : nodes_) {
		node->reset();
	}
}

//Port Chip::get_port(int port_id)
//{
//	return ports_[port_id];
//}
