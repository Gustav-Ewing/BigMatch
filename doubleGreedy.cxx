#include <cassert>
#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
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
std::vector<Pair> greedy();
int test(Graph graph);
Pairing doubleGreedy();

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

template <class Archive> void serialize(Archive &arx, Edge &edge) {
  arx(edge.first, edge.second); // Serialize both elements of the pair
}

class Neighborhood2 {
public:
  std::unordered_map<u_int32_t, std::vector<Edge>> hoods;
  template <class Archive> void serialize(Archive &arx) { arx(hoods); }
};

class ShardMapNew {
  static inline Neighborhood2 producerShard;
  // Neighborhood consumerShard;
  static inline std::unordered_set<u_int32_t> existingShards;
  static inline u_int32_t activeShard;
  static inline bool active = false;

public:
  ShardMapNew() = delete;

  static inline u_int32_t shardCount;

  static void addProducer(u_int32_t producer, u_int32_t consumer,
                          u_int32_t weight) {

    u_int32_t shardOfProducer = producer / ShardMapNew::shardCount;
    // std::cout << "shard: " << shardOfProducer << '\n';

    // if the shard is already in the set this returns false
    // otherwise it adds it to the set and returns true
    auto [unused, added] = existingShards.insert(shardOfProducer);

    // this means a new shard needs to be created
    if (added) {
      // if there is an active shard save it to the disk
      if (ShardMapNew::active) {
        ShardMapNew::saveShard();
      }
      // when the current shard is inactive make sure to set its number
      // and activate it
      ShardMapNew::active = true;
      ShardMapNew::activeShard = shardOfProducer;

      // std::cout << "Creating shard " << shardOfProducer << '\n';
      // build the new vector and add it to the map
      std::vector<Edge> tmpVector;
      tmpVector.emplace_back(consumer, weight);
      ShardMapNew::producerShard.hoods[producer] = tmpVector;

    }

    // shard already exists so just load it
    else {
      ShardMapNew::loadShard(shardOfProducer);
      std::vector<Edge> tmpVector = ShardMapNew::producerShard.hoods[producer];
      tmpVector.emplace_back(consumer, weight);
      ShardMapNew::producerShard.hoods[producer] = tmpVector;
      // std::cout << "producers in map: "
      // << ShardMapNew::producerShard.hoods.size() << '\n';
    }
    // std::cout << ShardMapNew::producerShard.size() << '\n';
  }

  // saves current shard to the disk and clears the map
  // make sure that you dont use this correctly
  // and dont overwrite a preexisting shard by mistake
  static void saveShard() {

    // std::cout << "Saving shard " << activeShard << '\n';

    // this is here so that you dont overwrite a shard by manually performing a
    // saveShard into a loadShard without realizing that loadShard performs a
    // saveShard implicitly
    if (!active) {
      return;
    }

    std::ofstream file("shards/map" + std::to_string(ShardMapNew::activeShard) +
                           ".bin",
                       std::ios::binary);
    cereal::BinaryOutputArchive archive(file);
    archive(ShardMapNew::producerShard);
    ShardMapNew::producerShard.hoods.clear();
    ShardMapNew::active = false;
  }

  // loads the shard
  // if the shard is already loaded nothing happens
  // if the shard is not loaded then the old shard is saved
  // and then the new one is loaded
  static void loadShard(u_int32_t shard) {

    // std::cout << "Fake Loading shard " << shard << '\n';
    // only early return when the existing shard is the correct one and
    // it is active
    if (shard == ShardMapNew::activeShard && ShardMapNew::active) {
      return;
    }
    ShardMapNew::saveShard();
    // std::cout << "Loading shard " << shard << '\n';
    std::ifstream file("shards/map" + std::to_string(shard) + ".bin",
                       std::ios::binary);
    cereal::BinaryInputArchive archive(file);
    archive(ShardMapNew::producerShard);
    ShardMapNew::active = true;
    ShardMapNew::activeShard = shard;
  }

  static std::vector<Edge> getProducerNeighborhood(u_int32_t producer) {
    // std::cout << producer << '\n';
    u_int32_t producersShard = producer / ShardMapNew::shardCount;
    ShardMapNew::loadShard(producersShard);

    // grab the specific hood
    auto kvpair = ShardMapNew::producerShard.hoods.find(producer);

    // check if it doesnt exist
    // in this case send a dummy value back for now
    if (kvpair == ShardMapNew::producerShard.hoods.end()) {
      std::cout << "Didnt find a neighborhood" << '\n';
      std::vector<Edge> dummy;
      dummy.emplace_back(0, 0);
      return dummy;
    }
    // otherwise just return it
    // note that first should be equal to producer here
    // otherwise we got the wrong neighborhood
    assert(kvpair->first == producer);
    return kvpair->second;
  }
};

