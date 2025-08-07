#include <algorithm>
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

using Weight = uint32_t;

class Configuration {
public:
  static inline size_t Prosumers = 1'000'000;
  static inline size_t Consumers = Prosumers * 5;
  static inline auto Size = static_cast<double>(Prosumers + Consumers);
  static inline size_t EdgesPerChunk = 10'000'000;
  static inline int SparseFactor = static_cast<int>(
      3 * log(Size) / Size); // percent chance to not create make_edge
  static inline unsigned long Seed = 1234;
  static inline int Gamma =
      2; // GAMMA value, determines the scaling of weights for a nodes edges
  static inline int Beta = 4;
  static inline Weight MaxWeight = 100; // Max allowed weight
};

enum class CLIOptions {
  Prosumers,
  Consumers,
  Size,
  EdgesPerChunk,
  SparseFactor,
  Seed,
  Gamma,
  Beta,
  MaxWeight,
  Unknown
};

static CLIOptions hashCLIOptions(std::string input) {
  if (input == "-p")
    return CLIOptions::Prosumers;
  if (input == "-c")
    return CLIOptions::Consumers;
  if (input == "--SIZE")
    return CLIOptions::Size;
  if (input == "-e")
    return CLIOptions::EdgesPerChunk;
  if (input == "-s")
    return CLIOptions::SparseFactor;
  if (input == "-g")
    return CLIOptions::Gamma;
  if (input == "-b")
    return CLIOptions::Beta;
  if (input == "-w")
    return CLIOptions::MaxWeight;

  return CLIOptions::Unknown;
}

using namespace std;

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
    uint64_t a = static_cast<uint32_t>(p.first);
    uint64_t b = static_cast<uint32_t>(p.second);
    u_int64_t hash = (a << 32) | (b);
    // cout << "a = " << p.first << " and b = " << p.second << " and hash is "
    // << hash << "\n";
    return hash;
  }
};

static int remove_old_graphs() {
  filesystem::create_directory("graphs");
  namespace fs = std::filesystem;

  std::string directory =
      "./graphs/"; // Change to your target directory if needed
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
  std::string filename = "graphs/graph0.txt";
  std::string temp_filename = "graphs/temp_graph0.txt";

  std::ifstream input_file(filename);
  std::ofstream temp_file(temp_filename);

  if (!input_file || !temp_file) {
    std::cerr << "Error opening file.\n";
    return 1;
  }

  // Write the new line first
  temp_file << Configuration::Prosumers << " " << Configuration::Consumers
            << " " << num_of_edges << "\n";
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

int main(int argc, char *argv[]) {
  remove_old_graphs();
  std::cout.imbue(std::locale("en_US.UTF-8")); // Use thousands separator

  for (int i = 1; i < argc; i++) {
    switch (hashCLIOptions(argv[i])) {
    case CLIOptions::Consumers:
      Configuration::Consumers = stoul(argv[i + 1]);
      break;
    case CLIOptions::Prosumers:
      Configuration::Prosumers = stoul(argv[i + 1]);
      break;
    case CLIOptions::Size:
      Configuration::Size = stod(argv[i + 1]);
      break;
    case CLIOptions::EdgesPerChunk:
      Configuration::EdgesPerChunk = stoul(argv[i + 1]);
      break;
    case CLIOptions::SparseFactor:
      Configuration::SparseFactor = stoi(argv[i + 1]);
      break;
    case CLIOptions::Seed:
      Configuration::Seed = stoul(argv[i + 1]);
      break;
    case CLIOptions::Gamma:
      Configuration::Gamma = stoi(argv[i + 1]);
      break;
    case CLIOptions::Beta:
      Configuration::Beta = stoi(argv[i + 1]);
      break;
    case CLIOptions::MaxWeight:
      Configuration::MaxWeight = static_cast<Weight>(stoul(argv[i + 1]));
      break;
    case CLIOptions::Unknown:
      return 1;
    }

    mt19937 gen;
    mt19937 gen2;
    mt19937 gen3;
    gen.seed(Configuration::Seed);
    gen2.seed(Configuration::Seed);
    gen3.seed(Configuration::Seed);
    std::uniform_real_distribution<> uniform_distrib(0, 1);
    std::poisson_distribution<uint32_t> weight_distrib(
        (Configuration::MaxWeight) / 2);
    // below is for binomial dist
    std::binomial_distribution<> degree_dist(
        static_cast<int>(Configuration::Consumers),
        Configuration::SparseFactor);
    // below 2 rows are for exponential distribution of edges
    // double expected_degree = SPARSEFACTOR * CONSUMERS; // USED FOR
    // EXPONENTIAL std::exponential_distribution<> degree_dist(1.0 /
    // expected_degree); // USED FOR EXPONENTIAL
    std::uniform_int_distribution<> consumer_dist(
        0, static_cast<int>(Configuration::Consumers) - 1);

    unordered_map<std::pair<int, int>, Weight, pair_hash> graph;
    // unordered_map<int32_t, Weight> consumer_weights;
    // for (u_int32_t i = 0; i < CONSUMERS; i++) {
    //   consumer_weights[i] = MAXWEIGHT;
    // }
    std::vector<Weight> consumer_weights(Configuration::Consumers,
                                         Configuration::MaxWeight);

    int chunk = 0;
    uint32_t num_of_edges = 0;
    for (size_t i = 0; i < Configuration::Prosumers; i++) {

      // int degree = degree_dist(gen3);

      int degree =
          std::max(1, std::min((int)degree_dist(gen3),
                               static_cast<int>(Configuration::Consumers)));
      int producer_current_edges = 0;

      Weight weight_limit = Configuration::MaxWeight;
      while (producer_current_edges < degree) {

        auto consumer = static_cast<size_t>(consumer_dist(gen3));
        if (graph.find({i, consumer}) != graph.end()) {
          continue;
        }
        Weight consumer_weight_limit = Configuration::MaxWeight;

        consumer_weight_limit = consumer_weights[consumer] *
                                static_cast<unsigned int>(Configuration::Beta);
        weight_distrib.param(poisson_distribution<uint32_t>::param_type(
            (min(weight_limit, consumer_weight_limit)) / 2));

        Weight new_weight = weight_distrib(gen);
        while (new_weight > weight_limit ||
               new_weight > consumer_weight_limit || new_weight == 0) {
          new_weight = weight_distrib(gen);
        }
        weight_limit =
            std::min(new_weight * static_cast<Weight>(Configuration::Gamma),
                     weight_limit);
        if (graph.find({i, consumer}) != graph.end()) {
          continue;
        }
        graph[{i, consumer}] = new_weight;
        consumer_weights[consumer] =
            std::min(consumer_weights[consumer], new_weight);
        producer_current_edges++;
      }

      if (graph.size() > Configuration::EdgesPerChunk ||
          i == Configuration::Consumers - 1) {
        string file = "graphs/graph" + to_string(chunk) + ".txt";

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

      cout << "Prosumer: " << i + 1 << " out of " << Configuration::Prosumers
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
