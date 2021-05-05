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
  Graph min_graph;
  History history;
  Output output;
  std::vector<int> right_most_path;
  MinProjection min_projection;

  gbolt_instance_t(int max_edges, int max_vertice, const string& output_file_thread)
    : history(max_edges, max_vertice), output(output_file_thread) {
    right_most_path.reserve(DEFAULT_PATH_LEN);
    min_projection.reserve(DEFAULT_PATH_LEN);
  }

  // Extend

  /*!
  Clears right_most_path, then stores into it the rightmost path of the dfs code
  list. The path is stored such that the first item in right_most_path is the
  index of the edge 'discovering' the rightmost vertex, the second is the index
  of the edge discovering the 'from' vertex of the first edge, and so on.
  */
  void update_right_most_path(const DfsCodes &dfs_codes) {
    update_right_most_path(dfs_codes, dfs_codes.size());
  }

  /*!
  Same behavior as update_right_most_path(dfs_codes), except
  behaves as if dfs_codes is truncated to the given size.
  */
  void update_right_most_path(const DfsCodes &dfs_codes, size_t size) {
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

  // Count

  /*!
  Returns true iff dfs_codes is a minimal DFS code sequence.
  Side effects:
  If is_min returns true, min_graph will be built using dfs_codes.
  */
  bool is_min(const DfsCodes &dfs_codes);

  bool is_projection_min(const DfsCodes &dfs_codes);

  bool judge_backward(
    const DfsCodes& dfs_codes,
    dfs_code_t &min_dfs_code,
    size_t projection_start_index,
    size_t projection_end_index);

  bool judge_forward(
    const DfsCodes& dfs_codes,
    dfs_code_t &min_dfs_code,
    size_t projection_start_index,
    size_t projection_end_index);

  // This isn't the best place to put this, but this is temporary.
  static void build_graph(const DfsCodes &dfs_codes, Graph& graph);

  // Report
  void report(const DfsCodes &dfs_codes, const Projection &projection,
    int nsupport, int prev_thread_id, int prev_graph_id);
};

class GBolt {
 public:
  GBolt(const string &output_file, double support) :
    output_file_(output_file), support_(support) {}

  void read_input(const string &input_file, const string &separator);

  void execute();

  void save(bool output_parent = false, bool output_pattern = false, bool output_frequent_nodes = false);

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

  #ifdef GBOLT_SERIAL
  constexpr static int thread_id() { return 0; }
  #else
  static int thread_id() { return omp_get_thread_num(); }
  #endif

  gbolt_instance_t& thread_instance() {
    return gbolt_instances_[thread_id()];
  }

  // Extend

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
  vector<gbolt_instance_t> gbolt_instances_;
  dfs_code_project_compare_t dfs_code_project_compare_;
  dfs_code_forward_compare_t dfs_code_forward_compare_;
  dfs_code_backward_compare_t dfs_code_backward_compare_;
};

}  // namespace gbolt

#endif  // INCLUDE_GBOLT_H_
