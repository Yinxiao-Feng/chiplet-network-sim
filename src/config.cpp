#include "config.h"

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

Parameters::Parameters(const std::string &config_file) {
  // default parameters
  // topology = Topology::DragonflySW;
  // topology = Topology::SingleChipMesh;
  // topology = Topology::DragonflyChiplet;

  // radix = 32;
  // scale = 4;
  // misrouting = false;

  // buffer_size = 16;
  // IF_buffer_size = 16;
  // vc_number = 2;

  // processing_time = 1;
  // microarchitecture = Router::OneStage;
  //// 0: each stage finish in one cycle, no extra delay
  // routing_time = 0;
  // vc_allocating_time = 0;
  // sw_allocating_time = 0;

  // injection_increment = 0.1;
  // simulation_time = 1000;
  // traffic = Traffic::uniform;
  // packet_length = 1;
  // timeout_threshold = 200;
  // threads = 16;

  boost::property_tree::ptree params;
  if (!config_file.empty()) boost::property_tree::ini_parser::read_ini(config_file, params);

  topology = params.get<std::string>("topology", "SingleChipMesh");
  radix = params.get<int>("radix", 16);
  scale = params.get<int>("scale", 4);
  misrouting = params.get<bool>("misrouting", false);

  buffer_size = params.get<int>("buffer_size", 16);
  IF_buffer_size = params.get<int>("IF_buffer_size", 16);
  vc_number = params.get<int>("vc_number", 2);

  processing_time = params.get<int>("processing_time", 1);
  microarchitecture = params.get<std::string>("microarchitecture", "OneStage");
  routing_time = params.get<int>("routing_time", 0);
  vc_allocating_time = params.get<int>("vc_allocating_time", 0);
  sw_allocating_time = params.get<int>("sw_allocating_time", 0);

  injection_increment = params.get<double>("injection_increment", 0.1);
  simulation_time = params.get<uint64_t>("simulation_time", 1000);
  traffic = params.get<std::string>("traffic", "uniform");
  packet_length = params.get<int>("packet_length", 1);
  timeout_threshold = params.get<int>("timeout_threshold", 200);
  timeout_limit = params.get<int>("timeout_limit", 100);
  threads = params.get<int>("threads", 16);

  if (traffic == "dc_traces")
    trace_file = params.get<std::string>("Files.trace_file");
  else if (traffic == "netrace")
    netrace_file = params.get<std::string>("Files.netrace_file");

  output_file = params.get<std::string>("Files.output_file", "../../output/output.csv");
  log_file = params.get<std::string>("Files.log_file", "../../output/log.txt");
}
