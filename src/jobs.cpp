#include "gspl_sprites/jobs.hpp"

#include <algorithm>
#include <cctype>
#include <condition_variable>
#include <exception>
#include <limits>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>

namespace gspl::sprites {

bool terminal(JobStatus status) noexcept {
  return status==JobStatus::succeeded||status==JobStatus::failed||status==JobStatus::cancelled||status==JobStatus::dependency_failed||status==JobStatus::deadline_exceeded;
}

class JobSystem::State final {
public:
  struct Job {
    JobSpec spec;
    JobStatus status{JobStatus::queued};
    std::uint32_t attempts{};
    std::string error;
    std::stop_source cancellation;
  };

  explicit State(JobSystemLimits value):limits(std::move(value)){
    if(limits.worker_count==0||limits.worker_count>64||limits.maximum_jobs==0||limits.maximum_events==0||limits.ram_bytes==0)throw std::invalid_argument("invalid job-system limits");
    if(limits.deterministic_scheduling)limits.worker_count=1;
    workers.reserve(limits.worker_count);for(std::uint32_t i=0;i<limits.worker_count;++i)workers.emplace_back([this](std::stop_token token){worker(token);});
  }

  ~State(){stop();}

  void add_event(const Job& job,std::string message={}){
    if(message.size()>1024)message.resize(1024);
    if(event_log.size()==limits.maximum_events)event_log.erase(event_log.begin());
    event_log.push_back({++event_sequence,job.spec.id,job.status,job.attempts,std::move(message),job.spec.resources});
  }

  bool propagate_locked(){
    const auto now=std::chrono::steady_clock::now();bool changed=true;
    bool any=false;while(changed){changed=false;for(auto&[id,job]:jobs){static_cast<void>(id);if(job.status!=JobStatus::queued)continue;if(job.spec.start_deadline&&now>=*job.spec.start_deadline){job.status=JobStatus::deadline_exceeded;job.error="start deadline exceeded";add_event(job,job.error);changed=true;any=true;continue;}for(const auto&dependency_id:job.spec.dependencies){const auto&dependency=jobs.at(dependency_id);if(terminal(dependency.status)&&dependency.status!=JobStatus::succeeded){job.status=JobStatus::dependency_failed;job.error="dependency did not succeed: "+dependency_id;add_event(job,job.error);changed=true;any=true;break;}}}}
    return any;
  }

  bool resources_fit(const Job& job)const{
    if(job.spec.resources.ram_bytes>limits.ram_bytes-running_ram)return false;
    if(job.spec.resources.vram_bytes==0)return true;
    if(!job.spec.resources.accelerator)return false;
    const auto capacity=limits.vram_bytes.find(*job.spec.resources.accelerator);if(capacity==limits.vram_bytes.end())return false;
    const auto used=running_vram.find(*job.spec.resources.accelerator);const auto consumed=used==running_vram.end()?0:used->second;
    return job.spec.resources.vram_bytes<=capacity->second-consumed;
  }

  bool dependencies_succeeded(const Job& job)const{return std::ranges::all_of(job.spec.dependencies,[&](const auto&id){return jobs.at(id).status==JobStatus::succeeded;});}

  Job* select_locked(){
    Job* selected=nullptr;
    for(auto&[id,job]:jobs){static_cast<void>(id);if(job.status!=JobStatus::queued||!dependencies_succeeded(job)||!resources_fit(job))continue;if(selected==nullptr||job.spec.priority>selected->spec.priority||(job.spec.priority==selected->spec.priority&&job.spec.id<selected->spec.id))selected=&job;}
    return selected;
  }

  void allocate(const Job& job){running_ram+=job.spec.resources.ram_bytes;if(job.spec.resources.vram_bytes>0)running_vram[*job.spec.resources.accelerator]+=job.spec.resources.vram_bytes;}
  void release(const Job& job){running_ram-=job.spec.resources.ram_bytes;if(job.spec.resources.vram_bytes>0)running_vram[*job.spec.resources.accelerator]-=job.spec.resources.vram_bytes;}

  void worker(std::stop_token worker_stop){
    for(;;){
      Job* selected=nullptr;
      {
        std::unique_lock lock(mutex);
        for(;;){
          if(propagate_locked())condition.notify_all();
          if(worker_stop.stop_requested()||shutting_down)return;
          selected=select_locked();if(selected!=nullptr)break;
          std::optional<std::chrono::steady_clock::time_point> earliest;
          for(const auto&[id,job]:jobs){static_cast<void>(id);if(job.status==JobStatus::queued&&job.spec.start_deadline&&(!earliest||*job.spec.start_deadline<*earliest))earliest=job.spec.start_deadline;}
          if(earliest)condition.wait_until(lock,*earliest);else condition.wait(lock);
        }
        selected->status=JobStatus::running;++selected->attempts;allocate(*selected);add_event(*selected);condition.notify_all();
      }
      std::string failure;try{selected->spec.execute(selected->cancellation.get_token());}catch(const std::exception&error){failure=error.what();}catch(...){failure="unknown job exception";}
      {
        std::lock_guard lock(mutex);release(*selected);
        if(selected->cancellation.stop_requested()||shutting_down){selected->status=JobStatus::cancelled;selected->error="cancelled";add_event(*selected,selected->error);}
        else if(!failure.empty()){
          selected->error=failure;
          if(selected->attempts<=selected->spec.maximum_retries){selected->status=JobStatus::queued;selected->cancellation=std::stop_source{};add_event(*selected,"retry after failure: "+failure);}
          else{selected->status=JobStatus::failed;add_event(*selected,failure);}
        }else{selected->status=JobStatus::succeeded;selected->error.clear();add_event(*selected);}
        propagate_locked();condition.notify_all();
      }
    }
  }

