
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

using Weight = uint32_t;
// using Graph = std::unordered_map<std::pair<int, int>, Weight>;
using Pairing = vector<tuple<uint32_t, uint32_t, uint32_t>>;

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

using Graph = unordered_map<std::pair<int, int>, Weight, pair_hash, pair_equal>;
Pairing doubleGreedy(Graph graph);

int main() {
  string filename = "graph.txt";
  ifstream graphFile(filename);
  string inputstr;

  unordered_map<std::pair<int, int>, Weight, pair_hash, pair_equal> graph;

  while (getline(graphFile, inputstr)) {
    istringstream input;
    input.str(inputstr);
    if (inputstr.empty()) {
      continue;
    }

    string node1, node2, weight;
    getline(input, node1, ' ');
    getline(input, node2, ' ');
    getline(input, weight, ' ');
    // cout << node1 << "\t" << node2 << "\t" << weight << "\n";

    graph[{stoul(node1), stoul(node2)}] = stoul(weight);
    // test()
  }

  Pairing result;
  result = doubleGreedy(graph);

  return 0;
}

Pairing doubleGreedy(Graph graph) {
  Pairing matching;
  const u_int32_t size = 10; // manually set for now
  bool producer[size];
  for (bool &entry : producer) {
    entry = true;
  }

  bool consumer[size];
  for (bool &entry : consumer) {
    entry = true;
  }

  for (u_int32_t i = 0; i < size; i++) {
    vector<u_int32_t> path; // init path

    // find available node
    if (producer[i] != true) {
      continue;
    }

    path.push_back(i); // add to path

    // todo repeat for rest of path
  }

  return matching;
}

// to test whether the graph was read in correctly
int test(Graph graph) {
  string file = "graphtest.txt";
  ofstream stream; // To Write into a File, Use "ofstream"
  stream.open(file);
  for (const auto &[key, value] : graph) {
    stream << key.first << " " << key.second << " " << value << '\n';
  }
  stream.close();
}
