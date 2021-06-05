#include <history.h>
#include <graph.h>
#include <algorithm>

namespace gbolt {

static inline void toggle(bool& b) { b = !b; }

void History::build(const prev_dfs_t &start, const Graph &graph) {

  if (current == nullptr || cur_graph != &graph) {
    // No current encoding, encode from scratch
    std::fill_n(has_edges_, graph.nedges, false);
    std::fill_n(has_vertice_, graph.vertice.size(), false);
    edge_size_ = 0;

    auto *cur_dfs = &start;
    do {
      edges_[edge_size_++] = &(cur_dfs->edge);
      has_edges_[cur_dfs->edge.id] = true;
      ++has_vertice_[cur_dfs->edge.from];
      ++has_vertice_[cur_dfs->edge.to];
    }
    while ((cur_dfs = cur_dfs->prev) != nullptr);

    cur_graph = &graph;
  }
  else {
    // Has an encoded projection, assume it is the same size and distinct.
    // (allows the do-while)
    // Reuse as much of this as possible.

    auto *new_dfs = &start;
    auto *old_dfs = current;

    int modify_index = 0;

    do {
      edges_[modify_index++] = &(new_dfs->edge);

      // Remove old edge
      toggle(has_edges_[old_dfs->edge.id]);
      --has_vertice_[old_dfs->edge.from];
      --has_vertice_[old_dfs->edge.to];

      // Add new edge
      toggle(has_edges_[new_dfs->edge.id]);
      ++has_vertice_[new_dfs->edge.from];
      ++has_vertice_[new_dfs->edge.to];
    }
    // As the code lengths are the same, this will also catch the case
    // where new_dfs and old_dfs end up as nullptr at the same time.
    while((new_dfs = new_dfs->prev) != (old_dfs = old_dfs->prev));
  }
  current = &start;
}

void History::build_edges_min(const MinProjection &projection, const Graph &graph, int start) {
  std::fill_n(has_edges_, graph.nedges, false);
  edge_size_ = 0;

  do {
    auto &cur_dfs = projection[start];
    edges_[edge_size_++] = &(cur_dfs.edge);
    has_edges_[cur_dfs.edge.id] = 1;
    start = cur_dfs.prev;
  }
  while (start != -1);
}

void History::build_vertice_min(const MinProjection &projection, const Graph &graph, int start) {
  std::fill_n(has_vertice_, graph.vertice.size(), 0);
  edge_size_ = 0;

  do {
    auto &cur_dfs = projection[start];
    edges_[edge_size_++] = &(cur_dfs.edge);
    has_vertice_[cur_dfs.edge.from] = 1;
    has_vertice_[cur_dfs.edge.to] = 1;
    start = cur_dfs.prev;
  }
  while (start != -1);
}

}  // namespace gbolt
