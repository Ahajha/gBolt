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

void gbolt_instance_t::build_min_graph(const DfsCodes &dfs_codes) {
  int edge_id = 0;
  Vertice& vertice = min_graph.vertice;
  vertice.clear();

  // New size is just large enough to ensure no extra vertices at the end.
  // The max vertex id is either the last code's 'from' (if it is backwards),
  // or the last code's 'to' (if it is forwards). So just take the max of the two.
  const int max_v_id = std::max(dfs_codes.back()->from, dfs_codes.back()->to);
  vertice.resize(max_v_id + 1);

  for (const auto edge : dfs_codes) {
    // Push vertice
    vertice[edge->from].label = edge->from_label;
    vertice[edge->from].id = edge->from;
    vertice[edge->to].label = edge->to_label;
    vertice[edge->to].id = edge->to;
    // Push an edge
    vertice[edge->from].edges.emplace_back(
      edge->from, edge->edge_label, edge->to, edge_id);
    vertice[edge->to].edges.emplace_back(
      edge->to, edge->edge_label, edge->from, edge_id);
    ++edge_id;
  }
  min_graph.nedges = edge_id;
}

void gbolt_instance_t::update_right_most_path(const DfsCodes &dfs_codes, size_t size) {
  right_most_path.clear();
  int prev_id = -1;

  // Go in reverse, since we need to first look for the edge that discovered
  // the rightmost vertex
  for (auto i = size; i > 0; --i) {
    // Only consider forward edges (as by definition the rightmost path only
    // consists of edges 'discovering' new nodes). The first forward edge (or
    // equivalently, the last forward edge in dfs_codes) is the edge discovering
    // the rightmost vertex. After that, each new edge is the edge discovering
    // the 'from' of the previous one.
    if (dfs_codes[i - 1]->from < dfs_codes[i - 1]->to &&
      (right_most_path.empty() || prev_id == dfs_codes[i - 1]->to)) {
      prev_id = dfs_codes[i - 1]->from;
      right_most_path.push_back(i - 1);
    }
  }
}

bool gbolt_instance_t::is_min(const DfsCodes &dfs_codes) {
  build_min_graph(dfs_codes);

  if (dfs_codes.size() == 1) {
    update_right_most_path(dfs_codes, 1);
    return true;
  }

  min_projection.clear();

  // The first code in the sequence must be the
  // smallest if the sequence itself is minimal.
  const dfs_code_t& min_dfs_code = *(dfs_codes.front());

  for (const auto& vertex : min_graph.vertice) {

    for (const auto& edge : vertex.edges) {

      // Partial pruning: if the first label is greater than the
      // second label, then there must be another graph whose second
      // label is greater than the first label.
      const int vertex_to_label = min_graph.vertice[edge.to].label;
      if (vertex.label <= vertex_to_label) {

        // Hypothetical DFS code of this edge if it were the first edge in
        // a code sequence
        dfs_code_t dfs_code(0, 1, vertex.label, edge.label, vertex_to_label);

        // If this DFS code is smaller than the first code, the code sequence
        // is not minimal.
        if (dfs_code_project_compare_t{}(dfs_code, min_dfs_code)) {
          return false;
        }

        // If the code is minimal, it is an instance of this code sequence.
        if (dfs_code == min_dfs_code) {
          min_projection.emplace_back(&edge, -1);
        }
      }
    }
  }

  update_right_most_path(dfs_codes, 1);
  return is_projection_min(dfs_codes);
}

bool gbolt_instance_t::exists_backwards(const size_t projection_start_index) {
  const size_t projection_end_index = min_projection.size();
  for (auto j = projection_start_index; j < projection_end_index; ++j) {
    history.build_edges_min(min_projection, min_graph, j);
    const edge_t& last_edge = history.get_edge(right_most_path[0]);
    const vertex_t& last_node = min_graph.vertice[last_edge.to];
    for (const auto& ln_edge : last_node.edges) {
      if (history.has_edges(ln_edge.id))
        continue;
      // i > 0, because a backward edge cannot go to the last vertex.
      for (auto i = right_most_path.size() - 1; i > 0; --i) {

        const edge_t& edge = history.get_edge(right_most_path[i]);
        const vertex_t& to_node = min_graph.vertice[edge.to];

        if (ln_edge.to == edge.from &&
            (ln_edge.label > edge.label ||
             (ln_edge.label == edge.label &&
              last_node.label >= to_node.label))) {
          return true;
        }
      }
    }
  }
  return false;
}

