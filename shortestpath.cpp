#include "lispylist.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <istream>
#include <memory>
#include <ostream>
#include <string_view>
#include <sstream>
#include <vector>

std::ostream debug{nullptr};

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
  int layer_count = 1;
  for (; layer != layers_end; ++layer, ++layer_count) {
    debug << "Examining layer " << layer_count << '\n';
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
    debug << "    previous layer has " << previous_layer.size() << " vertices\n";
    debug << "    current layer has " << current_layer.size() << " vertices\n";

    // Update `current_layer` (and `previous_layer`, if necessary) based on the
    // edges between the two layers.
    for (auto iter = edges_begin; iter != edges_end; ++iter) {
      const auto [from, to, weight] = *iter;
      if (previous_layer[from] == nil) {
        debug << "    previous vertex " << from << " now has minimum weight zero\n";
        previous_layer[from] = nil.prepend(VertexState{
          .least_total_weight_to_here = 0,
          .vertex = from
        });
      }
      const VertexState& previous = previous_layer[from].head();
      const double proposed_total = previous.least_total_weight_to_here + weight;
      if (current_layer[to] == nil || current_layer[to].head().least_total_weight_to_here > proposed_total) {
        debug << "    current vertex " << to << " now has minimum weight " << proposed_total << '\n';
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

void print_layer_subgraph(
    int layer,
    const std::vector<int>& vertices,
    std::ostream& graphviz) {
  graphviz <<
    "\n"
    "  subgraph cluster_" << layer << " {\n"
    "    style=filled;\n"
    "    color=lightgrey;\n"
    "    node [style=filled, color=white];\n"
    "    label = \"Layer " << layer << "\";\n"
    "\n";
  for (const int vertex : vertices) {
    graphviz <<
    "    node_" << layer << "_" << vertex << " [label=\"" << vertex << "\"];\n";
  }
  graphviz <<
    "  }\n";
}

bool read_layer(
    std::istream& lines,
    std::string& buffer,
    std::istringstream& buffer_stream,
    std::vector<Edge>& destination,
    std::ostream& graphviz,
    std::vector<int>& vertices_scratch,
    int layer) {
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
      break;
    }
    buffer_stream >> to >> weight;
    if (!buffer_stream) {
      return false;
    }
    destination.push_back(Edge{.from = from, .to = to, .weight = weight});
  }

  // Append graph output:

  // We'll use `vertices` to determine the distinct `from` vertices (at least
  // for the first layer) and `to` vertices (for all layers).
  std::vector<int>& vertices = vertices_scratch;

  if (layer == 1) {
    // Edges go from layer n-1 to layer n. If this is layer 1, then we have to
    // state the nodes for layer 0 first.
    for (const Edge& edge : destination) {
      vertices.push_back(edge.from);
    }
    std::sort(vertices.begin(), vertices.end());
    vertices.erase(std::unique(vertices.begin(), vertices.end()), vertices.end());
    print_layer_subgraph(0, vertices, graphviz);
    vertices.clear();
  }

  // Print the "to" vertices.
  for (const Edge& edge : destination) {
    vertices.push_back(edge.to);
  }
  std::sort(vertices.begin(), vertices.end());
  vertices.erase(std::unique(vertices.begin(), vertices.end()), vertices.end());
  print_layer_subgraph(layer, vertices, graphviz);

  // Print all of the edges.
  graphviz <<
    "\n";
  for (const Edge& edge : destination) {
    graphviz <<
    "  node_" << (layer - 1) << "_" << edge.from << " -> node_" << layer << "_" << edge.to << " [label=\"" << edge.weight << "\"]\n";
  }

  vertices.clear();

  return true;
}

struct LayerGeneratorState {
  std::vector<Edge> incoming;
  std::istream& input;
  std::string buffer;
  std::istringstream buffer_stream;
  std::ostream& graphviz;
  std::vector<int> vertices_scratch;
  int layer;
};

class LayerIterator {
  std::shared_ptr<LayerGeneratorState> state;

public:
  LayerIterator()
  : state(nullptr) {
  }
  explicit LayerIterator(std::istream& input, std::ostream& graphviz)
  : state(new LayerGeneratorState{
      .incoming = {},
      .input = input,
      .buffer = {},
      .buffer_stream = {},
      .graphviz = graphviz,
      .vertices_scratch = {},
      .layer = 0
    }) {
    // Get the initial layer.
    ++(*this);
  }
  LayerIterator(const LayerIterator&) = default;
  LayerIterator(LayerIterator&&) = default;

  LayerIterator& operator++() {
    if (!read_layer(
        state->input,
        state->buffer,
        state->buffer_stream,
        state->incoming,
        state->graphviz,
        state->vertices_scratch,
        ++state->layer)) {
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
  if (const char *raw = std::getenv("DEBUG")) {
    if (std::string_view{raw} == "1") {
      debug.rdbuf(std::cerr.rdbuf());
    }
  }

  std::cout <<
    "strict digraph {\n"
    "  fontname=\"Helvetica,Arial,sans-serif\"\n"
    "  node [fontname=\"Helvetica,Arial,sans-serif\"]\n"
    "  edge [fontname=\"Helvetica,Arial,sans-serif\", fontsize=\"8pt\"]\n"
    "  rankdir=\"LR\";\n";

  const std::vector<LispyList<VertexState>> paths = cheapest_paths(
    LayerIterator{std::cin, std::cout},
    LayerIterator{});
  int num_layers = -1;
  debug << "Optimal paths (backwards):\n";
  std::cout <<
    "\n";
  for (int i = 0; i < int(paths.size()); ++i) {
    const LispyList<VertexState>& list = paths[i];
    if (num_layers == -1) {
      num_layers = std::distance(list.begin(), list.end());
    }
    int current_layer = num_layers - 1;
    debug << "weight " << list.head().least_total_weight_to_here << ':';
    int to = -1;
    int from = -1;
    for (auto it = list.begin(); it != list.end(); ++it, --current_layer) {
      // debug << "current layer is " << current_layer << '\n';
      debug << " -> " << it->vertex;
      if (to == -1) {
        to = it->vertex;
        continue;
      } else if (from == -1) {
        from = it->vertex;
      } else {
        to = from;
        from = it->vertex;
      }
      std::cout <<
    "  node_" << current_layer << "_" << from << " -> node_" << (current_layer + 1) << "_" << to << " [penwidth=\"3\", color=\"red\"];\n";
    }
    // std::cout <<
    // "  node_0_" << from << " -> node_1_" << to << " [penwidth=\"3\", color=\"red\"];\n";
    debug << '\n';
  }

  std::cout <<
    "}\n";
}
