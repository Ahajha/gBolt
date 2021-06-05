#ifndef INCLUDE_HISTORY_H_
#define INCLUDE_HISTORY_H_

#include <graph.h>

namespace gbolt {

class History {
 public:
  History(int max_edges, int max_vertice) : edge_size_(0) {
    edges_ = new ConstEdgePointer[max_edges];
    has_edges_ = new bool[max_edges];
    has_vertice_ = new int[max_vertice];
  }

  /*!
  Clears the contents of this History, and refills the arrays
  using start.
  */
  void build(const prev_dfs_t &start, const Graph &graph);

  void build_edges_min(const MinProjection &projection, const Graph &graph, int start);

  void build_vertice_min(const MinProjection &projection, const Graph &graph, int start);

  bool has_edges(int index) const {
    return has_edges_[index];
  }

  bool has_vertice(int index) const {
    return has_vertice_[index] != 0;
  }

  const edge_t& get_edge(int index) const {
    return *(edges_[edge_size_ - index - 1]);
  }

  void clear() { current = nullptr; }

  ~History() {
    delete[] edges_;
    delete[] has_edges_;
    delete[] has_vertice_;
  }

 private:
  using ConstEdgePointer = const edge_t *;

  // edges_ and edge_size_ effectively make up a vector. The claim
  // here seems to be that doing it this way saves memory in comparison
  // to push_back, though I have no idea how this is the case.
  ConstEdgePointer *edges_;

  // has_edges_ and has_vertice_ at a given index is true if this history has been
  // built with an object that has that given edge/vertex ID.
  bool *has_edges_;
  int *has_vertice_;
  int edge_size_;

  const prev_dfs_t* current = nullptr;
  const Graph* cur_graph = nullptr;
};

}  // namespace gbolt

#endif  // INCLUDE_HISTORY_H_
