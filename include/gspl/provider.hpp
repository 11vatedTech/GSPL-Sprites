#pragma once
#include "gspl/diagnostics.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace gspl {

enum class ProviderCapability : std::uint32_t {
    inference = 1,
    deterministic = 2,
    seeded = 4,
    cancellation = 8,
    timeouts = 16,
    device_selection = 32,
    cpu_only = 64,
};

enum class DeviceType : std::uint8_t {
    cpu,
    gpu,
    npu,
    unknown,
};

struct ProviderInfo {
    std::string name;
    std::string version;
    std::vector<ProviderCapability> capabilities;
    DeviceType default_device{DeviceType::cpu};
    bool available{false};
};

struct ProviderInput {
    std::vector<float> data;
    std::vector<std::uint64_t> shape;
    std::string name;
};

struct ProviderOutput {
    std::vector<float> data;
    std::vector<std::uint64_t> shape;
    std::string name;
};

struct ProviderConfig {
    std::string model_path;
    std::uint64_t timeout_ms{30000};
    std::uint64_t max_input_size{1'048'576};
    std::uint64_t max_output_size{1'048'576};
    std::optional<std::uint64_t> seed;
    bool deterministic{false};
    DeviceType preferred_device{DeviceType::cpu};
    bool enable_cancellation{false};
};

class Provider {
public:
    virtual ~Provider() = default;
    virtual ProviderInfo info() const = 0;
    virtual DiagnosticResult initialize(ProviderConfig const& config) = 0;
    virtual DiagnosticResult execute(std::vector<ProviderInput> const& inputs,
                                      std::vector<ProviderOutput>& outputs) = 0;
    virtual bool is_initialized() const = 0;
    virtual void cancel() {}
    virtual DiagnosticResult validate_model() { return {}; }
};

struct ProviderRegistry {
    static ProviderRegistry& instance();
    bool register_provider(std::string name, std::unique_ptr<Provider> provider);
    Provider* lookup(std::string const& name) const;
    std::vector<std::string> available_providers() const;
    void clear();
private:
    ProviderRegistry() = default;
    std::unordered_map<std::string, std::unique_ptr<Provider>> providers_;
};

class NullProvider final : public Provider {
public:
    ProviderInfo info() const override;
    DiagnosticResult initialize(ProviderConfig const& config) override;
    DiagnosticResult execute(std::vector<ProviderInput> const& inputs,
                              std::vector<ProviderOutput>& outputs) override;
    bool is_initialized() const override { return initialized_; }
private:
    bool initialized_{false};
    ProviderConfig config_;
};

class FakeTestProvider final : public Provider {
public:
    explicit FakeTestProvider(std::string name = "test-fake");
    ProviderInfo info() const override;
    DiagnosticResult initialize(ProviderConfig const& config) override;
    DiagnosticResult execute(std::vector<ProviderInput> const& inputs,
                              std::vector<ProviderOutput>& outputs) override;
    bool is_initialized() const override { return initialized_; }
    void set_fail_on_execute(bool fail) { fail_on_execute_ = fail; }
    void set_output_data(std::vector<float> data) { output_data_ = std::move(data); }
private:
    std::string name_;
    bool initialized_{false};
    bool fail_on_execute_{false};
    std::vector<float> output_data_;
};

} // namespace gspl
