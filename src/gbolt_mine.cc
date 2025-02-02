#include <gbolt.h>
#include <graph.h>
#include <sstream>
#include <database.h>
#include <unordered_set>

namespace gbolt {

void GBolt::find_frequent_nodes_and_edges(const Database& db) {
  std::unordered_map<int, std::vector<int> > vertex_labels;
  std::unordered_map<int, int> edge_labels;

  const auto& graphs = db.get_graphs();
  for (auto i = 0; static_cast<size_t>(i) < graphs.size(); ++i) {
    std::unordered_set<int> vertex_set;
    std::unordered_set<int> edge_set;
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
    if (kv_pair.second.size() >= static_cast<size_t>(nsupport_)) {
      frequent_vertex_labels_.insert(kv_pair);
    }
  }
  for (const auto& kv_pair : edge_labels) {
    if (kv_pair.second >= nsupport_) {
      frequent_edge_labels_.insert(kv_pair);
    }
  }
}

void gbolt_instance_t::report(const DfsCodes &dfs_codes,
  const Projection &projection, int nsupport,
  int prev_thread_id, int prev_graph_id) {
  std::stringstream ss;

  // Min_graph is guaranteed to be built already.
  for (const auto& vertex : min_graph.vertice) {
    ss << "v " << vertex.id << ' ' << vertex.label << '\n';
  }
  for (const auto edge : dfs_codes) {
    ss << "e " << edge->from << ' ' << edge->to
      << ' ' << edge->edge_label << '\n';
  }
  ss << "x: ";
  int prev_id = -1;
  for (const auto& link : projection) {
    if (link.id != prev_id) {
      prev_id = link.id;
      ss << prev_id << ' ';
    }
  }
  ss << '\n';

  output.push_back(ss.str(), nsupport, prev_thread_id, prev_graph_id);
}

void GBolt::save(bool output_parent, bool output_pattern, bool output_frequent_nodes) {
  #ifndef GBOLT_SERIAL
  #pragma omp parallel
  #endif
  {
    thread_instance().output.save(output_parent, output_pattern);
  }
  // Save output for frequent nodes
  if (output_frequent_nodes) {
    std::string output_file_nodes = output_file_ + ".nodes";
    Output output_frequent_nodes_(output_file_nodes);

    for (const auto& kv_pair : frequent_vertex_labels_) {
      std::stringstream ss;

      ss << "v 0 " << kv_pair.first;
      ss << '\n';
      ss << "x: ";
      for (auto g_id : kv_pair.second) {
        ss << g_id << ' ';
      }
      ss << '\n';

      output_frequent_nodes_.push_back(ss.str(), kv_pair.second.size());
    }
    output_frequent_nodes_.save(false, true);
  }
}

void GBolt::mine_subgraph(
  const Projection &projection,
  DfsCodes &dfs_codes) {
  gbolt_instance_t& instance = thread_instance();

  const int prev_thread_id = thread_id();
  const int prev_graph_id = instance.output.size() - 1;

  // Enumerate backward paths and forward paths by different rules
  ProjectionMapBackward projection_map_backward;
  ProjectionMapForward projection_map_forward;
  enumerate(dfs_codes, projection, instance.right_most_path,
    projection_map_backward, projection_map_forward);
  // Recursive mining: first backward, last backward, and then last forward to the first forward
  for (auto it = projection_map_backward.begin(); it != projection_map_backward.end(); ++it) {
    mine_child(it->second, it->first, dfs_codes, prev_thread_id, prev_graph_id);
  }
  for (auto it = projection_map_forward.rbegin(); it != projection_map_forward.rend(); ++it) {
    mine_child(it->second, it->first, dfs_codes, prev_thread_id, prev_graph_id);
  }
  #ifndef GBOLT_SERIAL
  #pragma omp taskwait
  #endif
}

void GBolt::mine_child(
  const Projection &projection,
  const dfs_code_t& next_code,
  DfsCodes &dfs_codes,
  int prev_thread_id,
  int prev_graph_id) {
  // Partial pruning, like apriori
  const int nsupport = count_support(projection);
  if (nsupport < nsupport_) {
    return;
  }
  #ifndef GBOLT_SERIAL
  #pragma omp task shared(projection, prev_thread_id, prev_graph_id, nsupport) firstprivate(dfs_codes)
  #endif
  {
    dfs_codes.emplace_back(&next_code);
    gbolt_instance_t& instance = thread_instance();
    if (instance.is_min(dfs_codes)) {
      instance.report(dfs_codes, projection, nsupport, prev_thread_id, prev_graph_id);
      mine_subgraph(projection, dfs_codes);
    }
    #ifdef GBOLT_SERIAL
    dfs_codes.pop_back();
    #endif
  }
}

}  // namespace gbolt
