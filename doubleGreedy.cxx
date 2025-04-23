#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// #define SIZE 20

u_int32_t graphSize = 0;
u_int32_t nrProducers = 0;
u_int32_t nrConsumers = 0;
u_int32_t entries = 0;

using Weight = uint32_t;
using Pair = std::tuple<uint32_t, uint32_t, uint32_t>;
using Pairing = std::vector<std::vector<Pair>>;

struct pair_equal {
  bool operator()(const std::pair<int, int> &lhs,
                  const std::pair<int, int> &rhs) const {
    return (lhs.first == rhs.first && lhs.second == rhs.second) ||
           (lhs.first == rhs.second && lhs.second == rhs.first);
  }
};

struct pair_hash {
  size_t operator()(const std::pair<int, int> &p) const {
    uint64_t a = static_cast<uint32_t>(std::min(p.first, p.second));
    uint64_t b = static_cast<uint32_t>(std::max(p.second, p.first));
    u_int64_t hash = (a << 32) | (b);
    // cout << "a = " << p.first << " and b = " << p.second << " and hash is "
    // << hash << "\n";
    return hash;
  }
};
using Edge = std::pair<u_int32_t, u_int32_t>;
using Graph = std::unordered_map<std::pair<u_int32_t, u_int32_t>, Weight,
                                 pair_hash, pair_equal>;
using Neighborhood = std::unordered_map<u_int32_t, std::vector<Edge>>;
Neighborhood producerNeighborhoods;
Neighborhood consumerNeighborhoods;

// namespace {
std::vector<Pair> greedy(Graph graph);
int test(Graph graph);
Pairing doubleGreedy(Graph graph);

Edge nextEdge(u_int32_t node, std::vector<Pair> path, bool typeNode,
              bool consumers[], bool producers[],
              std::unordered_set<u_int32_t> consumerInPath,
              std::unordered_set<u_int32_t> producerInPath);

// std::pair<u_int32_t, u_int32_t> nextEdge(u_int32_t node, std::vector<Pair>
// path,
//                                          bool typeNode, bool consumer[],
//                                          bool producer[]);
// } // namespace

const static auto make_edge = [](int a, int b) {
  return std::make_pair(std::min(a, b), std::max(a, b));
};

