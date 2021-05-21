#ifndef INCLUDE_GRAPH_H_
#define INCLUDE_GRAPH_H_

#include <vector>

namespace gbolt {

// Direct edge structure
struct edge_t {
  edge_t(int from, int to, int label, int id) :
    from(from), to(to), label(label), id(id) {}

  int from;
  int to;
  int label;
  int id;
};

// dfs projection links
struct min_prev_dfs_t {
  min_prev_dfs_t(const edge_t& edge, int prev) :
    edge(edge), prev(prev) {}

  const edge_t& edge;
  int prev;
};
using MinProjection = std::vector<min_prev_dfs_t>;

/*!
A prev_dfs_t represents an instance of a subgraph being found within
an input graph.
*/
struct prev_dfs_t {
  prev_dfs_t(int id, const edge_t& edge, const prev_dfs_t *prev) :
    id(id), edge(edge), prev(prev) {}

  //! The ID of the graph the subgraph was found in.
  int id;

  //! A reference to the last edge in the DFS code representation of the subgraph.
  const edge_t& edge;

  //! A pointer to the subgraph instance without the last edge.
  const prev_dfs_t *prev;
};

/*!
A Projection represents a support list. Subgraphs are grouped by graph ID, such that
if two prev_dfs_ts have the same ID, they are either adjacent or separated by other
prev_dfs_ts with the same ID.
*/
using Projection = std::vector<prev_dfs_t>;

// dfs codes forward and backward compare
struct dfs_code_t {
  bool operator != (const dfs_code_t &t) const {
    return (from != t.from) || (to != t.to) ||
      (from_label != t.from_label) || (edge_label != t.edge_label) ||
      (to_label != t.to_label);
  }

  bool operator == (const dfs_code_t &t) const {
    return (from == t.from) && (to == t.to) &&
      (from_label == t.from_label) && (edge_label == t.edge_label) &&
      (to_label == t.to_label);
  }

  int from;
  int to;
  int from_label;
  int edge_label;
  int to_label;
};
using DfsCodes = std::vector<const dfs_code_t *>;

struct dfs_code_project_compare_t {
  bool operator() (const dfs_code_t &first, const dfs_code_t &second) const {
    if (first.from_label != second.from_label) {
      return first.from_label < second.from_label;
    } else {
      if (first.edge_label != second.edge_label) {
        return first.edge_label < second.edge_label;
      } else {
        return first.to_label < second.to_label;
      }
    }
  }
};

struct dfs_code_backward_compare_t {
  bool operator() (const dfs_code_t &first, const dfs_code_t &second) const {
    if (first.to != second.to) {
      return first.to < second.to;
    } else {
      return first.edge_label < second.edge_label;
    }
  }
};

struct dfs_code_forward_compare_t {
  bool operator() (const dfs_code_t &first, const dfs_code_t &second) const {
    if (first.from != second.from) {
      return first.from > second.from;
    } else {
      if (first.edge_label != second.edge_label) {
        return first.edge_label < second.edge_label;
      } else {
        return first.to_label < second.to_label;
      }
    }
  }
};

struct vertex_t {
  vertex_t() : id(0), label(0) {}
  vertex_t(int id, int label) : id(id), label(label) {}

  int id;
  int label;
  std::vector<edge_t> edges;
};
using Vertice = std::vector<vertex_t>;

class Graph {
 public:
  Graph() : id(0), nedges(0) {}

  int id;
  int nedges;
  Vertice vertice;
};

}  // namespace gbolt

#endif  // INCLUDE_GRAPH_H_
