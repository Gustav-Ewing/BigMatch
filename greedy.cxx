#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define L_VALUE 10000

using namespace std;
using Producer = u_int32_t;
using Consumer = u_int32_t;
using Weight = u_int32_t;
using Chunk = std::unordered_map<Producer, unordered_map<Consumer, Weight>>;
u_long nrProducers;
u_long nrConsumers;
u_long nrEdges;
u_long graph_size;
unordered_set<Consumer> matched_consumers;
unordered_map<Producer, Weight> all_matches;

Chunk read_chunk(u_int32_t chunk) {
  Chunk neighborhoods;
  string filename = "graphs/graph" + to_string(chunk) + ".txt";

  ifstream chunk_stream(filename);

  if (!chunk_stream.is_open()) {
    return neighborhoods;
  }
  string line;
  // read metadata of first line of first chunk
  if (chunk == 0) {
    getline(chunk_stream, line, '\n');
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

    std::cout << "read metadata" << '\n';
  }
  vector<u_int32_t> line_words;
  string word;
  while (getline(chunk_stream, line)) {
    line_words.clear();
    stringstream unsplit_line(line);
    while (unsplit_line >> word) {
      line_words.push_back(static_cast<u_int32_t>(stoul(word)));
    }
    neighborhoods[line_words[0]][line_words[1]] = line_words[2];
  }
  return neighborhoods;
}

void match_chunk(Chunk chunk) {
  for (const auto &[producer, edges] : chunk) {
    tuple<Consumer, Weight> best_match = {nrConsumers + 1, 0};
    u_int32_t checked_edges = 0;
    for (const auto &[consumer, weight] : edges) {
      if (checked_edges > L_VALUE) {
        break;
      }
      if (matched_consumers.find(consumer) != matched_consumers.end()) {
        continue;
      }
      if (weight > get<1>(best_match)) {
        best_match = {consumer, weight};
      }
      checked_edges++;
    }
    if (!(get<1>(best_match) == nrConsumers + 1)) {
      all_matches[producer] = get<1>(best_match);
      matched_consumers.insert(get<0>(best_match));
    }
  }
}

int main() {

  std::cout.imbue(std::locale("en_US.UTF-8")); // Use thousands separator
  auto start = chrono::high_resolution_clock::now();
  u_int32_t chunk_number = 0;

  Chunk chunk_neighborhoods = read_chunk(chunk_number);
  while (!chunk_neighborhoods.empty()) {
    cout << "Matching chunk number: " << chunk_number << "\t\r" << flush;
    match_chunk(chunk_neighborhoods);

    chunk_number++;
    chunk_neighborhoods.clear();
    chunk_neighborhoods = read_chunk(chunk_number);
  }
  Weight max_weight = 0;

  for (const auto &[producer, weight] : all_matches) {
    max_weight += weight;
  }
  cout << "\nsize of matches is: " << all_matches.size() << '\n';

  cout << "Max weight is : " << max_weight << '\n';
  auto stop = chrono::high_resolution_clock::now();
  const chrono::duration<double> elapsed_seconds{stop - start};
  cout << "\nExecution time: " << elapsed_seconds.count() << " seconds" << '\n';
  return 0;
}
