#include "gspl/studio/benchmark.hpp"
#include "gspl/sdk.hpp"
#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>

using gspl::benchmark::BenchmarkRegistry;

static const std::string kSmallEntity = R"(
    module bench_small;
    entity TestEntity {
        gene identity { stable_id: "test"; }
        classification "test";
        morphology { part body { size 1.0, 2.0, 0.5; } }
    }
)";

static const std::string kMediumEntity = R"(
    module bench_medium;
    entity Voltfox {
        gene identity { stable_id: "voltfox_ref"; }
        classification "animal";
        metadata { author "test"; version "1.0"; }
        morphology {
            part head { size 2.0, 2.0, 1.0; offset 0.0, 2.5, 0.0; }
            part body { size 3.0, 2.5, 1.5; offset 0.0, 0.0, 0.0; }
            part tail { size 0.5, 1.5, 0.5; offset -2.5, -0.5, 0.0; }
            joint neck { from head; to body; type hinge; }
        }
    }
)";

static std::string generate_large_module(size_t num_parts) {
    std::string s = "module bench_large;\n";
    s += "entity MegaEntity {\n";
    s += "    gene identity { stable_id: \"mega\"; }\n";
    s += "    classification \"test\";\n";
    s += "    morphology {\n";
    for (size_t i = 0; i < num_parts; ++i) {
        s += "        part p" + std::to_string(i) + " { size 1.0, 1.0, 1.0; offset 0.0, " + std::to_string(i * 2) + ", 0.0; }\n";
    }
    s += "    }\n";
    s += "}\n";
    return s;
}

static void benchmark_compilation_throughput(BenchmarkRegistry& reg) {
    {
        gspl::benchmark::Timer t("compile_small_entity", reg);
        gspl::GsplContext ctx;
        auto buf = gspl::SourceBuffer::from_string("small", kSmallEntity);
        auto result = ctx.compile_source(std::move(buf));
        if (!result.ok()) std::fprintf(stderr, "small entity compilation had errors\n");
    }
    {
        gspl::benchmark::Timer t("compile_medium_entity", reg);
        gspl::GsplContext ctx;
        auto buf = gspl::SourceBuffer::from_string("medium", kMediumEntity);
        auto result = ctx.compile_source(std::move(buf));
        if (!result.ok()) std::fprintf(stderr, "medium entity compilation had errors\n");
    }
    {
        gspl::benchmark::Timer t("compile_large_entity_100_parts", reg);
        gspl::GsplContext ctx;
        auto buf = gspl::SourceBuffer::from_string("large", generate_large_module(100));
        auto result = ctx.compile_source(std::move(buf));
        if (!result.ok()) std::fprintf(stderr, "large entity compilation had errors\n");
    }
}

static void benchmark_memory_usage(BenchmarkRegistry& reg) {
    // Allocate and compile multiple entities to stress memory
    std::vector<gspl::GsplContext> contexts;
    {
        gspl::benchmark::Timer t("memory_10_concurrent_contexts", reg);
        for (int i = 0; i < 10; ++i) {
            auto& ctx = contexts.emplace_back();
            auto buf = gspl::SourceBuffer::from_string("mem_" + std::to_string(i), kMediumEntity);
            ctx.compile_source(std::move(buf));
        }
    }
}

static void benchmark_regression_detection(BenchmarkRegistry& reg) {
    std::vector<gspl::benchmark::BenchmarkResult> baselines = {
        {"compile_small_entity", 5.0, "ms"},
        {"compile_medium_entity", 20.0, "ms"},
        {"compile_large_entity_100_parts", 100.0, "ms"},
    };

    // First collect real results
    benchmark_compilation_throughput(reg);

    // Then check for regression
    std::printf("  Checking regression against stored baselines...\n");
    reg.regression_against(baselines, 50.0);
}

int main() {
    std::printf("GSPL Performance Benchmarks\n");
    std::printf("===========================\n\n");

    BenchmarkRegistry reg;

    std::printf("--- Compilation Throughput ---\n");
    benchmark_compilation_throughput(reg);

    std::printf("\n--- Memory Usage ---\n");
    benchmark_memory_usage(reg);

    std::printf("\n--- Regression Detection ---\n");
    auto reg_only = BenchmarkRegistry{};
    benchmark_regression_detection(reg_only);

    reg.report();

    // Also add memory tracking benchmark
    {
        auto large = generate_large_module(500);
        gspl::benchmark::Timer t("compile_large_entity_500_parts", reg);
        gspl::GsplContext ctx;
        auto buf = gspl::SourceBuffer::from_string("huge", large);
        ctx.compile_source(std::move(buf));
    }

    reg.report();

    std::printf("\nAll benchmarks completed.\n");
    return 0;
}
