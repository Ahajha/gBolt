#ifndef INCLUDE_GBOLT_H_
#define INCLUDE_GBOLT_H_

#include <common.h>
#include <graph.h>
#include <history.h>
#include <output.h>
#include <map>
#include <vector>
#include <string>

namespace gbolt {

class Database;

struct gbolt_instance_t {
  Graph *min_graph = nullptr;
  DfsCodes *min_dfs_codes = nullptr;
  History *history = nullptr;
  Output *output = nullptr;
  std::vector<int> *right_most_path = nullptr;
  MinProjection *min_projection = nullptr;

  ~gbolt_instance_t() {
    delete this->min_graph;
    delete this->min_dfs_codes;
    delete this->history;
    delete this->output;
    delete this->right_most_path;
    delete this->min_projection;
  }
};

class GBolt {
 public:
  GBolt(const string &output_file, double support) :
    output_file_(output_file), support_(support),
    gbolt_instances_(nullptr) {}

  void read_input(const string &input_file, const string &separator);

  void execute();

  void save(bool output_parent = false, bool output_pattern = false, bool output_frequent_nodes = false);

  ~GBolt() {
    delete[] gbolt_instances_;
  }

 private:
  using ProjectionMap = map<dfs_code_t, Projection, dfs_code_project_compare_t>;
  using ProjectionMapBackward = map<dfs_code_t, Projection, dfs_code_backward_compare_t>;
  using ProjectionMapForward = map<dfs_code_t, Projection, dfs_code_forward_compare_t>;

 private:
  // Mine
  void init_instances();

  void project();

  void find_frequent_nodes_and_edges(const Database& db);

  void mine_subgraph(
    const Projection &projection,
    DfsCodes &dfs_codes,
    int prev_nsupport,
    int prev_thread_id,
    int prev_graph_id);

  void mine_child(
    const Projection &projection,
    const dfs_code_t* next_code,
    DfsCodes &dfs_codes,
    int prev_thread_id,
    int prev_graph_id);

  // Extend

  /*!
  Stores into right_most_path the rightmost path of the dfs code list. The
  path is stored such that the first item in right_most_path is the index of
  the edge 'discovering' the rightmost vertex, the second is the index of the
  edge discovering the 'from' vertex of the first edge, and so on.
  */
  void build_right_most_path(const DfsCodes &dfs_codes, std::vector<int> &right_most_path) {
    int prev_id = -1;

    // Go in reverse, since we need to first look for the edge that discovered
    // the rightmost vertex
    for (auto i = dfs_codes.size(); i > 0; --i) {
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

  void update_right_most_path(const DfsCodes &dfs_codes, std::vector<int> &right_most_path) {
    auto *last_dfs_code = dfs_codes.back();
    // filter out a simple case
    if (last_dfs_code->from > last_dfs_code->to) {
      return;
    }
    right_most_path.clear();
    build_right_most_path(dfs_codes, right_most_path);
  }

  void enumerate(
    const DfsCodes &dfs_codes,
    const Projection &projection,
    const std::vector<int> &right_most_path,
    ProjectionMapBackward &projection_map_backward,
    ProjectionMapForward &projection_map_forward);

  void get_first_forward(
    const prev_dfs_t &prev_dfs,
    const History &history,
    const Graph &graph,
    const DfsCodes &dfs_codes,
    const std::vector<int> &right_most_path,
    ProjectionMapForward &projection_map_forward);

  void get_other_forward(
    const prev_dfs_t &prev_dfs,
    const History &history,
    const Graph &graph,
    const DfsCodes &dfs_codes,
    const std::vector<int> &right_most_path,
    ProjectionMapForward &projection_map_forward);

  void get_backward(
    const prev_dfs_t &prev_dfs,
    const History &history,
    const Graph &graph,
    const DfsCodes &dfs_codes,
    const std::vector<int> &right_most_path,
    ProjectionMapBackward &projection_map_backward);

  // Count
  int count_support(const Projection &projection);

  void build_graph(const DfsCodes &dfs_codes, Graph &graph);

  bool is_min(const DfsCodes &dfs_codes);

  bool is_projection_min(
    const DfsCodes &dfs_codes,
    const Graph &min_graph,
    History &history,
    DfsCodes &min_dfs_codes,
    std::vector<int> &right_most_path,
    MinProjection &projection,
    size_t projection_start_index);

  bool judge_backward(
    const std::vector<int> &right_most_path,
    const Graph &min_graph,
    History &history,
    dfs_code_t &min_dfs_code,
    DfsCodes &min_dfs_codes,
    MinProjection &projection,
    size_t projection_start_index,
    size_t projection_end_index);

  bool judge_forward(
    const std::vector<int> &right_most_path,
    const Graph &min_graph,
    History &history,
    dfs_code_t &min_dfs_code,
    DfsCodes &min_dfs_codes,
    MinProjection &projection,
    size_t projection_start_index,
    size_t projection_end_index);

  // Report
  void report(const DfsCodes &dfs_codes, const Projection &projection,
    int nsupport, int prev_thread_id, int prev_graph_id);

 private:
  // Graphs after reconstructing
  vector<Graph> graphs_;
  // Single instance of minigraph
  #ifdef GBOLT_PERFORMANCE
  unordered_map<int, vector<int> > frequent_vertex_labels_;
  unordered_map<int, int> frequent_edge_labels_;
  #else
  map<int, vector<int> > frequent_vertex_labels_;
  map<int, int> frequent_edge_labels_;
  #endif
  string output_file_;
  double support_;
  int nsupport_;
  gbolt_instance_t *gbolt_instances_;
  dfs_code_project_compare_t dfs_code_project_compare_;
  dfs_code_forward_compare_t dfs_code_forward_compare_;
  dfs_code_backward_compare_t dfs_code_backward_compare_;
};

}  // namespace gbolt

#endif  // INCLUDE_GBOLT_H_
