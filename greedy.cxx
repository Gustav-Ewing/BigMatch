#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

using namespace std;
using Producer = u_int32_t;
using Consumer = u_int32_t;
using Weight = u_int32_t;
using Neighborhoods =
    std::unordered_map<Producer, unordered_map<Consumer, Weight>>;
u_long nrProducers;
u_long nrConsumers;
u_long nrEdges;
u_long graph_size;
void read_metadata() {

  std::string filename = "graph" + std::to_string(0) + ".txt";
  std::ifstream chunk_stream(filename);
  std::string inputstr;

  getline(chunk_stream, inputstr, '\n');
  std::istringstream input;
  input.str(inputstr);

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
  chunk_stream.close();
}

Neighborhoods read_chunk(u_int32_t chunk) {
  Neighborhoods neighborhoods;
  string filename = "graph" + to_string(chunk) + ".txt";
  ifstream chunk_stream(filename);
  string line;

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
    stringstream unsplit_line(line);
    while (unsplit_line >> word) {
      line_words.push_back(static_cast<u_int32_t>(stoul(word)));
    }
    neighborhoods[line_words[0]][line_words[1]] = line_words[2];
  }
  return neighborhoods;
}

int main() {

  u_int32_t chunk_number = 0;
  read_metadata();
  return 0;
}
