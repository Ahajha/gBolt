#include <gbolt.h>
#include <database.h>
#include <common.h>
#include <algorithm>

namespace gbolt {

void GBolt::read_input(const string &input_file, const string &separator) {
  Database db;

  #ifdef GBOLT_PERFORMANCE
  struct timeval time_start, time_end;
  double elapsed = 0.0;
  CPU_TIMER_START(elapsed, time_start);
  #endif

  db.read_input(input_file, separator);

  #ifdef GBOLT_PERFORMANCE
  CPU_TIMER_END(elapsed, time_start, time_end);
  LOG_INFO("gbolt read input time: %f", elapsed);
  CPU_TIMER_START(elapsed, time_start);
  #endif

  nsupport_ = db.get_graphs().size() * support_;

  find_frequent_nodes_and_edges(db);

  // Prune the initial graph by frequent labels
  db.construct_graphs(frequent_vertex_labels_, frequent_edge_labels_, graphs_);

  #ifdef GBOLT_PERFORMANCE
  CPU_TIMER_END(elapsed, time_start, time_end);
  LOG_INFO("gbolt construct graph time: %f", elapsed);
  #endif
}

void GBolt::execute() {
  #ifdef GBOLT_PERFORMANCE
  struct timeval time_start, time_end;
  double elapsed = 0.0;
  CPU_TIMER_START(elapsed, time_start);
  #endif

  // Graph mining
  init_instances();
  project();

  #ifdef GBOLT_PERFORMANCE
  CPU_TIMER_END(elapsed, time_start, time_end);
  LOG_INFO("gbolt mine graph time: %f", elapsed);
  #endif
}

void GBolt::init_instances() {
  #ifdef GBOLT_SERIAL
  int num_threads = 1;
  #else
  int num_threads = omp_get_max_threads();
  #endif
  gbolt_instances_ = new gbolt_instance_t[num_threads];

  // Prepare history instance
  int max_edges = 0;
  int max_vertice = 0;
  for (const auto& graph : graphs_) {
    max_edges = std::max(graph.get_nedges(), max_edges);
    max_vertice = std::max(
      static_cast<int>(graph.size()), max_vertice);
  }

  // Init an instance for each thread
  for (auto i = 0; i < num_threads; ++i) {
    #ifdef GBOLT_PERFORMANCE
    LOG_INFO("gbolt create thread %d", i);
    #endif
    string output_file_thread = output_file_ + ".t" + std::to_string(i);
    gbolt_instances_[i].history = new History(max_edges, max_vertice);
    gbolt_instances_[i].output = new Output(output_file_thread);
    gbolt_instances_[i].min_graph = new Graph();
    gbolt_instances_[i].min_dfs_codes = new DfsCodes();
    gbolt_instances_[i].right_most_path = new std::vector<int>(DEFAULT_PATH_LEN);
    gbolt_instances_[i].min_projection = new MinProjection(DEFAULT_PATH_LEN);
  }
}

void GBolt::project() {
  ProjectionMap projection_map;

  // Construct the first edge
  for (const auto& graph : graphs_) {

    for (const auto& vertex : graph.get_vertice()) {

      for (const auto& edge : vertex.edges) {
        // Partial pruning: if the first label is greater than the
        // second label, then there must be another graph whose second
        // label is greater than the first label.
        const int vertex_to_label = graph.get_vertex(edge.to).label;
        if (vertex.label <= vertex_to_label) {
          // Push dfs code according to the same edge label
          dfs_code_t dfs_code(0, 1, vertex.label, edge.label, vertex_to_label);
          // Push all the graphs
          projection_map[dfs_code].emplace_back(graph.get_id(), &edge, nullptr);
        }
      }
    }
  }
  // Mine subgraphs
  int prev_graph_id = -1;
  #ifdef GBOLT_SERIAL
  int prev_thread_id = 1;
  #else
  int prev_thread_id = omp_get_thread_num();
  #endif
  DfsCodes dfs_codes;
  #ifndef GBOLT_SERIAL
  #pragma omp parallel
  #pragma omp single nowait
  #endif
  {
    for (const auto& kv_pair : projection_map) {
      // Parital pruning, like apriori
      const Projection &projection = kv_pair.second;
      int nsupport = count_support(projection);
      if (nsupport < nsupport_) {
        continue;
      }
      #ifdef GBOLT_SERIAL
      dfs_codes.emplace_back(&(kv_pair.first));
      mine_subgraph(projection, dfs_codes, nsupport, prev_thread_id, prev_graph_id);
      dfs_codes.pop_back();
      #else
      #pragma omp task shared(projection, prev_thread_id, prev_graph_id) firstprivate(dfs_codes, nsupport)
      {
        dfs_codes.emplace_back(&(kv_pair.first));
        mine_subgraph(projection, dfs_codes, nsupport, prev_thread_id, prev_graph_id);
      }
      #endif
    }
  }
  #ifndef GBOLT_SERIAL
  #pragma omp taskwait
  #endif
}

}  // namespace gbolt
