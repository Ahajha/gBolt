#include <gbolt.h>
#include <history.h>
#include <algorithm>


namespace gbolt {

int GBolt::count_support(const Projection &projection) {
  int prev_id = -1;
  int size = 0;

  for (const auto& link : projection) {
    if (prev_id != link.id) {
      prev_id = link.id;
      ++size;
    }
  }
  return size;
}

void GBolt::build_graph(const DfsCodes &dfs_codes, Graph &graph) {
  int edge_id = 0;
  Vertice *vertice = graph.get_p_vertice();

  for (const auto edge : dfs_codes) {
    // Push vertice
    vertice->resize(std::max(edge->from + 1, static_cast<int>(vertice->size())));
    vertice->resize(std::max(edge->to + 1, static_cast<int>(vertice->size())));
    (*vertice)[edge->from].label = edge->from_label;
    (*vertice)[edge->from].id = edge->from;
    (*vertice)[edge->to].label = edge->to_label;
    (*vertice)[edge->to].id = edge->to;
    // Push an edge
    (*vertice)[edge->from].edges.emplace_back(
      edge->from, edge->edge_label, edge->to, edge_id);
    (*vertice)[edge->to].edges.emplace_back(
      edge->to, edge->edge_label, edge->from, edge_id);
    ++edge_id;
  }
  graph.set_nedges(edge_id);
}

bool GBolt::is_min(const DfsCodes &dfs_codes) {
  if (dfs_codes.size() == 1)
    return true;

  // Reuse memory, TODO: necessary or not? just set an index?
  #ifdef GBOLT_SERIAL
  gbolt_instance_t *instance = gbolt_instances_;
  #else
  gbolt_instance_t *instance = gbolt_instances_ + omp_get_thread_num();
  #endif
  Graph *min_graph = instance->min_graph;
  DfsCodes *min_dfs_codes = instance->min_dfs_codes;
  History *history = instance->history;
  MinProjection *min_projection = instance->min_projection;
  std::vector<int> *right_most_path = instance->right_most_path;

  // Clear cache data structures
  min_graph->clear();
  min_dfs_codes->clear();
  min_projection->clear();
  right_most_path->clear();
  // Build min graph
  build_graph(dfs_codes, *min_graph);

  dfs_code_t min_dfs_code;
  bool first_dfs_code = true;

  for (const auto& vertex : *(min_graph->get_p_vertice())) {
    Edges edges;

    if (get_forward_init(vertex, *min_graph, edges)) {
      for (const auto& edge : edges) {
        // Push dfs code according to the same edge label
        const vertex_t *vertex_from = min_graph->get_p_vertex(edge->from);
        const vertex_t *vertex_to = min_graph->get_p_vertex(edge->to);
        dfs_code_t dfs_code(0, 1, vertex_from->label, edge->label, vertex_to->label);
        // Push back all the graphs
        if (first_dfs_code || dfs_code_project_compare_(dfs_code, min_dfs_code)) {
          first_dfs_code = false;
          min_dfs_code = dfs_code;
          min_projection->clear();
        }
        if (dfs_code == min_dfs_code) {
          min_projection->emplace_back(edge, -1);
        }
      }
    }
  }
  min_dfs_codes->push_back(&min_dfs_code);
  if (*(dfs_codes[0]) != min_dfs_code) {
    return false;
  }
  build_right_most_path(*min_dfs_codes, *right_most_path);
  return is_projection_min(dfs_codes, *min_graph, *history,
    *min_dfs_codes, *right_most_path, *min_projection, 0);
}

bool GBolt::judge_backward(
  const std::vector<int> &right_most_path,
  const Graph &min_graph,
  History &history,
  dfs_code_t &min_dfs_code,
  DfsCodes &min_dfs_codes,
  MinProjection &projection,
  size_t projection_start_index,
  size_t projection_end_index) {
  bool first_dfs_code = true;

  // i > 1, because it cannot reach the path itself
  for (auto i = right_most_path.size(); i > 1; --i) {
    for (auto j = projection_start_index; j < projection_end_index; ++j) {
      history.build_edges_min(projection, min_graph, j);

      const edge_t *edge = history.get_p_edge(right_most_path[i - 1]);
      const edge_t *last_edge = history.get_p_edge(right_most_path[0]);
      const vertex_t *from_node = min_graph.get_p_vertex(edge->from);
      const vertex_t *last_node = min_graph.get_p_vertex(last_edge->to);
      const vertex_t *to_node = min_graph.get_p_vertex(edge->to);

      for (const auto& ln_edge : last_node->edges) {
        if (history.has_edges(ln_edge.id))
          continue;
        if (ln_edge.to == edge->from &&
            (ln_edge.label > edge->label ||
             (ln_edge.label == edge->label &&
              last_node->label >= to_node->label))) {
          int from_id = min_dfs_codes[right_most_path[0]]->to;
          int to_id = min_dfs_codes[right_most_path[i - 1]]->from;
          dfs_code_t dfs_code(from_id, to_id,
            last_node->label, ln_edge.label, from_node->label);
          if (first_dfs_code || dfs_code_backward_compare_(dfs_code, min_dfs_code)) {
            first_dfs_code = false;
            min_dfs_code = dfs_code;
            projection.resize(projection_end_index);
          }
          if (dfs_code == min_dfs_code) {
            projection.emplace_back(&ln_edge, j);
          }
        }
      }
    }
    if (projection.size() > projection_end_index)
      return true;
  }
  return false;
}

bool GBolt::judge_forward(
  const std::vector<int> &right_most_path,
  const Graph &min_graph,
  History &history,
  dfs_code_t &min_dfs_code,
  DfsCodes &min_dfs_codes,
  MinProjection &projection,
  size_t projection_start_index,
  size_t projection_end_index) {
  int min_label = min_dfs_codes[0]->from_label;
  bool first_dfs_code = true;

  for (auto i = projection_start_index; i < projection_end_index; ++i) {
    history.build_vertice_min(projection, min_graph, i);

    const edge_t *last_edge = history.get_p_edge(right_most_path[0]);
    const vertex_t *last_node = min_graph.get_p_vertex(last_edge->to);

    for (const auto& ln_edge : last_node->edges) {
      const vertex_t *to_node = min_graph.get_p_vertex(ln_edge.to);
      if (history.has_vertice(ln_edge.to) || to_node->label < min_label)
        continue;
      int to_id = min_dfs_codes[right_most_path[0]]->to;
      dfs_code_t dfs_code(to_id, to_id + 1, last_node->label, ln_edge.label, to_node->label);
      if (first_dfs_code || dfs_code_forward_compare_(dfs_code, min_dfs_code)) {
        first_dfs_code = false;
        min_dfs_code = dfs_code;
        projection.resize(projection_end_index);
      }
      if (dfs_code == min_dfs_code) {
        projection.emplace_back(&ln_edge, i);
      }
    }
  }

  if (projection.size() == projection_end_index) {
    for (auto i : right_most_path) {
      for (auto j = projection_start_index; j < projection_end_index; ++j) {
        history.build_vertice_min(projection, min_graph, j);

        const edge_t *cur_edge = history.get_p_edge(i);
        const vertex_t *cur_node = min_graph.get_p_vertex(cur_edge->from);
        const vertex_t *cur_to = min_graph.get_p_vertex(cur_edge->to);

        for (const auto& cn_edge : cur_node->edges) {
          const vertex_t *to_node = min_graph.get_p_vertex(cn_edge.to);
          if (history.has_vertice(to_node->id) || cur_edge->to == to_node->id || to_node->label < min_label)
            continue;
          if (cur_edge->label < cn_edge.label ||
              (cur_edge->label == cn_edge.label &&
               cur_to->label <= to_node->label)) {
            int from_id = min_dfs_codes[i]->from;
            int to_id = min_dfs_codes[right_most_path[0]]->to;
            dfs_code_t dfs_code(from_id, to_id + 1,
              cur_node->label, cn_edge.label, to_node->label);
            if (first_dfs_code || dfs_code_forward_compare_(dfs_code, min_dfs_code)) {
              first_dfs_code = false;
              min_dfs_code = dfs_code;
              projection.resize(projection_end_index);
            }
            if (dfs_code == min_dfs_code) {
              projection.emplace_back(&cn_edge, j);
            }
          }
        }
      }
      if (projection.size() > projection_end_index) {
        break;
      }
    }
  }
  if (projection.size() > projection_end_index)
    return true;
  else 
    return false;
}

bool GBolt::is_projection_min(
  const DfsCodes &dfs_codes,
  const Graph &min_graph,
  History &history,
  DfsCodes &min_dfs_codes,
  std::vector<int> &right_most_path,
  MinProjection &projection,
  size_t projection_start_index) {
  dfs_code_t min_dfs_code;
  size_t projection_end_index = projection.size();

  if (judge_backward(right_most_path, min_graph, history, min_dfs_code, min_dfs_codes,
    projection, projection_start_index, projection_end_index)) {
    min_dfs_codes.emplace_back(&min_dfs_code);
    // Dfs code not equals to min dfs code
    if (*(dfs_codes[min_dfs_codes.size() - 1]) != min_dfs_code) {
      return false;
    }
    // Current dfs code is min
    update_right_most_path(min_dfs_codes, right_most_path);
    return is_projection_min(dfs_codes, min_graph, history,
      min_dfs_codes, right_most_path, projection, projection_end_index);
  }

  if (judge_forward(right_most_path, min_graph, history, min_dfs_code, min_dfs_codes,
    projection, projection_start_index, projection_end_index)) {
    min_dfs_codes.emplace_back(&min_dfs_code);
    // Dfs code not equals to min dfs code
    if (*(dfs_codes[min_dfs_codes.size() - 1]) != min_dfs_code) {
      return false;
    }
    // Current dfs code is min
    update_right_most_path(min_dfs_codes, right_most_path);
    return is_projection_min(dfs_codes, min_graph, history,
      min_dfs_codes, right_most_path, projection, projection_end_index);
  }

  return true;
}

}  // namespace gbolt