int readShard(u_int32_t shardNumber) {
  std::string filename = "graph" + std::to_string(shardNumber) + ".txt";
  std::ifstream graphFile(filename);
  std::string inputstr;

  // producerNeighborhoods.clear();
  // consumerNeighborhoods.clear();
  Graph graph;
  getline(graphFile, inputstr);
  // this line only contains metadata we should already know
  // therefore no need to process

  while (getline(graphFile, inputstr)) {
    // std::cout << inputstr << '\n';
    std::istringstream input;
    input.str(inputstr);
    if (inputstr.empty()) {
      continue;
    }

    std::string node1, node2, weight;
    getline(input, node1, ' ');
    getline(input, node2, ' ');
    getline(input, weight, ' ');
    // cout << node1 << "\t" << node2 << "\t" << weight << "\n";

    graph[{stoul(node1), stoul(node2)}] = stoul(weight);
    // this isnt used anywhere as far as I remember

    // trying a adjacency list approach
    // first check if an enntry already exists
    // if it does extract it and this node to that neighborhood and read it

    u_int32_t producer = 1 + stoul(node1);
    u_int32_t consumer = 1 + stoul(node2);
    u_int32_t edgeWeight = stoul(weight);

    std::vector<Edge> tmp;
    if (producerNeighborhoods.count(producer) != 0u) {
      tmp = producerNeighborhoods[producer];
    }
    tmp.emplace_back(consumer, edgeWeight);
    producerNeighborhoods[producer] = tmp;

    tmp.clear();
    if (consumerNeighborhoods.count(consumer) != 0u) {
      tmp = consumerNeighborhoods[consumer];
    }
    tmp.emplace_back(producer, edgeWeight);
    consumerNeighborhoods[consumer] = tmp;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  bool useDouble = false;
  if (argc > 1) {
    if (strcmp(argv[1], "double") == 0) {
      useDouble = true;
    }
  }

  std::string filename = "graph" + std::to_string(0) + ".txt";
  std::ifstream graphFile(filename);
  std::string inputstr;

  Graph graph;
  getline(graphFile, inputstr);
  std::istringstream input;
  input.str(inputstr);

  std::string sRows, sColumns, sAmount;
  getline(input, sRows, ' ');
  getline(input, sColumns, ' ');
  getline(input, sAmount, ' ');

  // could increment these by one inorder to not have to do weirdness in the
  // loops later on
  nrProducers = stoul(sRows);
  nrConsumers = stoul(sColumns);
  entries = stoul(sAmount);
  graphSize = nrProducers + nrConsumers;

  std::cout << "read metadata" << '\n';

  // test(graph);

  std::cout << "Matching" << '\n';
  Pairing result;
  std::vector<Pair> resultNormal;

  if (useDouble) {
    result = doubleGreedy(graph);
  } else {
    resultNormal = greedy(graph);
  }
  std::cout << "Finished Matching" << '\n';
  /*
  //testcode for the print below
  Pair test = make_tuple(1, 2, 10);
  result.clear();
  result.push_back(test);
  */

  if (!useDouble) {

    u_int64_t summer = 0;
    int counter = 0;
    for (Pair element : resultNormal) {
      if (false) {
        std::cout << "\t" << "Pair " << counter++ << " :" << "\n";
        std::cout << "\t\t" << "First Node: " << "\t" << std::get<0>(element)
                  << "\n";
        std::cout << "\t\t" << "Second Node: " << "\t" << std::get<1>(element)
                  << "\n";
        std::cout << "\t\t" << "Weight: " << "\t" << std::get<2>(element)
                  << "\n";
      }
      summer += std::get<2>(element); // the weight to running total of weights
    }
    std::cout << '\n' << "The number pairs is: " << resultNormal.size();
    std::cout << '\n' << "The total weight is: " << summer << '\n' << '\n';
  } else {
    u_int64_t summer = 0;
    for (u_int32_t i = 1; i < result.size(); i++) {
      std::cout << "Path " << i << " :" << "\n";
      int counter = 0;
      for (Pair element : result[i]) {
        std::cout << "\t" << "Pair " << counter++ << " :" << "\n";
        std::cout << "\t\t" << "First Node: " << "\t" << std::get<0>(element)
                  << "\n";
        std::cout << "\t\t" << "Second Node: " << "\t" << std::get<1>(element)
                  << "\n";
        std::cout << "\t\t" << "Weight: " << "\t" << std::get<2>(element)
                  << "\n";
        summer +=
            std::get<2>(element); // the weight to running total of weights
      }
    }
    std::cout << '\n' << "The total weight is: " << summer << '\n' << '\n';
  }
  return 0;
}

std::vector<Pair> greedy(Graph graph) {
  std::vector<Pair> matching;

  // this approach is ugly but havent seen any good options
  //  init and set all nodes as available
  bool consumers[nrConsumers];
  bool producers[nrProducers];
  for (u_int32_t i = 1; i < nrConsumers + 1; i++) {
    consumers[i] = true;
  }
  for (u_int32_t i = 1; i < nrProducers + 1; i++) {
    producers[i] = true;
  }

  // load Shard 0 before starting
  u_int32_t loadedShard = 0;
  std::cout << "loading shard: " << loadedShard << '\n';
  readShard(loadedShard);
  std::cout << "loaded shard: " << loadedShard << '\n';

  std::vector<std::pair<u_int32_t, u_int32_t>> neighbors;
  for (u_int32_t i = 1; i < nrProducers + 1; i++) {
    if (!producers[i]) {
      continue;
    }
    std::cout << "producer: " << i << "\t\r" << std::flush;
    if (producerNeighborhoods.count(i) == 0) {
      loadedShard++;
      readShard(loadedShard);
      std::cout << "\r\033[K";
      std::cout << "loaded shard: " << loadedShard << '\n';
    }
    neighbors = producerNeighborhoods[i];
    u_int32_t highestWeight = 0;
    u_int32_t highestIndex = 0;
    for (std::pair<u_int32_t, u_int32_t> neighbor : neighbors) {
      if (!consumers[neighbor.first]) {
        continue;
      }
      // std::cout << neighbor.second << '\n';
      if (neighbor.second > highestWeight) {
        highestIndex = neighbor.first;
        highestWeight = neighbor.second;
      }
    }
    if (highestIndex == 0) {
      continue;
    }
    matching.emplace_back(i, highestIndex, highestWeight);
    producers[i] = false;
    consumers[highestIndex] = false;
  }
  return matching;
}

Pairing doubleGreedy(Graph graph) {
  Pairing matching;

  readShard(0);

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
    std::vector<Pair> path; // init path

    // find available node
    if (!producers[i]) {
      continue;
    }

    // path.push_back(i); // add to path

    // repeat for rest of path
    u_int32_t nextNode = i;
    u_int32_t originNode;
    std::pair<u_int32_t, u_int32_t> edge;
    bool typeNode = true;

    std::unordered_set<u_int32_t> producerInPath;
    std::unordered_set<u_int32_t> consumerInPath;

    // fix this != 0 condition so it ends the loop properly (make sure no
    // weight is ever 0 except when there is none) should be fine now but need
    // to double check this also moved the check further down in the loop to
    // make sure it doesnt add nodes with 0
    while (true) {
      originNode = nextNode;
      edge = nextEdge(originNode, path, typeNode, consumers, producers,
                      consumerInPath, producerInPath);
      typeNode = !typeNode;
      nextNode = std::get<0>(edge);
      if (typeNode) {
        consumerInPath.insert(nextNode);
      } else {
        producerInPath.insert(nextNode);
      }

      u_int32_t weight = std::get<1>(edge);
      // u_int32_t weight = 1; // tmp value change to real value later
      Pair nextPair = std::make_tuple(originNode, nextNode, weight);
      if (nextNode == 0) {
        break;
      }
      // add only the edges that were found from a producer
      // this is not strictly correct but works for now
      // it will be slightly lower than what it should be for double
      // extra sketchy because typenode has now been fliped for the next
      // iteration so -> !typenode
      if (!typeNode) {
        path.push_back(nextPair);
      }
    }
    matching.push_back(path);
  }

  return matching;
}