static int remove_old_shards() {
  namespace fs = std::filesystem;
  fs::create_directory("shards");
  std::string directory =
      "./shards/"; // Change to your target directory if needed
  std::regex pattern("^shard.*\\.bin$");

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

void setUpMap(u_int32_t chunks) {
  remove_old_shards();
  // standard first shard naming maybe change this later?
  std::string filename = "graph0.txt";
  std::ifstream graphFile(filename);
  std::string inputstr;

  getline(graphFile, inputstr);

  std::istringstream input;
  input.str(inputstr);
  if (inputstr.empty()) {
    std::cout
        << "very bad!!! error when reading first line of first graph file";
    return;
  }

  std::cout << "metadata setup start" << '\n';
  std::string c, p, e;
  getline(input, c, ' ');
  getline(input, p, ' ');
  getline(input, e, ' ');

  nrConsumers = u_int32_t(std::stoul(c));
  nrProducers = u_int32_t(std::stoul(p));
  entries = u_int32_t(std::stoul(e));
  graphSize = nrProducers + nrConsumers; // is this still used though?

  std::cout << "metadata setup done" << '\n';

  for (u_int32_t i = 0; i < chunks; i++) {
    std::string filename = "graph" + std::to_string(i) + ".txt";
    std::ifstream graphFile(filename);
    std::string inputstr;

    // metadata only gets added to file 0 for now?
    // maybe we should concilidate into a metadata only file then?
    if (i == 0) {
      getline(graphFile, inputstr);
    }
    std::cout << "adding producers from chunk: " << i << '\n';
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

      // trying a adjacency list approach
      // first check if an enntry already exists
      // if it does extract it and this node to that neighborhood and read it

      u_int32_t producer = 1 + u_int32_t(stoul(node1));
      u_int32_t consumer = 1 + u_int32_t(stoul(node2));
      u_int32_t edgeWeight = u_int32_t(stoul(weight));

      // std::cout << producer << ' ' << consumer << ' ' << edgeWeight << '\n';
      ShardMapNew::addProducer(producer, consumer, edgeWeight);
      // std::cout << ShardMapNew::getProducerNeighborhood(producer).size()
      // << '\n';
      // std::cout << "added producers from shard: " << i << '\n';

      // preparing for a future function that adds consumer neighborhoods for
      // the double greedy algorithm
      // TODO shardMap.addConsumer(consumer, producer, weight);
    }
  }
}

int main(int argc, char *argv[]) {
  bool useDouble = false;
  if (argc > 1) {
    if (strcmp(argv[1], "double") == 0) {
      useDouble = true;
    }
  }

  ShardMapNew::shardCount = 500000;
  setUpMap(3897);
  // auto tmp = ShardMapNew::getProducerNeighborhood(10);
  // std::cout << tmp[0].first << ' ' << tmp[0].second << '\n';
  // return 0;

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
    result = doubleGreedy();
  } else {
    resultNormal = greedy();
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

int readShard(u_int32_t shardNumber) {
  std::string filename = "graph" + std::to_string(shardNumber) + ".txt";
  std::ifstream graphFile(filename);
  std::string inputstr;

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

    graph[{stoul(node1), stoul(node2)}] = u_int32_t(stoul(weight));
    // this isnt used anywhere as far as I remember

    // trying a adjacency list approach
    // first check if an enntry already exists
    // if it does extract it and this node to that neighborhood and read it

    u_int32_t producer = 1 + u_int32_t(stoul(node1));
    u_int32_t consumer = 1 + u_int32_t(stoul(node2));
    u_int32_t edgeWeight = u_int32_t(stoul(weight));

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
std::vector<Pair> greedy() {
  std::vector<Pair> matching;

  // this approach is ugly but havent seen any good options
  //  init and set all nodes as available
  std::pmr::unordered_set<u_int32_t> consumers;
  std::pmr::unordered_set<u_int32_t> producers;

  std::vector<std::pair<u_int32_t, u_int32_t>> neighbors;
  for (u_int32_t i = 1; i < nrProducers + 1; i++) {
    if (producers.find(i) != producers.end()) {
      continue;
    }
    neighbors = ShardMapNew::getProducerNeighborhood(i);
    u_int32_t highestWeight = 0;
    u_int32_t highestIndex = 0;
    for (std::pair<u_int32_t, u_int32_t> neighbor : neighbors) {
      if (consumers.find(neighbor.first) != consumers.end()) {
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
    producers.insert(i);
    consumers.insert(highestIndex);
  }
  return matching;
}

Pairing doubleGreedy() {
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
