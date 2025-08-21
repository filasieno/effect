#include <benchmark/benchmark.h>
#include <thread>
#include <chrono>

void hello() {
  std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
}

static void BM_hello(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    hello();
  } 
}

// Register the function as a benchmark
BENCHMARK(BM_hello);

// Run the benchmark
BENCHMARK_MAIN();