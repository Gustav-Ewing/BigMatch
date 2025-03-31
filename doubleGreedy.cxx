
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

using Weight = uint32_t;
// using Graph = std::unordered_map<std::pair<int, int>, Weight>;

using Pair = tuple<uint32_t, uint32_t, uint32_t>;
using Pairing = vector<Pair>;

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

u_int32_t nextEdge(u_int32_t node, vector<u_int32_t> path, bool dist[]);

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

  /*
  //testcode for the print below
  Pair test = make_tuple(1, 2, 10);
  result.clear();
  result.push_back(test);
  */

  int counter = 0;
  for (Pair element : result) {
    cout << "Pair " << counter++ << " :" << "\n";
    cout << "\t" << "First Node: " << "\t" << get<0>(element) << "\n";
    cout << "\t" << "Second Node: " << "\t" << get<1>(element) << "\n";
    cout << "\t" << "Weight: " << "\t" << get<2>(element) << "\n";
  }

  return 0;
}

Pairing doubleGreedy(Graph graph) {
  Pairing matching;
  const u_int32_t size = 10; // manually set for now

  bool dist[size];
  int counter = 0;
  // true here implies producer
  for (bool &entry : producer) {
    if (counter++ % 3 == 0) {
      entry = true;
    }
  }

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
    if (!producer[i] || !dist[i]) {
      continue;
    }

    path.push_back(i); // add to path

    // repeat for rest of path
    u_int32_t nextNode = i;
    u_int32_t originNode;
    // fix this != 0 condition so it ends the loop properly
    while (nextNode != 0) {
      originNode = nextNode;
      nextNode = nextEdge(originNode, path, dist);
      path.push_back(nextNode);
    }
  }

  return matching;
}

u_int32_t nextEdge(u_int32_t node, vector<u_int32_t> path, bool dist[]) {
  bool producer = false;
  if (dist[node]) {
    producer = true;
  }
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

  return 0;
}
