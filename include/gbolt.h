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
    : history(max_edges, max_vertice), output(output_file_thread) {}

  //!@name Methods relating to determining if a DFS code sequence is minimal:
  ///@{

  /*!
  Returns true iff dfs_codes is a minimal DFS code sequence.
  Side effects:
  If is_min returns true, min_graph will be built using dfs_codes,
  and right_most_path will be updated.
  */
  bool is_min(const DfsCodes &dfs_codes);

  /*!
  Clears min_graph, then constructs a graph
  representation of dfs_codes into min_graph.
  */
  void build_min_graph(const DfsCodes &dfs_codes);

  /*!
  Returns true iff dfs_codes is a minimal DFS code sequence.
  Validates all codes except the first.
  */
  bool is_projection_min(const DfsCodes &dfs_codes);

  /*!
  Clears right_most_path, then stores into it the rightmost path of the dfs code
  list. The path is stored such that the first item in right_most_path is the
  index of the edge 'discovering' the rightmost vertex, the second is the index
  of the edge discovering the 'from' vertex of the first edge, and so on.
  Dfs_codes is treated as if it is truncated to the given size.
  */
  void update_right_most_path(const DfsCodes &dfs_codes, size_t size);

  /*!
  Returns true iff any projection can be extended with a backward edge.
  */
  bool exists_backwards(const size_t projection_start_index);

  /*!
  Returns true iff the given code is the minimum possible backward code
  that can be extended from any projection. If so, adds all possible
  projections into min_projection.
  */
  bool is_backward_min(
    const DfsCodes& dfs_codes,
    const dfs_code_t &min_dfs_code,
    const size_t projection_start_index);

  /*!
  Returns true iff the given code is the minimum possible forward code
  that can be extended from any projection. If so, adds all possible
  projections into min_projection.
  */
  bool is_forward_min(
    const DfsCodes& dfs_codes,
    const dfs_code_t &min_dfs_code,
    const size_t projection_start_index);

  ///@}
  //!@name Methods relating to output:
  ///@{

  /*!
  Reports a given DFS code sequence as frequent, outputs the graph
  and all projections of it.
  */
  void report(const DfsCodes &dfs_codes, const Projection &projection,
    int nsupport, int prev_thread_id, int prev_graph_id);

  ///@}
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
    const dfs_code_t& next_code,
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
};

}  // namespace gbolt

#endif  // INCLUDE_GBOLT_H_
