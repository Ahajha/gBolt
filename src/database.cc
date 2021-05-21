#include <database.h>
#include <graph.h>
#include <common.h>
#include <fstream>
#include <cstdlib>
#include <cstring>

namespace gbolt {

void Database::read_input(const std::string &input_file, const std::string &separator) {
  std::ifstream fin(input_file);

  if (!fin.is_open()) {
    LOG_ERROR("Open file error! %s", input_file.c_str());
  }

  char line[FILE_MAX_LINE];

  while (fin.getline(line, FILE_MAX_LINE)) {
    char *pch = strtok(line, separator.c_str());

    // Empty line, just skip
    if (!pch) continue;

    // The original implementation did not do bounds checking, so it seems that
    // having an incomplete line is UB. This may be fixed in the future.

    // First character indicates the type of data
    switch(*pch) {
      case 't': {
        // Discard the '#' character
        strtok(nullptr, separator.c_str());
        int id = atoi(strtok(nullptr, separator.c_str()));
        input_graphs_.emplace_back(id);
        break;
      }
      case 'v': {
        int id = atoi(strtok(nullptr, separator.c_str()));
        int label = atoi(strtok(nullptr, separator.c_str()));
        input_graphs_.back().vertices.emplace_back(id, label);
        break;
      }
      case 'e': {
        int from = atoi(strtok(nullptr, separator.c_str()));
        int to = atoi(strtok(nullptr, separator.c_str()));
        int label = atoi(strtok(nullptr, separator.c_str()));
        input_graphs_.back().edges.emplace_back(from, to, label);
        break;
      }
      default:
        LOG_ERROR("Reading input error!");
    }
  }
}

// Construct graph by labels
void Database::construct_graphs(
  const std::unordered_map<int, std::vector<int> > &frequent_vertex_labels,
  const std::unordered_map<int, int> &frequent_edge_labels,
  std::vector<Graph> &graphs) {

  graphs.reserve(input_graphs_.size());

  std::unordered_map<int, int> id_map;

  for (const auto& input_graph : input_graphs_) {
    graphs.emplace_back();

    auto& vertice = graphs.back().vertice;
    vertice.reserve(input_graph.vertices.size());

    int vertex_id = 0;
    for (const auto& vert : input_graph.vertices) {
      if (frequent_vertex_labels.find(vert.label)
        != frequent_vertex_labels.end()) {
        id_map[vert.id] = vertex_id;
        vertice.emplace_back(vertex_id++, vert.label);
      }
    }

    int edge_id = 0;
    for (const auto& edge : input_graph.edges) {
      const int label_from = vertice[edge.from].label;
      const int label_to = vertice[edge.to].label;
      if (frequent_vertex_labels.find(label_from) != frequent_vertex_labels.end() &&
        frequent_vertex_labels.find(label_to) != frequent_vertex_labels.end() &&
        frequent_edge_labels.find(edge.label) != frequent_edge_labels.end()) {
        const int to = id_map[edge.to];
        const int from = id_map[edge.from];
        vertice[from].edges.emplace_back(from, to, edge.label, edge_id);
        vertice[to  ].edges.emplace_back(to, from, edge.label, edge_id);
        ++edge_id;
      }
    }

    graphs.back().id = input_graph.id;
    graphs.back().nedges = edge_id;

    id_map.clear();
  }
}

}  // namespace gbolt
