#pragma once

#include <cstdio>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

namespace gspl::benchmark {

struct BenchmarkResult {
    std::string name;
    double duration_ms;
    std::string unit;
};

class BenchmarkRegistry {
    std::vector<BenchmarkResult> results_;
public:
    void record(std::string name, double ms, std::string unit = "ms") {
        results_.push_back({std::move(name), ms, std::move(unit)});
    }

    void report() const {
        std::printf("\n=== Benchmark Results ===\n");
        for (auto const& r : results_) {
            std::printf("  %-40s %8.2f %s\n", r.name.c_str(), r.duration_ms, r.unit.c_str());
        }
        std::printf("=========================\n");
    }

    bool regression_against(std::vector<BenchmarkResult> const& baselines, double threshold_pct = 20.0) const {
        bool any_regression = false;
        for (auto const& r : results_) {
            auto it = std::find_if(baselines.begin(), baselines.end(),
                [&](auto const& b) { return b.name == r.name; });
            if (it != baselines.end() && it->duration_ms > 0) {
                double pct = (r.duration_ms - it->duration_ms) / it->duration_ms * 100.0;
                if (pct > threshold_pct) {
                    std::printf("  REGRESSION: %s %.1f%% slower (%.2f vs %.2f %s)\n",
                        r.name.c_str(), pct, r.duration_ms, it->duration_ms, r.unit.c_str());
                    any_regression = true;
                }
            }
        }
        return any_regression;
    }

    auto const& results() const { return results_; }
    void clear() { results_.clear(); }
};

struct Timer {
    std::chrono::high_resolution_clock::time_point start;
    std::string label;
    BenchmarkRegistry* reg;

    Timer(std::string l, BenchmarkRegistry& r)
        : start(std::chrono::high_resolution_clock::now())
        , label(std::move(l)), reg(&r) {}

    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration<double, std::milli>(end - start).count();
        reg->record(std::move(label), ms);
    }
};

#define BENCHMARK_SCOPE(reg, name) gspl::benchmark::Timer _gspl_bm_timer_##__LINE__(name, reg)

} // namespace gspl::benchmark