  void stop()noexcept{
    std::vector<std::jthread> joining;
    {std::lock_guard lock(mutex);if(stopped)return;stopped=true;shutting_down=true;for(auto&[id,job]:jobs){static_cast<void>(id);job.cancellation.request_stop();if(job.status==JobStatus::queued){job.status=JobStatus::cancelled;job.error="job system shutdown";add_event(job,job.error);}}joining.swap(workers);condition.notify_all();}
    for(auto&worker:joining)worker.request_stop();
    condition.notify_all();
  }

  JobSystemLimits limits;
  mutable std::mutex mutex;
  std::condition_variable_any condition;
  std::map<std::string,Job,std::less<>> jobs;
  std::vector<std::jthread> workers;
  std::vector<JobEvent> event_log;
  std::uint64_t event_sequence{};
  std::uint64_t running_ram{};
  std::map<DeviceBackend,std::uint64_t> running_vram;
  bool shutting_down{};
  bool stopped{};
};

JobSystem::JobSystem(JobSystemLimits limits):state_(std::make_unique<State>(std::move(limits))){}
JobSystem::~JobSystem(){shutdown();}

void JobSystem::submit(JobSpec job){
  if(job.id.empty()||job.id.size()>128||!std::ranges::all_of(job.id,[](unsigned char c){return std::isalnum(c)!=0||c=='.'||c=='_'||c=='-';})||!job.execute||job.maximum_retries>16)throw std::invalid_argument("invalid job specification");
  std::ranges::sort(job.dependencies);if(std::ranges::adjacent_find(job.dependencies)!=job.dependencies.end()||std::ranges::find(job.dependencies,job.id)!=job.dependencies.end())throw std::invalid_argument("duplicate or self job dependency");
  std::lock_guard lock(state_->mutex);if(state_->shutting_down)throw std::runtime_error("job system is shutting down");if(state_->jobs.size()>=state_->limits.maximum_jobs)throw std::runtime_error("job count exceeds limit");if(state_->jobs.contains(job.id))throw std::invalid_argument("duplicate job id");for(const auto&dependency:job.dependencies)if(!state_->jobs.contains(dependency))throw std::invalid_argument("job dependency must already exist: "+dependency);
  if(job.resources.ram_bytes>state_->limits.ram_bytes)throw std::invalid_argument("job RAM requirement exceeds system budget");
  if(job.resources.vram_bytes>0){
    if(!job.resources.accelerator)throw std::invalid_argument("VRAM job requires accelerator affinity");
    const auto capacity=state_->limits.vram_bytes.find(*job.resources.accelerator);
    if(capacity==state_->limits.vram_bytes.end()||job.resources.vram_bytes>capacity->second)throw std::invalid_argument("job VRAM requirement exceeds device budget");
  }
  const auto id=job.id;
  auto[found,inserted]=state_->jobs.emplace(id,State::Job{std::move(job), {}, {}, {}, {}});
  static_cast<void>(inserted);
  state_->add_event(found->second);
  state_->condition.notify_all();
}

bool JobSystem::cancel(std::string_view id){std::lock_guard lock(state_->mutex);const auto found=state_->jobs.find(id);if(found==state_->jobs.end())throw std::invalid_argument("unknown job id");auto&job=found->second;if(terminal(job.status))return false;job.cancellation.request_stop();if(job.status==JobStatus::queued){job.status=JobStatus::cancelled;job.error="cancelled before execution";state_->add_event(job,job.error);state_->propagate_locked();}state_->condition.notify_all();return true;}

JobSnapshot JobSystem::wait(std::string_view id){std::unique_lock lock(state_->mutex);const auto found=state_->jobs.find(id);if(found==state_->jobs.end())throw std::invalid_argument("unknown job id");state_->condition.wait(lock,[&]{return terminal(found->second.status);});return{found->second.spec.id,found->second.status,found->second.attempts,found->second.error};}
void JobSystem::wait_all(){std::unique_lock lock(state_->mutex);state_->condition.wait(lock,[&]{return std::ranges::all_of(state_->jobs,[](const auto&item){return terminal(item.second.status);});});}
JobSnapshot JobSystem::snapshot(std::string_view id)const{std::lock_guard lock(state_->mutex);const auto found=state_->jobs.find(id);if(found==state_->jobs.end())throw std::invalid_argument("unknown job id");return{found->second.spec.id,found->second.status,found->second.attempts,found->second.error};}
std::vector<JobEvent> JobSystem::events()const{std::lock_guard lock(state_->mutex);return state_->event_log;}
JobSystemMetrics JobSystem::metrics()const{std::lock_guard lock(state_->mutex);JobSystemMetrics result;for(const auto&[id,job]:state_->jobs){static_cast<void>(id);++result.jobs_by_status[job.status];}result.reserved_ram_bytes=state_->running_ram;result.reserved_vram_bytes=state_->running_vram;return result;}
void JobSystem::shutdown()noexcept{if(state_)state_->stop();}
} // namespace gspl::sprites
