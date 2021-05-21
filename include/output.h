#ifndef INCLUDE_OUTPUT_H_
#define INCLUDE_OUTPUT_H_

#include <vector>
#include <string>

namespace gbolt {

class Output {
 public:
  explicit Output(const std::string &output_file): output_file_(output_file) {}

  int size() const {
    return support_.size();
  }

  void push_back(const std::string &str, int nsupport, int graph_id);

  void push_back(const std::string &str, int nsupport, int graph_id, int thread_id, int parent_id);

  void save(bool output_parent = false, bool output_pattern = false);

 private:
  std::vector<std::string> buffer_;
  std::vector<int> support_;
  std::vector<int> thread_id_;
  std::vector<int> parent_id_;
  std::vector<int> graph_id_;
  const std::string output_file_;
};

}  // namespace gbolt

#endif  // INCLUDE_OUTPUT_H_
