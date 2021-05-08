#include <history.h>
#include <graph.h>
#include <cstring>

namespace gbolt {

void History::build(const prev_dfs_t &start, const Graph &graph) {
  memset(has_edges_, false, sizeof(bool) * graph.nedges);
  memset(has_vertice_, false, sizeof(bool) * graph.vertice.size());
  edge_size_ = 0;

  for (auto *cur_dfs = &start; cur_dfs != nullptr; cur_dfs = cur_dfs->prev) {
    edges_[edge_size_++] = cur_dfs->edge;
    has_edges_[cur_dfs->edge->id] = true;
    has_vertice_[cur_dfs->edge->from] = true;
    has_vertice_[cur_dfs->edge->to] = true;
  }
}

void History::build_edges_min(const MinProjection &projection, const Graph &graph, int start) {
  memset(has_edges_, false, sizeof(bool) * graph.nedges);
  edge_size_ = 0;

  while (start != -1) {
    auto &cur_dfs = projection[start];
    edges_[edge_size_++] = cur_dfs.edge;
    has_edges_[cur_dfs.edge->id] = true;
    start = cur_dfs.prev;
  }
}

void History::build_vertice_min(const MinProjection &projection, const Graph &graph, int start) {
  memset(has_vertice_, false, sizeof(bool) * graph.vertice.size());
  edge_size_ = 0;

  while (start != -1) {
    auto &cur_dfs = projection[start];
    edges_[edge_size_++] = cur_dfs.edge;
    has_vertice_[cur_dfs.edge->from] = true;
    has_vertice_[cur_dfs.edge->to] = true;
    start = cur_dfs.prev; 
  }
}

}  // namespace gbolt
