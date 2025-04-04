
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
using Neighborhood =
    unordered_map<u_int32_t, vector<pair<u_int32_t, u_int32_t>>>;
Neighborhood neighborhoods;

int test(Graph graph);

const static auto make_edge = [](int a, int b) {
  return std::make_pair(std::min(a, b), std::max(a, b));
};

Pairing doubleGreedy(Graph graph);

pair<u_int32_t, u_int32_t> nextEdge(u_int32_t node, vector<Pair> path,
                                    bool typeNode, bool consumer[],
                                    bool producer[]);

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

  // cout << "read metadata" << '\n';

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
    // if it does extract it and this node to that neighborhood and read it

    u_int32_t producer = stoul(node1);
    u_int32_t consumer = stoul(node2);
    u_int32_t edgeWeight = stoul(weight);

    vector<pair<u_int32_t, u_int32_t>> tmp;
    if (neighborhoods.count(producer)) {
      tmp = neighborhoods[producer];
    }
    tmp.push_back(make_pair(consumer, edgeWeight));
    neighborhoods[producer] = tmp;

    tmp.clear();
    if (neighborhoods.count(consumer)) {
      tmp = neighborhoods[consumer];
    }
    tmp.push_back(make_pair(producer, edgeWeight));
    neighborhoods[consumer] = tmp;
  }
  // test(graph);
  // cout << "Matching" << '\n';
  Pairing result;
  result = doubleGreedy(graph);

  // cout << "Finished Matching" << '\n';
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
    pair<u_int32_t, u_int32_t> edge;
    bool typeNode = true;

    // fix this != 0 condition so it ends the loop properly (make sure no weight
    // is ever 0 except when there is none)
    while (nextNode != 0) {
      originNode = nextNode;
      edge = nextEdge(originNode, path, typeNode, consumers, producers);
      typeNode = !typeNode;
      nextNode = get<0>(edge);
      u_int32_t weight = get<1>(edge);
      // u_int32_t weight = 1; // tmp value change to real value later
      Pair nextPair = make_tuple(originNode, nextNode, weight);
      path.push_back(nextPair);
    }
    matching.push_back(path);
  }

  return matching;
}

pair<u_int32_t, u_int32_t> nextEdge(u_int32_t node, vector<Pair> path,
                                    bool typeNode, bool consumers[],
                                    bool producers[]) {
  vector<pair<u_int32_t, u_int32_t>> neighbors;

  // cout << "Edging" << '\n';
  //  check whether the neighbors are available
  for (pair<u_int32_t, u_int32_t> neighbor : neighborhoods[node]) {
    if (typeNode) {
      if (consumers[get<0>(neighbor)]) {
        neighbors.push_back(neighbor);
      }
    } else {
      if (producers[get<0>(neighbor)]) {
        neighbors.push_back(neighbor);
      }
    }
  }

  // cout << "Grabbed neighbors" << '\n';

  u_int32_t highestIndex = 0;
  u_int32_t highestWeight = 0;
  for (pair<u_int32_t, u_int32_t> neighbor : neighbors) {
    highestIndex = max(get<0>(neighbor), highestIndex);
    highestWeight = max(get<1>(neighbor), highestWeight);
  }

  // cout << "Finding best" << '\n';

  // sketchy might work or might not work as intended
  if (highestIndex != 0) {
    if (typeNode) {
      producers[node] = false;
      consumers[highestIndex] = false;
    } else {
      consumers[node] = false;
      producers[highestIndex] = false;
    }
  }

  // cout << "Updating availability" << '\n';

  return make_pair(highestIndex, highestWeight);
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
