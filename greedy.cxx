#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <pthread.h>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <tuple>
#include <unordered_set>
#include <vector>

#define L_VALUE 10000

// Is it even worth letting Prodcuer and Consumer be different types?
// I just made Consumer into a Producer alias to avoid problems down the line.
using Producer = u_int32_t;
using Consumer = Producer;
using Weight = double;

using Edge = std::tuple<Producer, Consumer, Weight>;
using Match = Edge;
using MatchVec = std::vector<Match>;
using Neighborhood = std::vector<Edge>;
using Availability = std::unordered_set<Producer>;

bool swapProdCon;
bool sequential = true;

u_long nrProducers;
u_long nrConsumers;
u_long nrEdges;
u_long graph_size;

int readMetaData(std::string filelocation) {
  std::ifstream chunk_stream(filelocation);
  std::string line;

  if (!chunk_stream.is_open()) {
    std::cout << "error: couldn't read" << filelocation << "file\n";
    return 1;
  }

  // discard comments
  while (getline(chunk_stream, line, '\n')) {
    if (line[0] != '%') {
      break;
    }
  }
  std::istringstream input;
  input.str(line);

  std::string sRows, sColumns, sEdges;
  getline(input, sRows, ' ');
  getline(input, sColumns, ' ');
  getline(input, sEdges, ' ');

  // could increment these by one inorder to not have to do weirdness in the
  // loops later on
  nrProducers = stoul(sRows);
  nrConsumers = stoul(sColumns);
  nrEdges = stoul(sEdges);
  graph_size = nrProducers + nrConsumers;

  getline(chunk_stream, line, '\n');
  std::istringstream line2;
  line2.str(line);

  std::string sProducer, sConsumer;
  getline(line2, sProducer, ' ');
  getline(line2, sConsumer, ' ');

  // determine whether row 1 or 2 is the sorted row
  auto firstProducer = static_cast<Producer>(stoul(sProducer));
  auto firstConsumer = static_cast<Consumer>(stoul(sConsumer));
  if (firstProducer > firstConsumer) {
    swapProdCon = true;
  }

  std::cout << "read metadata" << '\n';
  return 0;
}

Match matchNeighborhood(Neighborhood *neighborhood,
                        Availability *availability) {
  if (neighborhood->size() == 0) {
    return std::make_tuple(0, 0, 0);
  }

  Weight currentWeight;
  Consumer currentConsumer;
  Producer producer = std::get<0>(neighborhood->front());
  Consumer bestConsumer = 0;
  Weight highestWeight = 0;

  for (Edge edge : *neighborhood) {
    currentConsumer = std::get<1>(edge);
    currentWeight = std::get<2>(edge);
    // if (availability->find(producer) != availability->end()) {
    // continue;
    // }
    if (availability->find(currentConsumer) != availability->end()) {
      continue;
    }
    if (highestWeight < currentWeight) {
      highestWeight = currentWeight;
      bestConsumer = currentConsumer;
    }
  }
  // need to double check this to make sure it is right
  if (producer == 0 || 0 == bestConsumer) {
    return std::make_tuple(0, 0, 0);
  }
  availability->insert(bestConsumer);
  availability->insert(producer);
  return std::make_tuple(producer, bestConsumer, highestWeight);
}

int readChunk(u_int32_t chunkToRead, Neighborhood *neighborhood,
              MatchVec *matches, Availability *availability,
              std::string filelocation, bool externalGraph) {
  std::string filename;
  if (!externalGraph) {
    filename = "graphs/graph" + std::to_string(chunkToRead) + ".txt";

    // external graphs are not chunked as far as I have seen
  } else if (chunkToRead == 0) {
    filename = filelocation;
  } else { // this else is a measureable slowdown on small graphs actually
    return 1;
  }

  std::ifstream chunk_stream(filename);

  if (!chunk_stream.is_open()) {
    // std::cout << "error: reading file *" << chunkToRead << ".txt\n";
    return 1;
  }

  std::string line;
  Producer producer;
  Consumer consumer;
  Weight weight;
  std::stringstream sLine("");
  u_int32_t lastProducer = 0;

  // discard comments
  while (getline(chunk_stream, line, '\n')) {
    if (line[0] != '%') {
      break;
    }
  }

  // discard metadata
  if (chunkToRead == 0) {
    getline(chunk_stream, line);
  }

  while (getline(chunk_stream, line)) {

    std::stringstream sLine;
    sLine.str(line);
    std::string sProducer, sConsumer, sWeight;

    // extract each 'word'
    getline(sLine, sProducer, ' ');
    getline(sLine, sConsumer, ' ');
    getline(sLine, sWeight, ' ');

    producer = static_cast<u_int32_t>(stoul(sProducer));
    consumer = static_cast<u_int32_t>(stoul(sConsumer));
    weight = std::stod(sWeight);

    // swaps the producer and consumer in case the consumers are sequential
    // instead of the producers
    if (swapProdCon) {
      std::swap(producer, consumer);
    }

    if (lastProducer == producer) {
      neighborhood->emplace_back(producer, consumer, weight);

    } else if (lastProducer < producer) {
      Match match = matchNeighborhood(neighborhood, availability);
      // dont add zero weight edges
      if (std::get<2>(match) != 0) {
        matches->push_back(match);
      }

      neighborhood->clear();
      lastProducer = producer;
      neighborhood->emplace_back(producer, consumer, weight);

    } else {
      sequential = false;
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  std::cout.imbue(std::locale("en_US.UTF-8")); // Use thousands separator
  auto start = std::chrono::high_resolution_clock::now();
  u_int32_t chunkNumber = 0;
  Neighborhood neighborhood;
  MatchVec matches;
  Availability availability;
  std::string filelocation = "graphs/graph0.txt";
  bool externalGraph = false;
  if (argc > 0) {
    filelocation = argv[1];
    externalGraph = true;
  }

  if (readMetaData(filelocation) == 1) {
    std::cout << "error: basefile couldnt be read" << "\n";
    return 1;
  }

  while (true) {
    std::cout << "Reading chunk number: " << chunkNumber << "\t\r"
              << std::flush;
    if (readChunk(chunkNumber, &neighborhood, &matches, &availability,
                  filelocation, externalGraph) == 1) {
      break;
    }
    chunkNumber++;
  }
  Weight max_weight = 0;

  for (const auto &[producer, consumer, weight] : matches) {
    max_weight += weight;
  }
  std::cout << "\nsize of matches is: " << matches.size() << '\n';

  std::cout << "Max weight is : " << max_weight << '\n';
  auto stop = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<double> elapsed_seconds{stop - start};
  std::cout << "\nExecution time: " << elapsed_seconds.count() << " seconds"
            << '\n';
  if (sequential) {

    std::cout << "Graph is sequential" << "\n";
  } else {

    std::cout << "Graph is not sequential " << "\n";
  }
  return 0;
}
