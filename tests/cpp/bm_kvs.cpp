#include <benchmark/benchmark.h>
#include <string>

#define private public
#define final 
#include "kvs.hpp"
#undef private
#undef final
#include "internal/kvs_helper.hpp"

static void BM_get_hash_bytes(benchmark::State& state) {
    // Prepare a test string of configurable size
    std::string data(state.range(0), 'a');
    for (auto _ : state) {
        benchmark::DoNotOptimize(get_hash_bytes(data));
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(data.size()));
}

// Register the function as a benchmark with different input sizes
BENCHMARK(BM_get_hash_bytes)->Range(16, 16<<10);

BENCHMARK_MAIN();