bool gbolt_instance_t::is_backward_min(
  const DfsCodes& dfs_codes,
  const dfs_code_t &min_dfs_code,
  const size_t projection_start_index) {
  const size_t projection_end_index = min_projection.size();

  // i > 0, because a backward edge cannot go to the last vertex.
  const int from_id = dfs_codes[right_most_path[0]]->to;
  for (auto i = right_most_path.size() - 1; i > 0; --i) {
    const int to_id = dfs_codes[right_most_path[i]]->from;
    for (auto j = projection_start_index; j < projection_end_index; ++j) {
      history.build_edges_min(min_projection, min_graph, j);

      const edge_t& edge = history.get_edge(right_most_path[i]);
      const edge_t& last_edge = history.get_edge(right_most_path[0]);
      const vertex_t& from_node = min_graph.vertice[edge.from];
      const vertex_t& last_node = min_graph.vertice[last_edge.to];
      const vertex_t& to_node = min_graph.vertice[edge.to];

      for (const auto& ln_edge : last_node.edges) {
        if (history.has_edges(ln_edge.id))
          continue;
        if (ln_edge.to == edge.from &&
            (ln_edge.label > edge.label ||
             (ln_edge.label == edge.label &&
              last_node.label >= to_node.label))) {
          dfs_code_t dfs_code(from_id, to_id,
            last_node.label, ln_edge.label, from_node.label);
          // A smaller code was found, so the given code is not minimal.
          if (dfs_code_backward_compare_t{}(dfs_code, min_dfs_code)) {
            return false;
          }
          if (dfs_code == min_dfs_code) {
            min_projection.emplace_back(&ln_edge, j);
          }
        }
      }
    }
    if (min_projection.size() > projection_end_index)
      return true;
  }
  return false;
}

bool gbolt_instance_t::is_forward_min(
  const DfsCodes& dfs_codes,
  const dfs_code_t &min_dfs_code,
  const size_t projection_start_index) {
  const size_t projection_end_index = min_projection.size();
  const int min_label = dfs_codes[0]->from_label;

  const int max_id = dfs_codes[right_most_path[0]]->to;
  for (auto i = projection_start_index; i < projection_end_index; ++i) {
    history.build_vertice_min(min_projection, min_graph, i);

    const edge_t& last_edge = history.get_edge(right_most_path[0]);
    const vertex_t& last_node = min_graph.vertice[last_edge.to];

    for (const auto& ln_edge : last_node.edges) {
      const vertex_t& to_node = min_graph.vertice[ln_edge.to];
      if (history.has_vertice(ln_edge.to) || to_node.label < min_label)
        continue;
      dfs_code_t dfs_code(max_id, max_id + 1, last_node.label, ln_edge.label, to_node.label);
      // A smaller code was found, so the given code is not minimal.
      if (dfs_code_forward_compare_t{}(dfs_code, min_dfs_code)) {
        return false;
      }
      if (dfs_code == min_dfs_code) {
        min_projection.emplace_back(&ln_edge, i);
      }
    }
  }

  if (min_projection.size() == projection_end_index) {
    for (auto i : right_most_path) {
      const int from_id = dfs_codes[i]->from;
      for (auto j = projection_start_index; j < projection_end_index; ++j) {
        history.build_vertice_min(min_projection, min_graph, j);

        const edge_t& cur_edge = history.get_edge(i);
        const vertex_t& cur_node = min_graph.vertice[cur_edge.from];
        const vertex_t& cur_to = min_graph.vertice[cur_edge.to];

        for (const auto& cn_edge : cur_node.edges) {
          const vertex_t& to_node = min_graph.vertice[cn_edge.to];
          if (history.has_vertice(to_node.id) || cur_edge.to == to_node.id || to_node.label < min_label)
            continue;
          if (cur_edge.label < cn_edge.label ||
              (cur_edge.label == cn_edge.label &&
               cur_to.label <= to_node.label)) {
            dfs_code_t dfs_code(from_id, max_id + 1,
              cur_node.label, cn_edge.label, to_node.label);
            // A smaller code was found, so the given code is not minimal.
            if (dfs_code_forward_compare_t{}(dfs_code, min_dfs_code)) {
              return false;
            }
            if (dfs_code == min_dfs_code) {
              min_projection.emplace_back(&cn_edge, j);
            }
          }
        }
      }
      if (min_projection.size() > projection_end_index) {
        return true;
      }
    }
  }
  return true;
}

bool gbolt_instance_t::is_projection_min(const DfsCodes &dfs_codes) {
  size_t projection_start_index = 0;

  // Start at index 1, index 0 has already been validated.
  for (size_t i = 1; i < dfs_codes.size(); ++i) {
    const dfs_code_t& code_to_validate = *(dfs_codes[i]);
    const size_t projection_end_index = min_projection.size();

    // Note: If a forward and a backward edge can be both be used to extend
    // the code, then the backward edge must be smaller.
    if (code_to_validate.from > code_to_validate.to) {
      // Code is backwards, ensure it is minimal.
      if (!is_backward_min(dfs_codes, code_to_validate, projection_start_index)) {
        return false;
      }
      // Backward edge validated, does not affect the rightmost path.
    }
    else {
      // Code is forwards, so ensure no backwards codes exist,
      // then ensure the code is minimal.
      if (exists_backwards(projection_start_index) ||
          !is_forward_min(dfs_codes, code_to_validate, projection_start_index)) {
        return false;
      }
      // Forward edge was validated, so update the rightmost path.
      update_right_most_path(dfs_codes, i + 1);
    }

    projection_start_index = projection_end_index;
  }

  return true;
}

}  // namespace gbolt
