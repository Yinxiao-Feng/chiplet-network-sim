#include "config.h"

Parameters::Parameters(const std::string &config_file) {
  config_file_path = config_file;
  params_ptree = boost::property_tree::ptree();
  if (!config_file_path.empty())
    boost::property_tree::ini_parser::read_ini(config_file_path, params_ptree);

  topology = params_ptree.get<std::string>("Network.topology", "SingleChipMesh");
  buffer_size = params_ptree.get<int>("Network.buffer_size", 16);
  vc_number = params_ptree.get<int>("Network.vc_number", 1);
  router_stages = params_ptree.get<std::string>("Network.router_stages", "ThreeStage");
  on_chip_latency = params_ptree.get<int>("Network.on_chip_latency", 0);
  processing_time = params_ptree.get<int>("Network.processing_time", 2);
  routing_time = params_ptree.get<int>("Network.routing_time", 0);
  vc_allocating_time = params_ptree.get<int>("Network.vc_allocating_time", 0);
  sw_allocating_time = params_ptree.get<int>("Network.sw_allocating_time", 0);

  traffic = params_ptree.get<std::string>("Workload.traffic", "uniform");
  packet_length = params_ptree.get<int>("Workload.packet_length", 5);

  injection_increment = params_ptree.get<float>("Simulation.injection_increment", 0.1);
  simulation_time = params_ptree.get<uint64_t>("Simulation.simulation_time", 10000);
  warmup_time = params_ptree.get<uint64_t>("Simulation.warmup_time", 1000);
  timeout_threshold = params_ptree.get<int>("Simulation.timeout_threshold", 200);
  timeout_limit = params_ptree.get<int>("Simulation.timeout_limit", 100);
  threads = params_ptree.get<int>("Simulation.threads", 0);
  if (threads >= 2) issue_width = params_ptree.get<int>("Simulation.issue_width", 10);
  else issue_width = 1000;

  if (traffic == "sd_traces")
    trace_file = params_ptree.get<std::string>("Files.trace_file");
  else if (traffic == "netrace")
    netrace_file = params_ptree.get<std::string>("Files.netrace_file");
  output_file = params_ptree.get<std::string>("Files.output_file", "../../output/output.csv");
  log_file = params_ptree.get<std::string>("Files.log_file", "../../output/log.txt");

  print_params();
}
