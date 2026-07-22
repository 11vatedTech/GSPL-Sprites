#include "gspl/provider.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace { void check(bool value, const char* msg) { if (!value) throw std::runtime_error(msg); } }

int main() {
    try {
        // ---- 1. NullProvider ----
        {
            gspl::NullProvider null_provider;
            auto info = null_provider.info();
            check(info.name == "null", "NullProvider name");
            check(info.available, "NullProvider should be available");

            gspl::ProviderConfig cfg;
            auto result = null_provider.initialize(cfg);
            check(result.ok(), "NullProvider init should succeed");
            check(null_provider.is_initialized(), "NullProvider should report initialized");
        }

        // ---- 2. NullProvider execution ----
        {
            gspl::NullProvider null_provider;
            gspl::ProviderConfig cfg;
            null_provider.initialize(cfg);

            gspl::ProviderInput inp;
            inp.name = "input";
            inp.data = {1.0f, 2.0f, 3.0f};
            inp.shape = {1, 3};

            std::vector<gspl::ProviderOutput> outputs;
            auto result = null_provider.execute({inp}, outputs);
            check(result.ok(), "NullProvider execute should succeed");
            check(outputs.size() == 1, "Should produce 1 output");
            check(outputs[0].name == "input_out", "Output name should be derived");
        }

        // ---- 3. FakeTestProvider ----
        {
            gspl::FakeTestProvider fake("test-fake");
            auto info = fake.info();
            check(info.name == "test-fake", "FakeTestProvider name");

            gspl::ProviderConfig cfg;
            cfg.deterministic = true;
            auto result = fake.initialize(cfg);
            check(result.ok(), "FakeTestProvider init should succeed");
        }

        // ---- 4. FakeTestProvider execution with custom output ----
        {
            gspl::FakeTestProvider fake("custom");
            fake.initialize({});
            fake.set_output_data({42.0f, 43.0f});

            gspl::ProviderInput inp;
            inp.name = "x";
            inp.data = {1.0f};
            inp.shape = {1, 1};

            std::vector<gspl::ProviderOutput> outputs;
            auto result = fake.execute({inp}, outputs);
            check(result.ok(), "Custom data execution should succeed");
            check(outputs[0].data.size() == 2, "Should have 2 output values");
            check(outputs[0].data[0] == 42.0f, "First output should be 42.0");
        }

        // ---- 5. FakeTestProvider failure mode ----
        {
            gspl::FakeTestProvider fake("fail-provider");
            fake.initialize({});
            fake.set_fail_on_execute(true);

            gspl::ProviderInput inp;
            inp.name = "x";
            inp.data = {1.0f};
            inp.shape = {1};

            std::vector<gspl::ProviderOutput> outputs;
            auto result = fake.execute({inp}, outputs);
            bool has_err = false;
            for (auto const& d : result.diagnostics) has_err = true;
            check(has_err, "Failing provider should produce diagnostic");
        }

        // ---- 6. ProviderRegistry ----
        {
            auto& reg = gspl::ProviderRegistry::instance();
            reg.clear();

            bool ok = reg.register_provider("null", std::make_unique<gspl::NullProvider>());
            check(ok, "NullProvider registration should succeed");

            ok = reg.register_provider("test", std::make_unique<gspl::FakeTestProvider>("test"));
            check(ok, "FakeTestProvider registration should succeed");

            auto providers = reg.available_providers();
            check(providers.size() >= 2, "Should have at least 2 providers");
        }

        // ---- 7. ProviderRegistry lookup ----
        {
            auto& reg = gspl::ProviderRegistry::instance();
            auto* null_prov = reg.lookup("null");
            check(null_prov != nullptr, "NullProvider should be findable");
            check(null_prov->info().name == "null", "Lookup should return correct provider");

            auto* missing = reg.lookup("nonexistent");
            check(missing == nullptr, "Missing provider should return nullptr");
        }

        // ---- 8. Provider initialization without calling init ----
        {
            gspl::NullProvider null_provider;
            check(!null_provider.is_initialized(), "Provider should not be initialized before init()");
        }

        std::cout << "ALL PROVIDER TESTS PASSED\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}
