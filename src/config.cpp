#include "config.h"

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

Parameters::Parameters(const std::string &config_file) {
  config_file_path = config_file;
  boost::property_tree::ptree params;
  if (!config_file_path.empty())
    boost::property_tree::ini_parser::read_ini(config_file_path, params);

  topology = params.get<std::string>("Network.topology", "SingleChipMesh");
  routing_algorithm = params.get<std::string>("Network.routing_algorithm", "XY");
  radix = params.get<int>("Network.radix", 16);
  scale = params.get<int>("Network.scale", 4);
  buffer_size = params.get<int>("Network.buffer_size", 16);
  IF_buffer_size = params.get<int>("Network.IF_buffer_size", 16);
  vc_number = params.get<int>("Network.vc_number", 1);
  router_stages = params.get<std::string>("Network.router_stages", "OneStage");
  processing_time = params.get<int>("Network.processing_time", 2);
  routing_time = params.get<int>("Network.routing_time", 0);
  vc_allocating_time = params.get<int>("Network.vc_allocating_time", 0);
  sw_allocating_time = params.get<int>("Network.sw_allocating_time", 0);

  traffic = params.get<std::string>("Workload.traffic", "uniform");
  packet_length = params.get<int>("Workload.packet_length", 1);

  injection_increment = params.get<double>("Simulation.injection_increment", 0.1);
  simulation_time = params.get<uint64_t>("Simulation.simulation_time", 10000);
  timeout_threshold = params.get<int>("Simulation.timeout_threshold", 200);
  timeout_limit = params.get<int>("Simulation.timeout_limit", 100);
  threads = params.get<int>("Simulation.threads", 0);

  if (traffic == "sd_traces")
    trace_file = params.get<std::string>("Files.trace_file");
  else if (traffic == "netrace")
    netrace_file = params.get<std::string>("Files.netrace_file");
  output_file = params.get<std::string>("Files.output_file", "../../output/output.csv");
  log_file = params.get<std::string>("Files.log_file", "../../output/log.txt");

  print_params();
}
