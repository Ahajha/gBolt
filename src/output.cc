#include <output.h>
#include <fstream>

namespace gbolt {

void Output::push_back(const std::string &str, int nsupport, int graph_id) {
  buffer_.push_back(str);
  support_.push_back(nsupport);
  graph_id_.push_back(graph_id);
}

void Output::push_back(const std::string &str, int nsupport, int graph_id, int thread_id, int parent_id) {
  buffer_.push_back(str);
  support_.push_back(nsupport);
  graph_id_.push_back(graph_id);
  thread_id_.push_back(thread_id);
  parent_id_.push_back(parent_id);
}

void Output::save(bool output_parent, bool output_pattern) {
  std::ofstream out(output_file_);

  for (std::size_t i = 0; i < buffer_.size(); ++i) {
    out << "t # " << graph_id_[i] << " * " << support_[i] << '\n';
    if (output_parent) {
      if (parent_id_[i] == -1)
        out << "parent : -1\n";
      else
        out << "parent : " << parent_id_[i] << " thread : " << thread_id_[i] << '\n';
    }
    if (output_pattern) {
      out << buffer_[i] << '\n';
    }
  }
}

}  // namespace gbolt
