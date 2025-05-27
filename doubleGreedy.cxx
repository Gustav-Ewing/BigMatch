#include <cassert>
#include <cctype>
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
#include <limits>
#include <list>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

u_int32_t graphSize = 0;
u_int32_t nrProducers = 0;
u_int32_t nrConsumers = 0;
u_int32_t entries = 0;

using Weight = uint32_t;
using Pair = std::tuple<uint32_t, uint32_t, uint32_t>;
using Pairing = std::vector<std::vector<Pair>>;

using Edge = std::pair<u_int32_t, u_int32_t>;
// using Neighborhood = std::unordered_map<u_int32_t, std::vector<Edge>>;

// namespace {
std::vector<Pair> greedy();
Pairing doubleGreedy();

Edge nextEdge(u_int32_t node, bool typeNode,
              const std::unordered_set<u_int32_t> &consumers,
              const std::unordered_set<u_int32_t> &producers);
// const std::unordered_set<u_int32_t> &consumersPath,
// const std::unordered_set<u_int32_t> &producersPath);

// std::pair<u_int32_t, u_int32_t> nextEdge(u_int32_t node, std::vector<Pair>
// path,
//                                          bool typeNode, bool consumer[],
//                                          bool producer[]);
// } // namespace

// appearently shouldnt be needed
template <class Archive> void serialize(Archive &arx, Edge &edge) {
  arx(edge.first, edge.second); // Serialize both elements of the pair
}

class Neighborhood {
public:
  std::unordered_map<u_int32_t, std::vector<Edge>> hoods;
  template <class Archive> void serialize(Archive &arx) { arx(hoods); }
};

using Shard = Neighborhood;

