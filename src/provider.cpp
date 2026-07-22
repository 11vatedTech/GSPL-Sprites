#include "gspl/provider.hpp"
#include <algorithm>
#include <chrono>
#include <thread>

namespace gspl {

ProviderRegistry& ProviderRegistry::instance() {
    static ProviderRegistry reg;
    return reg;
}

bool ProviderRegistry::register_provider(std::string name, std::unique_ptr<Provider> provider) {
    if (!provider) return false;
    auto [it, inserted] = providers_.try_emplace(std::move(name), std::move(provider));
    return inserted;
}

Provider* ProviderRegistry::lookup(std::string const& name) const {
    auto it = providers_.find(name);
    return it != providers_.end() ? it->second.get() : nullptr;
}

std::vector<std::string> ProviderRegistry::available_providers() const {
    std::vector<std::string> names;
    for (auto const& [name, _] : providers_) names.push_back(name);
    std::sort(names.begin(), names.end());
    return names;
}

void ProviderRegistry::clear() {
    providers_.clear();
}

// ---- NullProvider ----
ProviderInfo NullProvider::info() const {
    return {"null", "0.1.0", {ProviderCapability::cpu_only}, DeviceType::cpu, true};
}

DiagnosticResult NullProvider::initialize(ProviderConfig const& config) {
    config_ = config;
    initialized_ = true;
    return {};
}

DiagnosticResult NullProvider::execute(std::vector<ProviderInput> const& inputs,
                                        std::vector<ProviderOutput>& outputs) {
    if (!initialized_) {
        DiagnosticResult dr;
        dr.add_error(DiagnosticCode::GSPL_RESOURCE_EXCEEDED, "NullProvider not initialized", {});
        return dr;
    }
    for (auto const& inp : inputs) {
        ProviderOutput out;
        out.name = inp.name + "_out";
        out.shape = inp.shape;
        out.data.resize(inp.data.size(), 0.0f);
        outputs.push_back(std::move(out));
    }
    return {};
}

// ---- FakeTestProvider ----
FakeTestProvider::FakeTestProvider(std::string name) : name_(std::move(name)) {}

ProviderInfo FakeTestProvider::info() const {
    return {name_, "1.0.0",
            {ProviderCapability::inference, ProviderCapability::deterministic,
             ProviderCapability::seeded, ProviderCapability::cpu_only},
            DeviceType::cpu, true};
}

DiagnosticResult FakeTestProvider::initialize(ProviderConfig const&) {
    initialized_ = true;
    return {};
}

DiagnosticResult FakeTestProvider::execute(std::vector<ProviderInput> const& inputs,
                                            std::vector<ProviderOutput>& outputs) {
    if (!initialized_) {
        DiagnosticResult dr;
        dr.add_error(DiagnosticCode::GSPL_RESOURCE_EXCEEDED, "FakeTestProvider not initialized", {});
        return dr;
    }
    if (fail_on_execute_) {
        DiagnosticResult dr;
        dr.add_error(DiagnosticCode::GSPL_RESOURCE_EXCEEDED, "FakeTestProvider forced failure", {});
        return dr;
    }
    for (auto const& inp : inputs) {
        ProviderOutput out;
        out.name = inp.name + "_out";
        out.shape = inp.shape;
        if (!output_data_.empty()) {
            out.data = output_data_;
        } else {
            out.data.resize(inp.data.size(), 1.0f);
        }
        outputs.push_back(std::move(out));
    }
    return {};
}

} // namespace gspl
