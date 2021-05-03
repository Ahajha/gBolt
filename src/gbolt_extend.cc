#include <gbolt.h>
#include <history.h>
#include <common.h>

namespace gbolt {

void GBolt::enumerate(
  const DfsCodes &dfs_codes,
  const Projection &projection,
  const std::vector<int> &right_most_path,
  ProjectionMapBackward &projection_map_backward,
  ProjectionMapForward &projection_map_forward) {
  History& history = *(thread_instance().history);
  for (const auto& link : projection) {
    const Graph &graph = graphs_[link.id];
    history.build(link, graph);

    get_backward(link, history, graph, dfs_codes, right_most_path,
      projection_map_backward);
    get_first_forward(link, history, graph, dfs_codes, right_most_path,
      projection_map_forward);
    get_other_forward(link, history, graph, dfs_codes, right_most_path,
      projection_map_forward);
  }
}

void GBolt::get_backward(
  const prev_dfs_t &prev_dfs,
  const History &history,
  const Graph &graph,
  const DfsCodes &dfs_codes,
  const std::vector<int> &right_most_path,
  ProjectionMapBackward &projection_map_backward) {
  const edge_t& last_edge = history.get_edge(right_most_path[0]);
  const vertex_t& last_node = graph.vertice[last_edge.to];

  const int from_id = dfs_codes[right_most_path[0]]->to;
  for (auto i = right_most_path.size() - 1; i > 0; --i) {
    const edge_t& edge = history.get_edge(right_most_path[i]);
    const int from_node_label = graph.vertice[edge.from].label;
    const int to_node_label = graph.vertice[edge.to].label;
    const int to_id = dfs_codes[right_most_path[i]]->from;
    for (const auto& ln_edge : last_node.edges) {
      if (history.has_edges(ln_edge.id))
        continue;
      if (ln_edge.to == edge.from &&
          (ln_edge.label > edge.label ||
           (ln_edge.label == edge.label &&
            last_node.label >= to_node_label))) {
        dfs_code_t dfs_code(from_id, to_id,
          last_node.label, ln_edge.label, from_node_label);
        projection_map_backward[dfs_code].
          emplace_back(graph.id, &ln_edge, &prev_dfs);
      }
    }
  }
}

void GBolt::get_first_forward(
  const prev_dfs_t &prev_dfs,
  const History &history,
  const Graph &graph,
  const DfsCodes &dfs_codes,
  const std::vector<int> &right_most_path,
  ProjectionMapForward &projection_map_forward) {
  const edge_t& last_edge = history.get_edge(right_most_path[0]);
  const vertex_t& last_node = graph.vertice[last_edge.to];
  const int min_label = dfs_codes[0]->from_label;

  for (const auto& ln_edge : last_node.edges) {
    const int to_node_label = graph.vertice[ln_edge.to].label;
    // Partial pruning: if this label is less than the minimum label, then there
    // should exist another lexicographical order which renders the same letters, but
    // in the asecending order.
    // Could we perform the same partial pruning as other extending methods?
    // No, we cannot, for this time, the extending id is greater the the last node
    if (history.has_vertice(ln_edge.to) || to_node_label < min_label)
      continue;
    const int to_id = dfs_codes[right_most_path[0]]->to;
    dfs_code_t dfs_code(to_id, to_id + 1,
      last_node.label, ln_edge.label, to_node_label);
    projection_map_forward[dfs_code].
      emplace_back(graph.id, &ln_edge, &prev_dfs);
  }
}

void GBolt::get_other_forward(
  const prev_dfs_t &prev_dfs,
  const History &history,
  const Graph &graph,
  const DfsCodes &dfs_codes,
  const std::vector<int> &right_most_path,
  ProjectionMapForward &projection_map_forward) {
  const int min_label = dfs_codes[0]->from_label;

  const int to_id = dfs_codes[right_most_path[0]]->to;
  for (auto i : right_most_path) {
    const edge_t& cur_edge = history.get_edge(i);
    const vertex_t& cur_node = graph.vertice[cur_edge.from];
    const vertex_t& cur_to = graph.vertice[cur_edge.to];
    const int from_id = dfs_codes[i]->from;

    for (const auto& cn_edge : cur_node.edges) {
      const vertex_t& to_node = graph.vertice[cn_edge.to];
      // Partial pruning: guarantees that extending label is greater
      // than the minimum one
      if (history.has_vertice(to_node.id) ||
        to_node.id == cur_to.id || to_node.label < min_label)
        continue;
      if (cur_edge.label < cn_edge.label ||
          (cur_edge.label == cn_edge.label &&
           cur_to.label <= to_node.label)) {
        dfs_code_t dfs_code(from_id, to_id + 1, cur_node.label,
          cn_edge.label, to_node.label);
        projection_map_forward[dfs_code].
          emplace_back(graph.id, &cn_edge, &prev_dfs);
      }
    }
  }
}

}  // namespace gbolt
