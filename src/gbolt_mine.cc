#include <gbolt.h>
#include <graph.h>
#include <common.h>
#include <sstream>

namespace gbolt {

void GBolt::find_frequent_nodes_and_edges(const Database& db) {
  unordered_map<int, vector<int> > vertex_labels;
  unordered_map<int, int> edge_labels;

  const auto& graphs = db.get_graphs();
  for (auto i = 0; i < graphs.size(); ++i) {
    unordered_set<int> vertex_set;
    unordered_set<int> edge_set;
    for (const auto& vertex : graphs[i].vertices) {
      vertex_set.insert(vertex.label);
    }
    for (const auto& edge : graphs[i].edges) {
      edge_set.insert(edge.label);
    }
    for (auto label : vertex_set) {
      vertex_labels[label].emplace_back(i);
    }
    for (auto label : edge_set) {
      ++edge_labels[label];
    }
  }
  for (const auto& kv_pair : vertex_labels) {
    if (kv_pair.second.size() >= nsupport_) {
      frequent_vertex_labels_.insert(kv_pair);
    }
  }
  for (const auto& kv_pair : edge_labels) {
    if (kv_pair.second >= nsupport_) {
      frequent_edge_labels_.insert(kv_pair);
    }
  }
}

void GBolt::report(const DfsCodes &dfs_codes, const Projection &projection,
  int nsupport, int prev_thread_id, int prev_graph_id) {
  std::stringstream ss;
  Graph graph;
  build_graph(dfs_codes, graph);

  for (const auto& vertex : graph.get_vertice()) {
    ss << "v " << vertex.id << " " << vertex.label << '\n';
  }
  for (const auto edge : dfs_codes) {
    ss << "e " << edge->from << " " << edge->to
      << " " << edge->edge_label << '\n';
  }
  ss << "x: ";
  int prev = 0;
  for (std::size_t i = 0; i < projection.size(); ++i) {
    if (i == 0 || projection[i].id != prev) {
      prev = projection[i].id;
      ss << prev << " ";
    }
  }
  ss << '\n';
  #ifdef GBOLT_SERIAL
  gbolt_instance_t *instance = gbolt_instances_;
  #else
  gbolt_instance_t *instance = gbolt_instances_ + omp_get_thread_num();
  #endif
  Output& output = *(instance->output);
  output.push_back(ss.str(), nsupport, output.size(), prev_thread_id, prev_graph_id);
}

void GBolt::save(bool output_parent, bool output_pattern, bool output_frequent_nodes) {
  #ifdef GBOLT_SERIAL
  Output& output = *(gbolt_instances_->output);
  output.save(output_parent, output_pattern);
  #else
  #pragma omp parallel
  {
    gbolt_instance_t *instance = gbolt_instances_ + omp_get_thread_num();
    Output& output = *(instance->output);
    output.save(output_parent, output_pattern);
  }
  #endif
  // Save output for frequent nodes
  if (output_frequent_nodes) {
    string output_file_nodes = output_file_ + ".nodes";
    output_frequent_nodes_ = new Output(output_file_nodes);

    int graph_id = 0;
    for (const auto& kv_pair : frequent_vertex_labels_) {
      std::stringstream ss;

      ss << "v 0 " + std::to_string(kv_pair.first);
      ss << '\n';
      ss << "x: ";
      for (auto g_id : kv_pair.second) {
        ss << g_id << " ";
      }
      ss << '\n';

      output_frequent_nodes_->push_back(ss.str(), kv_pair.second.size(), graph_id++);
    }
    output_frequent_nodes_->save(false, true);
  }
}

void GBolt::mine_subgraph(
  const Projection &projection,
  DfsCodes &dfs_codes,
  int prev_nsupport,
  int prev_thread_id,
  int prev_graph_id) {
  if (!is_min(dfs_codes)) {
    return;
  }
  report(dfs_codes, projection, prev_nsupport, prev_thread_id, prev_graph_id);
  #ifdef GBOLT_SERIAL
  prev_thread_id = 0;
  #else
  prev_thread_id = omp_get_thread_num();
  #endif
  gbolt_instance_t *instance = gbolt_instances_ + prev_thread_id;
  Output *output = instance->output;
  prev_graph_id = output->size() - 1;

  // Find right most path
  std::vector<int> *right_most_path = instance->right_most_path;
  right_most_path->clear();
  build_right_most_path(dfs_codes, *right_most_path);

  // Enumerate backward paths and forward paths by different rules
  ProjectionMapBackward projection_map_backward;
  ProjectionMapForward projection_map_forward;
  enumerate(dfs_codes, projection, *right_most_path,
    projection_map_backward, projection_map_forward);
  // Recursive mining: first backward, last backward, and then last forward to the first forward
  for (auto it = projection_map_backward.begin(); it != projection_map_backward.end(); ++it) {
    Projection &projection = it->second;
    int nsupport = count_support(projection);
    if (nsupport < nsupport_) {
      continue;
    }
    #ifdef GBOLT_SERIAL
    dfs_codes.emplace_back(&(it->first));
    mine_subgraph(projection, dfs_codes, nsupport, prev_thread_id, prev_graph_id);
    dfs_codes.pop_back();
    #else
    #pragma omp task shared(dfs_codes, projection, prev_thread_id, prev_graph_id) firstprivate(nsupport)
    {
      DfsCodes dfs_codes_copy(dfs_codes);
      dfs_codes_copy.emplace_back(&(it->first));
      mine_subgraph(projection, dfs_codes_copy, nsupport, prev_thread_id, prev_graph_id);
    }
    #endif
  }
  for (auto it = projection_map_forward.rbegin(); it != projection_map_forward.rend(); ++it) {
    Projection &projection = it->second;
    int nsupport = count_support(projection);
    if (nsupport < nsupport_) {
      continue;
    }
    #ifdef GBOLT_SERIAL
    dfs_codes.emplace_back(&(it->first));
    mine_subgraph(projection, dfs_codes, nsupport, prev_thread_id, prev_graph_id);
    dfs_codes.pop_back();
    #else
    #pragma omp task shared(dfs_codes, projection, prev_thread_id, prev_graph_id) firstprivate(nsupport)
    {
      DfsCodes dfs_codes_copy(dfs_codes);
      dfs_codes_copy.emplace_back(&(it->first));
      mine_subgraph(projection, dfs_codes_copy, nsupport, prev_thread_id, prev_graph_id);
    }
    #endif
  }
  #ifndef GBOLT_SERIAL
  #pragma omp taskwait
  #endif
}

}  // namespace gbolt
