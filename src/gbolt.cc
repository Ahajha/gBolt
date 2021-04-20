#include <gbolt.h>
#include <common.h>
#include <cxxopts.hpp>

int main(int argc, char *argv[]) {
  cxxopts::Options options("gBolt", "very fast implementation for gSpan algorithm in data mining");
  options.add_options()
    ("i,input", "Input path of graph data", cxxopts::value<std::string>())
    ("o,output", "Output gbolt mining results", cxxopts::value<std::string>()->default_value(""))
    ("s,support", "Minimum subgraph frequency: (0.0, 1.0]", cxxopts::value<double>()->default_value("1.0"))
    ("m,mark", "Graph data separator", cxxopts::value<std::string>()->default_value(" "))
    ("p,parents", "Output subgraph parent ids",
     cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
    ("d,dfs", "Output subgraph dfs patterns",
     cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
    ("n,nodes", "Output frequent nodes",
     cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
    ("h,help", "gBolt help");

  if (argc == 1) {
    LOG_INFO(options.help().c_str());
    return 0;
  }

  auto result = options.parse(argc, argv);

  if (result["help"].count() || result["h"].count()) {
    LOG_INFO(options.help().c_str());
    return 0;
  }

  const std::string input = result["input"].as<std::string>();
  const std::string output = result["output"].as<std::string>();
  double support = result["support"].as<double>();
  const std::string mark = result["mark"].as<std::string>();
  bool parents = result["parents"].as<bool>();
  bool dfs = result["dfs"].as<bool>();
  bool nodes = result["nodes"].as<bool>();

  if (support > 1.0 || support <= 0.0) {
    LOG_ERROR("Support value should be less than 1.0 and greater than 0.0");
  }

  // Construct algorithm
  gbolt::GBolt gbolt(output, support);

  // Read input
  gbolt.read_input(input, mark);

  // Execute algorithm
  gbolt.execute();

  // Save results
  if (output.size() != 0) {
    #ifdef GBOLT_PERFORMANCE
    struct timeval time_start, time_end;
    double elapsed = 0.0;
    CPU_TIMER_START(elapsed, time_start);
    #endif
    gbolt.save(parents, dfs, nodes);
    #ifdef GBOLT_PERFORMANCE
    CPU_TIMER_END(elapsed, time_start, time_end);
    LOG_INFO("gbolt save output time: %f", elapsed);
    #endif
  }
  return 0;
}
