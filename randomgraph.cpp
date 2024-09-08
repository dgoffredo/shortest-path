#include <algorithm>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <random>

class LowerBoundedIntegerNormalDistribution {
  std::normal_distribution<> distribution;
  int lower_bound;

public:
  LowerBoundedIntegerNormalDistribution(int lower_bound, int mean, double standard_deviation)
  : distribution(mean, standard_deviation)
  , lower_bound(lower_bound) {}

  template <typename Generator>
  int operator()(Generator&& generator) {
    return std::round(std::max<double>(lower_bound, distribution(generator)));
  }
};

int main() {
  const std::uint32_t seed = []() {
    if (const char *const raw = std::getenv("RAND_SEED")) {
      return std::atoi(raw);
    }
    return 42;
  }();
  std::mt19937 generator{seed};
  LowerBoundedIntegerNormalDistribution layers{2, 10, 3.0};
  LowerBoundedIntegerNormalDistribution vertices_per_layer{1, 5, 2.0};
  LowerBoundedIntegerNormalDistribution inward_edges_per_vertex{1, 3, 1.0};
  std::normal_distribution<> edge_weight{5.0, 20.0};

  const int num_layers = layers(generator);
  int num_vertices_previous = vertices_per_layer(generator);
  for (int i = 1; i < num_layers; ++i) {
    const int num_vertices = vertices_per_layer(generator);
    std::uniform_int_distribution from{0, num_vertices_previous - 1};
    for (int to = 0; to < num_vertices; ++to) {
      const int inward_edges = inward_edges_per_vertex(generator);
      for (int edge = 0; edge < inward_edges; ++edge) {
        std::cout << from(generator) << ' ' << to << ' ' << std::setprecision(1) << edge_weight(generator) << '\t';
      }
    }
    std::cout << '\n';
    num_vertices_previous = num_vertices;
  }
}
