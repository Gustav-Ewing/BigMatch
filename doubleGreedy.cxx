
#include <codecvt>
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

// #define SIZE 20
using namespace std;

u_int32_t graphSize = 0;
u_int32_t nrProducers = 0;
u_int32_t nrConsumers = 0;
u_int32_t entries = 0;

using Weight = uint32_t;
using Pair = tuple<uint32_t, uint32_t, uint32_t>;
using Pairing = vector<vector<Pair>>;

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
using Graph = unordered_map<std::pair<u_int32_t, u_int32_t>, Weight, pair_hash,
                            pair_equal>;
using Neighborhood = unordered_map<u_int32_t, pair<u_int32_t, u_int32_t>>;
Neighborhood neighborhoods;

int test(Graph graph);

const static auto make_edge = [](int a, int b) {
  return std::make_pair(std::min(a, b), std::max(a, b));
};

Pairing doubleGreedy(Graph graph);

u_int32_t nextEdge(u_int32_t node, vector<Pair> path, bool typeNode,
                   bool consumer[], bool producer[]);

int main() {
  string filename = "graph.txt";
  ifstream graphFile(filename);
  string inputstr;

  Graph graph;
  getline(graphFile, inputstr);
  istringstream input;
  input.str(inputstr);

  string sRows, sColumns, sAmount;
  getline(input, sRows, ' ');
  getline(input, sColumns, ' ');
  getline(input, sAmount, ' ');

  nrProducers = stoul(sRows);
  nrConsumers = stoul(sColumns);
  entries = stoul(sAmount);
  graphSize = nrProducers + nrConsumers;

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

    // trying a adjacency list approach
    // first check if an enntry already exists
    // if it does extract it and this node to that neighborhood and readd it

    neighborhoods[stoul(node1)] = make_pair(stoul(node2), stoul(weight));
    neighborhoods[stoul(node2)] = make_pair(stoul(node1), stoul(weight));
  }
  // test(graph);

  Pairing result;
  result = doubleGreedy(graph);

  /*
  //testcode for the print below
  Pair test = make_tuple(1, 2, 10);
  result.clear();
  result.push_back(test);
  */

  for (u_int32_t i = 1; i < result.size(); i++) {
    int counter = 0;
    for (Pair element : result[i]) {
      cout << "Pair " << counter++ << " :" << "\n";
      cout << "\t" << "First Node: " << "\t" << get<0>(element) << "\n";
      cout << "\t" << "Second Node: " << "\t" << get<1>(element) << "\n";
      cout << "\t" << "Weight: " << "\t" << get<2>(element) << "\n";
    }
  }
  return 0;
}

Pairing doubleGreedy(Graph graph) {
  Pairing matching;

  // init and set all nodes as available
  bool consumers[nrConsumers];
  bool producers[nrProducers];
  for (u_int32_t i = 0; i < nrConsumers; i++) {
    consumers[i] = true;
  }
  for (u_int32_t i = 0; i < nrProducers; i++) {
    producers[i] = true;
  }

  for (u_int32_t i = 0; i < nrProducers; i++) {
    vector<Pair> path; // init path

    // find available node
    if (!producers[i]) {
      continue;
    }

    // path.push_back(i); // add to path

    // repeat for rest of path
    u_int32_t nextNode = i;
    u_int32_t originNode;
    bool typeNode = true;

    // fix this != 0 condition so it ends the loop properly (make sure no weight
    // is ever 0 except when there is none)
    while (nextNode != 0) {
      originNode = nextNode;
      nextNode = nextEdge(originNode, path, typeNode, consumers, producers);
      typeNode = !typeNode;
      u_int32_t weight = 1; // tmp value change to real value later
      Pair nextPair = make_tuple(originNode, nextNode, weight);
      path.push_back(nextPair);
    }
    matching.push_back(path);
  }

  return matching;
}

u_int32_t nextEdge(u_int32_t node, vector<Pair> path, bool typeNode,
                   bool consumers[], bool producers[]) {
  vector<pair<u_int32_t, u_int32_t>> neighbors;

  // true implies a producer
  if (typeNode) {
    neighbors = neighborhoods[node];
    for (u_int32_t i = 0; i < graphSize; i++) {
      if (consumers[i]) {
        neighbors.push_back(i);
      }
    }
  } else {
    for (u_int32_t i = 0; i < graphSize; i++) {
      if (producers[i]) {
        neighbors.push_back(i);
      }
    }
  }

  u_int32_t highest = 0;
  for (u_int32_t element : neighbors) {
    highest = max(element, highest);
  }

  // sketchy might work or might not work as intended
  if (highest != 0) {
    if (typeNode) {
      producers[node] = false;
      consumers[highest] = false;
    } else {
      consumers[node] = false;
      producers[highest] = false;
    }
  }

  return highest;
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
