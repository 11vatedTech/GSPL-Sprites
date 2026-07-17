#pragma once

#include "gspl_sprites/model.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stop_token>

namespace gspl::sprites {

enum class JobStatus { queued, running, succeeded, failed, cancelled, dependency_failed, deadline_exceeded };

struct JobResources {
  std::uint64_t ram_bytes{};
  std::uint64_t vram_bytes{};
  std::optional<DeviceBackend> accelerator;
};

struct JobSpec {
  std::string id;
  std::int32_t priority{};
  JobResources resources;
  std::vector<std::string> dependencies;
  std::uint32_t maximum_retries{};
  std::optional<std::chrono::steady_clock::time_point> start_deadline;
  std::function<void(std::stop_token)> execute;
};

struct JobSystemLimits {
  std::uint32_t worker_count{1};
  std::uint32_t maximum_jobs{4096};
  std::uint32_t maximum_events{65536};
  std::uint64_t ram_bytes{};
  std::map<DeviceBackend, std::uint64_t> vram_bytes;
  bool deterministic_scheduling{};
};

struct JobSnapshot {
  std::string id;
  JobStatus status{JobStatus::queued};
  std::uint32_t attempts{};
  std::string error;
};

struct JobEvent {
  std::uint64_t sequence{};
  std::string job_id;
  JobStatus status{JobStatus::queued};
  std::uint32_t attempt{};
  std::string message;
  JobResources resources;
};

struct JobSystemMetrics {
  std::map<JobStatus, std::uint32_t> jobs_by_status;
  std::uint64_t reserved_ram_bytes{};
  std::map<DeviceBackend, std::uint64_t> reserved_vram_bytes;
};

class JobSystem final {
public:
  explicit JobSystem(JobSystemLimits limits);
  ~JobSystem();
  JobSystem(const JobSystem&) = delete;
  JobSystem& operator=(const JobSystem&) = delete;
  JobSystem(JobSystem&&) = delete;
  JobSystem& operator=(JobSystem&&) = delete;

  void submit(JobSpec job);
  [[nodiscard]] bool cancel(std::string_view id);
  [[nodiscard]] JobSnapshot wait(std::string_view id);
  void wait_all();
  [[nodiscard]] JobSnapshot snapshot(std::string_view id) const;
  [[nodiscard]] std::vector<JobEvent> events() const;
  [[nodiscard]] JobSystemMetrics metrics() const;
  void shutdown() noexcept;

private:
  class State;
  std::unique_ptr<State> state_;
};

[[nodiscard]] bool terminal(JobStatus status) noexcept;

} // namespace gspl::sprites
