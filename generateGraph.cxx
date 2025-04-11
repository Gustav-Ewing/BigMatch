#include <algorithm>
#include <boost/pool/pool_alloc.hpp>
#include <boost/unordered_map.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <random>
#include <regex>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <utility>

#define PROSUMERS 1000000
#define CONSUMERS PROSUMERS * 5
#define SIZE PROSUMERS + (CONSUMERS)
#define EDGES_PER_CHUNK 10000000
#define SPARSEFACTOR                                                           \
  (3 * log(SIZE) / (SIZE)) // percent chance to not create make_edge
#define SEED 1234          // Current seed for the string
#define BETA                                                                   \
  2 // Beta value, determines the scaling of weights for a nodes edges
#define MAXWEIGHT 100 // Max allowed weight

using namespace std;
using Weight = uint32_t;

using Graph = std::unordered_map<std::pair<int, int>, Weight>;
// Custom hash function using uint64_t

struct pair_equal {
  bool operator()(const pair<int, int> &lhs, const pair<int, int> &rhs) const {
    return (lhs.first == rhs.first && lhs.second == rhs.second) ||
           (lhs.first == rhs.second && lhs.second == rhs.first);
  }
};

struct pair_hash {
  size_t operator()(const pair<int, int> &p) const {
    uint64_t a = static_cast<uint32_t>(min(p.first, p.second));
    uint64_t b = static_cast<uint32_t>(max(p.second, p.first));
    u_int64_t hash = (a << 32) | (b);
    // cout << "a = " << p.first << " and b = " << p.second << " and hash is "
    // << hash << "\n";
    return hash;
  }
};

static int remove_old_graphs() {
  namespace fs = std::filesystem;

  std::string directory = "./"; // Change to your target directory if needed
  std::regex pattern("^graph.*\\.txt$");

  try {
    for (const auto &entry : fs::directory_iterator(directory)) {
      if (fs::is_regular_file(entry.status())) {
        std::string filename = entry.path().filename().string();
        if (std::regex_match(filename, pattern)) {
          std::cout << "Removing: " << entry.path() << '\n';
          fs::remove(entry.path());
        }
      }
    }
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Filesystem error: " << e.what() << '\n';
  } catch (const std::regex_error &e) {
    std::cerr << "Regex error: " << e.what() << '\n';
  }

  return 0;
}

int insert_metadata(uint32_t num_of_edges) {
  cout << '\n';
  std::string filename = "graph0.txt";
  std::string temp_filename = "temp_graph0.txt";

  std::ifstream input_file(filename);
  std::ofstream temp_file(temp_filename);

  if (!input_file || !temp_file) {
    std::cerr << "Error opening file.\n";
    return 1;
  }

  // Write the new line first
  temp_file << PROSUMERS << " " << CONSUMERS << " " << num_of_edges << "\n";
  ;

  // Copy the rest of the file
  std::string line;
  while (std::getline(input_file, line)) {
    temp_file << line << '\n';
  }

  input_file.close();
  temp_file.close();

  // Replace the original file with the modified one
  std::filesystem::rename(temp_filename, filename);

  std::cout << "Line inserted at the top of " << filename << '\n';
  return 0;
}

int main() {
  remove_old_graphs();
  std::cout.imbue(std::locale("en_US.UTF-8")); // Use thousands separator

  mt19937 gen;
  mt19937 gen2;
  mt19937 gen3;
  gen.seed(SEED);
  gen2.seed(SEED);
  gen3.seed(SEED);
  std::uniform_real_distribution<> uniform_distrib(0, 1);
  std::poisson_distribution<uint32_t> weight_distrib((MAXWEIGHT) / 2);
  std::binomial_distribution<> degree_dist(CONSUMERS, SPARSEFACTOR);
  std::uniform_int_distribution<> consumer_dist(0, (CONSUMERS)-1);

  unordered_map<std::pair<int, int>, Weight, pair_hash, pair_equal> graph;
  int chunk = 0;
  uint32_t num_of_edges = 0;

  for (int i = 0; i < PROSUMERS; i++) {

    int degree = degree_dist(gen3);
    int producer_current_edges = 0;

    uint32_t weight_limit = MAXWEIGHT;

    while (producer_current_edges < degree) {

      int consumer = consumer_dist(gen3);
      if (graph.find({i, consumer}) != graph.end()) {
        continue;
      }

      weight_distrib.param(
          poisson_distribution<uint32_t>::param_type((weight_limit) / 2));
      uint32_t new_weight = weight_distrib(gen);
      while (new_weight > weight_limit || new_weight == 0) {
        new_weight = weight_distrib(gen);
      }
      weight_limit = std::min(new_weight * BETA, weight_limit);
      if (graph.find({i, consumer}) != graph.end()) {
        continue;
      }
      graph[{i, consumer}] = new_weight;
      producer_current_edges++;
    }

    if (graph.size() > EDGES_PER_CHUNK || i == PROSUMERS - 1) {
      string file = "graph" + to_string(chunk) + ".txt";

      ofstream stream; // To Write into a File, Use "ofstream"
      stream.open(file);
      for (const auto &[key, value] : graph) {
        stream << key.first << " " << key.second << " " << value << '\n';

        // Add '\n' character  ^^^^
      }
      stream.close();
      num_of_edges += graph.size();
      graph.clear();
      chunk++;
    }

    cout << "Prosumer: " << i + 1 << " out of " << PROSUMERS
         << " // Total edges: " << graph.size() + num_of_edges
         << " // Number of chunks: " << chunk << "\t\r" << flush;
  }

  if (graph.find({0, 1}) != graph.end()) {
    Weight weight = graph[{0, 1}];
    std::cout << '\n' << weight << '\n';
    cout << graph[{1, 0}] << '\n';
  }
  insert_metadata(num_of_edges);

  return 0;
}
