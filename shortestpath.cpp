#include "lispylist.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <istream>
#include <memory>
#include <sstream>
#include <vector>

struct Edge {
  int from; // vertex name, `>= 0`
  int to; // vertex name, `>= 0`
  double weight; // a real number
};

struct VertexState {
  double least_total_weight_to_here;
  int vertex; // as named by `Edge::from` or `Edge::to`
};

template <typename EdgeRangeIterator>
std::vector<LispyList<VertexState>> cheapest_paths(
    EdgeRangeIterator layer,
    EdgeRangeIterator layers_end) {
  // `nil` is a handy shorthand for the "empty" or "end" lispy list.
  const LispyList<VertexState> nil;

  // `*layer` can be unpacked as two forward iterators to `Edge`.
  // The idea is that a layer is represented as a sequence of edges from the
  // previous layer to the the current layer, and `[layer, layers_end)` is a
  // sequence of layers.
  // The sequences of edges are covered by forward iterators, while `layer` is
  // an input iterator (so layers can be generated lazily).
  std::vector<LispyList<VertexState>> previous_layer;
  std::vector<LispyList<VertexState>> current_layer;
  int layer_count = 1; // TODO: no
  for (; layer != layers_end; ++layer, ++layer_count /*TODO: no*/) {
    std::cout << "Examining layer " << layer_count << '\n';
    // Deduce which vertices are in a layer by examining the vertices named in
    // the edges between the two layers.
    int max_previous_vertex = -1;
    int max_current_vertex = -1;
    const auto [edges_begin, edges_end] = *layer;
    for (auto iter = edges_begin; iter != edges_end; ++iter) {
      const Edge& edge = *iter;
      max_previous_vertex = std::max(max_previous_vertex, edge.from);
      max_current_vertex = std::max(max_current_vertex, edge.to);
    }
    previous_layer.resize(max_previous_vertex + 1);
    current_layer.clear();
    current_layer.resize(max_current_vertex + 1);
    std::cout << "    previous layer has " << previous_layer.size() << " vertices\n";
    std::cout << "    current layer has " << current_layer.size() << " vertices\n";

    // Update `current_layer` (and `previous_layer`, if necessary) based on the
    // edges between the two layers.
    for (auto iter = edges_begin; iter != edges_end; ++iter) {
      const auto [from, to, weight] = *iter;
      if (previous_layer[from] == nil) {
        std::cout << "    previous vertex " << from << " now has minimum weight zero\n";
        previous_layer[from] = nil.prepend(VertexState{
          .least_total_weight_to_here = 0,
          .vertex = from
        });
      }
      const VertexState& previous = previous_layer[from].head();
      const double proposed_total = previous.least_total_weight_to_here + weight;
      if (current_layer[to] == nil || current_layer[to].head().least_total_weight_to_here > proposed_total) {
        std::cout << "    current vertex " << to << " now has minimum weight " << proposed_total << '\n';
        current_layer[to] = previous_layer[from].prepend(VertexState{
          .least_total_weight_to_here = proposed_total,
          .vertex = to
        });
      }
    }

    using std::swap;
    swap(previous_layer, current_layer);
  }

  // Now `previous_layer` contains the information about the vertices in the
  // final layer. Sort by `least_total_weight_to_here` and return all of the
  // paths that have the minimal total weight.
  struct ByTotalWeight {
    bool operator()(
        const LispyList<VertexState>& left,
        const LispyList<VertexState>& right) const {
      // Order by `least_total_weight_to_here`, with empty lists last.
      if (left.empty()) {
        return false;
      }
      if (right.empty()) {
        return true;
      }
      return left.head().least_total_weight_to_here < right.head().least_total_weight_to_here;
    }
  };

  std::sort(previous_layer.begin(), previous_layer.end(), ByTotalWeight{});
  assert(!previous_layer.empty());
  assert(!previous_layer[0].empty());
  const double least_total_weight = previous_layer[0].head().least_total_weight_to_here;
  const auto end_least = std::find_if(
    previous_layer.begin(),
    previous_layer.end(),
    [=](const LispyList<VertexState>& list) {
      return list.empty() || list.head().least_total_weight_to_here != least_total_weight;
  });
  previous_layer.erase(end_least, previous_layer.end());
  return previous_layer;
}

bool read_layer(
    std::istream& lines,
    std::string& buffer,
    std::istringstream& buffer_stream,
    std::vector<Edge>& destination) {
  destination.clear();

  if (!std::getline(lines, buffer)) {
    return false;
  }
  buffer_stream.clear();
  buffer_stream.str(buffer);

  int from;
  int to;
  double weight;
  for (;;) {
    buffer_stream >> from;
    if (!buffer_stream) {
      return true;
    }
    buffer_stream >> to >> weight;
    if (!buffer_stream) {
      return false;
    }
    destination.push_back(Edge{.from = from, .to = to, .weight = weight});
  }
}

struct LayerGeneratorState {
  std::vector<Edge> incoming;
  std::istream& input;
  std::string buffer;
  std::istringstream buffer_stream;
};

class LayerIterator {
  std::shared_ptr<LayerGeneratorState> state;

public:
  LayerIterator()
  : state(nullptr) {
  }
  explicit LayerIterator(std::istream& input)
  : state(new LayerGeneratorState{.incoming = {}, .input = input, .buffer = {}, .buffer_stream = {}}) {
    // Get the initial layer, if any.
    ++(*this);
  }
  LayerIterator(const LayerIterator&) = default;
  LayerIterator(LayerIterator&&) = default;

  LayerIterator& operator++() {
    if (!read_layer(state->input, state->buffer, state->buffer_stream, state->incoming)) {
      state.reset();
    }
    return *this;
  }

  LayerIterator operator++(int) {
    LayerIterator old = *this;
    ++(*this);
    return old;
  }

  // const std::vector<Edge>& operator*() const {
  //   return state->incoming;
  // }
  std::pair<std::vector<Edge>::const_iterator, std::vector<Edge>::const_iterator>
  operator*() const {
    assert(state);
    return std::make_pair(state->incoming.begin(), state->incoming.end());
  }

  bool operator==(const LayerIterator& other) const {
    return state == other.state;
  }

  bool operator!=(const LayerIterator& other) const {
    return state != other.state;
  }
};

int main() {
  const std::vector<LispyList<VertexState>> paths = cheapest_paths(
    LayerIterator{std::cin},
    LayerIterator{});
  std::cout << "Optimal paths (backwards):\n";
  for (const LispyList<VertexState>& list : paths) {
    std::cout << "weight " << list.head().least_total_weight_to_here << ':';
    for (auto it = list.begin(); it != list.end(); ++it) {
      std::cout << " -> " << it->vertex;
    }
    std::cout << '\n';
  }
}
