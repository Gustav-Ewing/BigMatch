#include <boost/pool/pool_alloc.hpp>
#include <boost/unordered_map.hpp>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <ostream>
#include <random>
#include <sys/types.h>
#include <unordered_map>
#include <utility>

#define SIZE 30000

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

const static auto make_edge = [](int a, int b) {
  return std::make_pair(std::min(a, b), std::max(a, b));
};

int main() {
  std::cout.imbue(std::locale("en_US.UTF-8")); // Use thousands separator
  mt19937 gen;
  mt19937 gen2;
  gen.seed(1234);
  gen2.seed(1234);
  std::uniform_int_distribution<> distrib(1, 100);
  unordered_map<std::pair<int, int>, Weight, pair_hash, pair_equal> graph;
  // boost::unordered_map<
  //     std::pair<int, int>, Weight, pair_hash, pair_equal,
  //     boost::fast_pool_allocator<std::pair<const int, std::string>>>
  //     graph;
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      if (i == j || distrib(gen2) < 97) {
        continue;
      }

      graph[{i, j}] = gen();
    }
    cout << "Node " << i << " out of " << SIZE
         << " // Size of hash: " << graph.size() << "\t\r" << flush;
  }

  if (graph.find({0, 1}) != graph.end()) {
    Weight weight = graph[{0, 1}];
    std::cout << '\n' << weight << '\n';
    cout << graph[{1, 0}] << '\n';
  }
  string file = "graph.txt";
  ofstream stream; // To Write into a File, Use "ofstream"
  stream.open(file);
  for (const auto &[key, value] : graph) {
    stream << value << " " << key.first << " " << key.second << '\n';

    // Add '\n' character  ^^^^
  }
  stream.close();

  return 0;
}