class Manager {
public:
  static std::vector<Edge> getProducerNeighborhood(u_int32_t producer) {
    // std::cout << producer << '\n';
    u_int32_t producersShardNr = producer / shardSizeProducer;

    Shard &producerShard = getProducerShard(producersShardNr);

    // grab the specific hood
    auto kvpair = producerShard.hoods.find(producer);

    // check if it doesnt exist
    // in this case send a dummy value back for now
    if (kvpair == producerShard.hoods.end()) {
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

  static std::vector<Edge> getConsumerNeighborhood(u_int32_t consumer) {
    u_int32_t consumersShardNr = consumer / shardSizeConsumer;

    Shard &consumerShard = getConsumerShard(consumersShardNr);

    // grab the specific hood
    auto kvpair = consumerShard.hoods.find(consumer);

    // check if it doesnt exist
    // in this case send a dummy value back for now
    if (kvpair == consumerShard.hoods.end()) {
      std::cout << "Didnt find a neighborhood" << '\n';
      std::vector<Edge> dummy;
      dummy.emplace_back(0, 0);
      return dummy;
    }

    // otherwise just return it
    // note that first should be equal to consumer here
    // otherwise we got the wrong neighborhood
    assert(kvpair->first == consumer);
    return kvpair->second;
  }

  // adds the specified edge to the producers neighborhood
  static void addProducer(u_int32_t producer, u_int32_t consumer,
                          u_int32_t weight) {
    u_int32_t producersShardNr = producer / shardSizeProducer;
    Shard &producerShard = getProducerShard(producersShardNr);

    producerShard.hoods[producer].emplace_back(consumer, weight);
  }

  // adds the specified edge to the consumers neighborhood
  static void addConsumer(u_int32_t consumer, u_int32_t producer,
                          u_int32_t weight) {
    u_int32_t consumersShardNr = consumer / shardSizeConsumer;
    Shard &consumerShard = getConsumerShard(consumersShardNr);

    // std::cout << "Before: " << producerShard.hoods.size() << '\n';
    consumerShard.hoods[consumer].emplace_back(producer, weight);
    // std::cout << "After: " << producerShard.hoods.size() << '\n';
  }
  static inline u_int32_t maxCacheSizeProducer =
      8000; // this value is in shards
  static inline u_int32_t shardSizeProducer =
      8000; // while this value is in producers
  static inline u_int32_t maxCacheSizeConsumer =
      8000; // this value is in shards
  static inline u_int32_t shardSizeConsumer =
      8000; // while this value is in consumers
private:
  using ListIt = std::list<u_int32_t>::iterator;

  struct CacheEntry {
    Shard shard;
    ListIt lruIt;
  };

  static Shard &getProducerShard(u_int32_t shardId) {
    auto mapIt = cacheProducer.find(shardId);
    // Shard is loaded currently
    // Move to back of LRU list
    // then early return it
    if (mapIt != cacheProducer.end()) {
      lruProducer.erase(mapIt->second.lruIt);
      lruProducer.push_back(shardId);
      mapIt->second.lruIt = std::prev(lruProducer.end());
      return mapIt->second.shard;
    }

    // Cache miss so load and evict if needed
    if (cacheProducer.size() >= maxCacheSizeProducer) {
      evictLRUProducer();
    }
    Shard shard;

    // does the requested shard exist already?
    auto setIt = existingShardsProducer.find(shardId);
    if (setIt != existingShardsProducer.end()) {
      // if the shard does exist load it
      shard = loadShard(shardId, true);
    } else {
      // otherwise mark it as existing and send a new shard back
      existingShardsProducer.insert(shardId);
    }

    lruProducer.push_back(shardId);
    cacheProducer[shardId] = {shard, std::prev(lruProducer.end())};
    return cacheProducer[shardId].shard;
  }

  static Shard &getConsumerShard(u_int32_t shardId) {
    auto mapIt = cacheConsumer.find(shardId);
    // Shard is loaded currently
    // Move to back of LRU list
    // then early return it
    if (mapIt != cacheConsumer.end()) {
      lruConsumer.erase(mapIt->second.lruIt);
      lruConsumer.push_back(shardId);
      mapIt->second.lruIt = std::prev(lruConsumer.end());
      return mapIt->second.shard;
    }

    // Cache miss so load and evict if needed
    if (cacheConsumer.size() >= maxCacheSizeConsumer) {
      evictLRUConsumer();
    }
    Shard shard;

    // does the requested shard exist already?
    auto setIt = existingShardsConsumer.find(shardId);
    if (setIt != existingShardsConsumer.end()) {
      // if the shard does exist load it
      shard = loadShard(shardId, false);
    } else {
      // otherwise mark it as existing and send a new shard back
      existingShardsConsumer.insert(shardId);
    }

    lruConsumer.push_back(shardId);
    cacheConsumer[shardId] = {shard, std::prev(lruConsumer.end())};
    return cacheConsumer[shardId].shard;
  }

  static void evictLRUProducer() {
    u_int32_t evictedShard = lruProducer.front();
    lruProducer.pop_front();
    saveProducerShard(evictedShard);
    cacheProducer.erase(evictedShard);
  }

  static void evictLRUConsumer() {
    u_int32_t evictedShard = lruConsumer.front();
    lruConsumer.pop_front();
    saveConsumerShard(evictedShard);
    cacheConsumer.erase(evictedShard);
  }

  // saves a producer shard to the disk and clears it from the map
  static void saveProducerShard(u_int32_t shardToSave) {

    std::ofstream file("shards/producerShard" + std::to_string(shardToSave) +
                           ".bin",
                       std::ios::binary);
    cereal::BinaryOutputArchive archive(file);
    archive(cacheProducer[shardToSave].shard);
    cacheProducer.erase(shardToSave);
  }

  // saves a consumer shard to the disk and clears it from the map
  static void saveConsumerShard(u_int32_t shardToSave) {

    std::ofstream file("shards/consumerShard" + std::to_string(shardToSave) +
                           ".bin",
                       std::ios::binary);
    cereal::BinaryOutputArchive archive(file);
    archive(cacheConsumer[shardToSave].shard);
    cacheConsumer.erase(shardToSave);
  }

  // loads the requested shard and returns it
  static Shard loadShard(u_int32_t shard, bool isProducer) {
    std::cout << "Loading shard " << shard << '\n';

    // Determines if a producer or consumer shard is loaded
    std::string middle;
    if (isProducer) {
      middle = "producer";
    } else {
      middle = "consumer";
    }

    std::ifstream file("shards/" + middle + "Shard" + std::to_string(shard) +
                           ".bin",
                       std::ios::binary);
    cereal::BinaryInputArchive archive(file);
    Shard tmp;
    archive(tmp);
    return tmp;
  }

  static inline std::unordered_map<u_int32_t, CacheEntry> cacheConsumer;
  static inline std::unordered_map<u_int32_t, CacheEntry> cacheProducer;
  static inline std::list<u_int32_t> lruConsumer;
  static inline std::list<u_int32_t> lruProducer;
  static inline std::unordered_set<u_int32_t> existingShardsConsumer;
  static inline std::unordered_set<u_int32_t> existingShardsProducer;

  Manager() = default;
  Manager(const Manager &) = delete;
  Manager &operator=(const Manager &) = delete;
};

static void remove_old_shards() {
  namespace fs = std::filesystem;
  std::string directory = "./shards/";

  // Create the directory if it doesn't exist
  fs::create_directory(directory);

  // Iterate and remove all files in the directory
  for (const auto &entry : fs::directory_iterator(directory)) {
    fs::remove(entry);
  }
  std::cout << "removed old shards" << '\n';
}

void setUpMap(bool useDouble) {
  remove_old_shards();
  // standard first shard naming maybe change this later?
  std::string filename = "graphs/graph0.txt";
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
  std::string p, c, e;
  getline(input, p, ' ');
  getline(input, c, ' ');
  getline(input, e, ' ');

  // note above is the correct order of the entries
  // did the wrong order in an earlier version
  nrProducers = u_int32_t(std::stoul(p));
  nrConsumers = u_int32_t(std::stoul(c));
  entries = u_int32_t(std::stoul(e));
  graphSize = nrProducers + nrConsumers; // is this still used though?

  std::cout << "metadata setup done" << '\n';

  // In a way this is just a while loop maybe I should make it one
  for (u_int32_t i = 0; i < std::numeric_limits<u_int32_t>::max(); i++) {
    std::string filename = "graphs/graph" + std::to_string(i) + ".txt";
    std::ifstream graphFile(filename);
    std::string inputstr;

    // if the file wasn't opened it is almost always because there are no more
    // chunks so the task is done and we should return
    if (!graphFile.is_open()) {
      return;
    }
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
      // first check if an entry already exists
      // if it does extract it and this node to that neighborhood and read it

      u_int32_t producer = 1 + u_int32_t(stoul(node1));
      u_int32_t consumer = 1 + u_int32_t(stoul(node2));
      u_int32_t edgeWeight = u_int32_t(stoul(weight));

      // std::cout << producer << ' ' << consumer << ' ' << edgeWeight <<
      // '\n'; ShardMapNew::addProducer(producer, consumer, edgeWeight);
      Manager::addProducer(producer, consumer, edgeWeight);
      if (useDouble) {
        Manager::addConsumer(consumer, producer, edgeWeight);
      }
      // std::cout << ShardMapNew::getProducerNeighborhood(producer).size()
      // << '\n';
      // std::cout << "added producers from shard: " << i << '\n';
    }
  }
}

int main(int argc, char *argv[]) {
  std::cout.imbue(std::locale("en_US.UTF-8")); // Use thousands separator
  bool useDouble = true;

  // Clang tidy hates this block it seems
  // All it does is select which algorithm to use based on user input
  // and then select paramaters based on user input as well
  if (argc > 1) {
    // Kinda unnecesary check but this avoid crashes on inputs like "norma"
    // If you mix numbers and letters that's your issue for now
    if (std::isalpha(argv[1][0])) {
      if (strcmp(argv[1], "normal") == 0) {
        useDouble = false;
        if (argc > 2) {
          Manager::maxCacheSizeProducer = u_int32_t(std::stoul(argv[2]));
        }
        if (argc > 3) {
          Manager::shardSizeProducer = u_int32_t(std::stoul(argv[3]));
        }
      }
    } else {
      if (argc > 1) {
        Manager::maxCacheSizeProducer = u_int32_t(std::stoul(argv[1]));
      }
      if (argc > 2) {
        Manager::shardSizeProducer = u_int32_t(std::stoul(argv[2]));
      }
      if (argc > 3) {
        Manager::maxCacheSizeConsumer = u_int32_t(std::stoul(argv[3]));
      }
      if (argc > 4) {
        Manager::shardSizeConsumer = u_int32_t(std::stoul(argv[4]));
      }
    }
  }

  auto start1 = std::chrono::high_resolution_clock::now();
  setUpMap(useDouble);

  auto start2 = std::chrono::high_resolution_clock::now();

  std::cout << "Matching" << '\n';
  Pairing result;
  std::vector<Pair> resultNormal;

  if (useDouble) {
    result = doubleGreedy();
  } else {
    resultNormal = greedy();
  }
  auto stop = std::chrono::high_resolution_clock::now();
  std::cout << "Finished Matching" << '\n';
  /*
  //testcode for the print below
  Pair test = make_tuple(1, 2, 10);
  result.clear();
  result.push_back(test);
  */

  std::pmr::unordered_set<u_int32_t> seenNodes;
  if (!useDouble) {

    u_int64_t summer = 0;
    for (Pair element : resultNormal) {
      seenNodes.insert(std::get<0>(element));
      summer += std::get<2>(element); // the weight to running total of weights
    }
    std::cout << '\n' << "The number pairs is: " << resultNormal.size();
    std::cout << '\n' << "The total weight is: " << summer << '\n' << '\n';
  } else {
    u_int64_t summer = 0;
    int counterTotal = 0;
    for (u_int32_t i = 0; i < result.size(); i++) {
      // std::cout << "Path " << i + 1 << " :" << "\n";
      int counter = 0;
      for (Pair element : result[i]) {
        seenNodes.insert(std::get<0>(element));
        counter++;
        /*
        std::cout << "\t" << "Pair " << counter++ << " :" << "\n";
        std::cout << "\t\t" << "First Node: " << "\t" << std::get<0>(element)
                  << "\n";
        std::cout << "\t\t" << "Second Node: " << "\t" << std::get<1>(element)
                  << "\n";
        std::cout << "\t\t" << "Weight: " << "\t" << std::get<2>(element)
                  << "\n";
        */
        summer +=
            std::get<2>(element); // the weight to running total of weights
      }
      counterTotal += counter;
    }
    std::cout << '\n' << "The number pairs is: " << counterTotal;
    std::cout << '\n' << "The total weight is: " << summer << '\n' << '\n';
  }
  const std::chrono::duration<double> elapsed_seconds1{start2 - start1};
  const std::chrono::duration<double> elapsed_seconds2{stop - start2};
  const std::chrono::duration<double> elapsed_seconds3{stop - start1};
  std::cout << "\nExecution time setup: " << elapsed_seconds1.count()
            << " seconds" << '\n';
  std::cout << "\nExecution time matching: " << elapsed_seconds2.count()
            << " seconds" << '\n';
  std::cout << "\nExecution time total: " << elapsed_seconds3.count()
            << " seconds" << '\n';
  for (u_int32_t i = 1; i < nrProducers + 1; i++) {
    if (seenNodes.find(i) == seenNodes.end()) {
      std::cout << i << " was not paired" << '\n';
    }
  }
  return 0;
}

std::vector<Pair> greedy() {
  std::vector<Pair> matching;

  std::unordered_set<u_int32_t> consumers;
  std::unordered_set<u_int32_t> producers;

  std::vector<std::pair<u_int32_t, u_int32_t>> neighbors;
  for (u_int32_t i = 1; i < nrProducers + 1; i++) {
    if (producers.find(i) != producers.end()) {
      continue;
    }
    neighbors = Manager::getProducerNeighborhood(i);
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

  // These sets keep track of the global availability of nodes
  std::unordered_set<u_int32_t> consumers;
  std::unordered_set<u_int32_t> producers;

  std::vector<Pair> path;

  for (u_int32_t i = 1; i < nrProducers + 1; i++) {
    path.clear(); // init path
    std::cout << "Matching producer number: " << i << "\t\r" << std::flush;
    // find available node
    if (producers.find(i) != producers.end()) {
      continue;
    }

    // setup
    // set the next node to explore to the start and mark it as taken
    u_int32_t nextNode = i;
    u_int32_t originNode;
    Edge edge;
    bool typeNode = true;

    // Using only the global sets for now but these could be needed for some
    // edge cases
    // These sets keep track of what nodes are apart of the path
    // std::unordered_set<u_int32_t> consumersPath;
    // std::unordered_set<u_int32_t> producersPath;

    // add the origin producer to the path
    // producersPath.insert(i);
    producers.insert(i);

    // repeat for rest of path
    // this while ends when a node 0 is found
    // which means the neighborhood had no available nodes
    while (true) {
      originNode = nextNode;
      // edge = nextEdge(originNode, typeNode, consumers, producers,
      // consumersPath, producersPath);

      edge = nextEdge(originNode, typeNode, consumers, producers);
      nextNode = edge.first;

      if (nextNode == 0) {
        break;
      }

      // Add the new node to the path
      if (typeNode) {
        // consumersPath.insert(nextNode);
        consumers.insert(nextNode);
      } else {
        // producersPath.insert(nextNode);
        producers.insert(nextNode);
      }

      // Pair nextPair = std::make_tuple(originNode, nextNode, edge.second);

      //  add only the edges that were found from a producer
      //  this is not strictly correct but works for now
      //  it will be slightly lower than what it should be for double
      if (typeNode) {
        path.emplace_back(originNode, nextNode, edge.second);
        // switched to the inplace construction above
        // path.push_back(nextPair);
      }
      // Finally flip typenode since
      // Producer -> Consumer
      // Consumer -> Producer
      typeNode = !typeNode;
    }
    // add the path to the collection of paths
    matching.push_back(path);
    // Also make the nodes in the path unavailable
    // while taking care to not include a straggler

    // assuming all pairs originate in producers for now
    for (Pair pair : path) {
      producers.insert(std::get<0>(pair));
      consumers.insert(std::get<1>(pair));
    }
  }

  return matching;
}

Edge nextEdge(u_int32_t node, bool typeNode,
              const std::unordered_set<u_int32_t> &consumers,
              const std::unordered_set<u_int32_t> &producers) {
  // const std::unordered_set<u_int32_t> &consumersPath,
  // const std::unordered_set<u_int32_t> &producersPath) {

  std::vector<Edge> neighbors;

  if (typeNode) {
    for (Edge neighbor : Manager::getProducerNeighborhood(node)) {

      // See doubleGreedy for why this is commented
      // If it is already in the path ignore it
      // if (consumersPath.find(neighbor.first) != consumersPath.end()) {
      // continue;
      // }

      // Add the consumer to the path if it is available
      if (consumers.find(neighbor.first) == consumers.end()) {
        neighbors.push_back(neighbor);
      }
    }
  } else {
    for (Edge neighbor : Manager::getConsumerNeighborhood(node)) {
      // if (producersPath.find(neighbor.first) != producersPath.end()) {
      // continue;
      // }

      if (producers.find(neighbor.first) == producers.end()) {
        neighbors.push_back(neighbor);
      }
    }
  }

  // Find the best neighbor in the neighborhood
  // Might be worth it to move this part into the if statements
  // It avoids the neighbors vector but puts more code in the if statements
  u_int32_t highestIndex = 0;
  u_int32_t highestWeight = 0;
  for (Edge neighbor : neighbors) {
    if (neighbor.second > highestWeight) {
      highestWeight = neighbor.second;
      highestIndex = neighbor.first;
    }
  }

  return std::make_pair(highestIndex, highestWeight);
}
