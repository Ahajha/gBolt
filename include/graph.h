#ifndef INCLUDE_GRAPH_H_
#define INCLUDE_GRAPH_H_

#include <common.h>
#include <vector>

namespace gbolt {

// Direct edge structure
struct edge_t {
  edge_t(int from, int label, int to, int id) :
    from(from), label(label), to(to), id(id) {}

  int from;
  int label;
  int to;
  int id;
};

// dfs projection links
struct min_prev_dfs_t {
  min_prev_dfs_t() : edge(NULL), prev(-1) {}

  min_prev_dfs_t(const edge_t *edge, int prev) :
    edge(edge), prev(prev) {}

  const edge_t *edge;
  int prev;
};
typedef std::vector<min_prev_dfs_t> MinProjection;

// dfs projection links
struct prev_dfs_t {
  prev_dfs_t() : id(0), edge(NULL), prev(NULL) {}

  prev_dfs_t(int id, const edge_t *edge, const prev_dfs_t *prev) :
    id(id), edge(edge), prev(prev) {}

  int id;
  const edge_t *edge;
  const prev_dfs_t *prev;
};
typedef vector<prev_dfs_t> Projection;

// dfs codes forward and backward compare
struct dfs_code_t {
  dfs_code_t() : from(0), to(0), from_label(0), edge_label(0), to_label(0) {}

  dfs_code_t(int from, int to, int from_label, int edge_label, int to_label) :
    from(from), to(to),
    from_label(from_label), edge_label(edge_label), to_label(to_label) {}

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
typedef vector<const dfs_code_t *> DfsCodes;

struct dfs_code_project_compare_t {
  bool operator() (const dfs_code_t &first, const dfs_code_t &second) {
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
  bool operator() (const dfs_code_t &first, const dfs_code_t &second) {
    if (first.to != second.to) {
      return first.to < second.to;
    } else {
      return first.edge_label < second.edge_label;
    }
  }
};

struct dfs_code_forward_compare_t {
  bool operator() (const dfs_code_t &first, const dfs_code_t &second) {
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
  vector<edge_t> edges;
};
typedef vector<vertex_t> Vertice;

class Graph {
 public:
  Graph() : id(0), nedges(0) {}

  int id;
  int nedges;
  Vertice vertice;
};

}  // namespace gbolt

#endif  // INCLUDE_GRAPH_H_
