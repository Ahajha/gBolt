#ifndef INCLUDE_DATABASE_H_
#define INCLUDE_DATABASE_H_

#include <common.h>
#include <vector>

namespace gbolt {

class Graph;

class Database {
 public:
  // Return graph count
  void read_input(const string &input_file, const string &separator);

  // Construct graph by frequent labels
  void construct_graphs(
  #ifdef GBOLT_PERFORMANCE
    const unordered_map<int, std::vector<int> > &frequent_vertex_labels,
    const unordered_map<int, int> &frequent_edge_labels,
  #else
    const map<int, std::vector<int> > &frequent_vertex_labels,
    const map<int, int> &frequent_edge_labels,
  #endif
    vector<Graph> &graphs);

  struct input_vertex {
    int id, label;
    input_vertex(int i, int la) : id(i), label(la) {}
  };
  struct input_edge {
    int from, to, label;
    input_edge(int f, int t, int la) : from(f), to(t), label(la) {}
  };
  struct input_graph {
    int id;
    std::vector<input_vertex> vertices;
    std::vector<input_edge> edges;
    input_graph(int i) : id(i) {}
  };
  const vector<input_graph>& get_graphs() const { return input_graphs_; }
 private:
  vector<input_graph> input_graphs_;
};

}  // namespace gbolt

#endif  // INCLUDE_DATABASE_H_
