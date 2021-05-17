#include <history.h>
#include <graph.h>
#include <algorithm>

namespace gbolt {

void History::build(const prev_dfs_t &start, const Graph &graph) {
  std::fill_n(has_edges_, graph.nedges, false);
  std::fill_n(has_vertice_, graph.vertice.size(), false);
  edge_size_ = 0;

  auto *cur_dfs = &start;
  do {
    edges_[edge_size_++] = &(cur_dfs->edge);
    has_edges_[cur_dfs->edge.id] = true;
    has_vertice_[cur_dfs->edge.from] = true;
    has_vertice_[cur_dfs->edge.to] = true;
  }
  while ((cur_dfs = cur_dfs->prev) != nullptr);
}

void History::build_edges_min(const MinProjection &projection, const Graph &graph, int start) {
  std::fill_n(has_edges_, graph.nedges, false);
  edge_size_ = 0;

  do {
    auto &cur_dfs = projection[start];
    edges_[edge_size_++] = &(cur_dfs.edge);
    has_edges_[cur_dfs.edge.id] = true;
    start = cur_dfs.prev;
  }
  while (start != -1);
}

void History::build_vertice_min(const MinProjection &projection, const Graph &graph, int start) {
  std::fill_n(has_vertice_, graph.vertice.size(), false);
  edge_size_ = 0;

  do {
    auto &cur_dfs = projection[start];
    edges_[edge_size_++] = &(cur_dfs.edge);
    has_vertice_[cur_dfs.edge.from] = true;
    has_vertice_[cur_dfs.edge.to] = true;
    start = cur_dfs.prev;
  }
  while (start != -1);
}

}  // namespace gbolt