Edge nextEdge(u_int32_t node, std::vector<Pair> path, bool typeNode,
              bool consumers[], bool producers[],
              std::unordered_set<u_int32_t> consumerInPath,
              std::unordered_set<u_int32_t> producerInPath) {
  std::vector<std::pair<u_int32_t, u_int32_t>> neighbors;

  // cout << "Edging" << '\n';
  //  check whether the neighbors are available
  if (typeNode) {
    for (std::pair<u_int32_t, u_int32_t> neighbor :
         producerNeighborhoods[node]) {
      if (consumerInPath.count(neighbor.first) > 0) {
        continue;
        // consumer already in path
      }
      if (consumers[std::get<0>(neighbor)]) {
        neighbors.push_back(neighbor);
      }
    }
  } else {
    for (std::pair<u_int32_t, u_int32_t> neighbor :
         consumerNeighborhoods[node]) {
      if (producerInPath.count(neighbor.first) > 0) {
        continue;
        // proucer already in path
      }
      if (producers[std::get<0>(neighbor)]) {
        neighbors.push_back(neighbor);
      }
    }
  }

  /*
  for (std::pair<u_int32_t, u_int32_t> neighbor : neighborhoods[node]) {
    if (typeNode) {
      if (consumers[std::get<0>(neighbor)]) {
        neighbors.push_back(neighbor);
      }
    } else {
      if (producers[std::get<0>(neighbor)]) {
        neighbors.push_back(neighbor);
      }
    }
  }
  */
  // cout << "Grabbed neighbors" << '\n';

  u_int32_t highestIndex = 0;
  u_int32_t highestWeight = 0;
  for (std::pair<u_int32_t, u_int32_t> neighbor : neighbors) {
    highestIndex = std::max(std::get<0>(neighbor), highestIndex);
    highestWeight = std::max(std::get<1>(neighbor), highestWeight);
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

  return std::make_pair(highestIndex, highestWeight);
}

// to test whether the graph was read in correctly
int test(Graph graph) {
  std::string file = "graphtest.txt";
  std::ofstream stream; // To Write into a File, Use "ofstream"
  stream.open(file);
  for (const auto &[key, value] : graph) {
    stream << key.first << " " << key.second << " " << value << '\n';
  }
  stream.close();

  return 0;
}
