#include "gspl_sprites/jobs.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

using namespace gspl::sprites;
using namespace std::chrono_literals;
namespace{void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}JobSpec job(std::string id,std::function<void(std::stop_token)> execute,std::vector<std::string> dependencies={},std::int32_t priority=0,JobResources resources={10,0,std::nullopt},std::uint32_t retries=0){return{std::move(id),priority,resources,std::move(dependencies),retries,std::nullopt,std::move(execute)};}void await(const std::atomic<bool>& value){const auto deadline=std::chrono::steady_clock::now()+2s;while(!value.load()){if(std::chrono::steady_clock::now()>deadline)throw std::runtime_error("timed out waiting for test job");std::this_thread::yield();}}}

int main(){try{
  JobSystemLimits limits{2,128,1024,100,{{DeviceBackend::cuda,50}},false};JobSystem system(limits);
  std::atomic<bool> first_done=false;std::atomic<bool> dependent_saw=false;system.submit(job("first",[&](std::stop_token){first_done=true;}));system.submit(job("dependent",[&](std::stop_token){dependent_saw=first_done.load();},{"first"}));check(system.wait("dependent").status==JobStatus::succeeded&&dependent_saw,"dependency order failed");
  std::atomic<int> retries=0;system.submit(job("retry",[&](std::stop_token){if(++retries==1)throw std::runtime_error("transient");},{},0,{10,0,std::nullopt},1));const auto retried=system.wait("retry");check(retried.status==JobStatus::succeeded&&retried.attempts==2,"retry policy failed");
  std::atomic<bool> should_not_run=false;system.submit(job("failure",[](std::stop_token){throw std::runtime_error("permanent");}));system.submit(job("failed-dependent",[&](std::stop_token){should_not_run=true;},{"failure"}));check(system.wait("failed-dependent").status==JobStatus::dependency_failed&&!should_not_run,"dependency failure did not propagate");
  std::atomic<bool> started=false;system.submit(job("cancel-running",[&](std::stop_token token){started=true;while(!token.stop_requested())std::this_thread::yield();}));await(started);check(system.cancel("cancel-running"),"running cancellation rejected");check(system.wait("cancel-running").status==JobStatus::cancelled,"running job not cancelled");
  auto expired=job("expired",[](std::stop_token){throw std::runtime_error("expired job ran");});expired.start_deadline=std::chrono::steady_clock::now()-1ms;system.submit(std::move(expired));check(system.wait("expired").status==JobStatus::deadline_exceeded,"start deadline ignored");

  std::atomic<int> active=0;std::atomic<int> maximum=0;std::atomic<bool> resource_started=false;std::atomic<bool> resource_release=false;const auto bounded=[&](std::stop_token){resource_started=true;const auto now=++active;int observed=maximum.load();while(now>observed&&!maximum.compare_exchange_weak(observed,now)){}while(!resource_release.load())std::this_thread::yield();--active;};system.submit(job("ram-a",bounded,{},0,{80,0,std::nullopt}));system.submit(job("ram-b",bounded,{},0,{80,0,std::nullopt}));await(resource_started);const auto metrics=system.metrics();check(metrics.reserved_ram_bytes==80&&metrics.jobs_by_status.at(JobStatus::running)==1,"resource metrics are incorrect");resource_release=true;check(system.wait("ram-a").status==JobStatus::succeeded&&system.wait("ram-b").status==JobStatus::succeeded&&maximum==1,"RAM budget allowed unsafe concurrency");
  bool oversized=false;try{system.submit(job("vram-too-large",[](std::stop_token){},{},0,{10,51,DeviceBackend::cuda}));}catch(const std::invalid_argument&){oversized=true;}check(oversized,"VRAM admission limit ignored");
  system.wait_all();const auto events=system.events();check(!events.empty()&&events.size()<=limits.maximum_events,"event log is absent or unbounded");for(std::size_t i=1;i<events.size();++i)check(events[i].sequence>events[i-1].sequence,"event sequence is not monotonic");system.shutdown();

  JobSystem deterministic({4,32,64,100,{},true});std::atomic<bool> release=false;std::atomic<bool> gate_started=false;std::mutex order_mutex;std::vector<std::string> order;
  deterministic.submit(job("gate",[&](std::stop_token){gate_started=true;while(!release.load())std::this_thread::yield();}));await(gate_started);deterministic.submit(job("low",[&](std::stop_token){std::lock_guard lock(order_mutex);order.push_back("low");},{"gate"},1));deterministic.submit(job("high",[&](std::stop_token){std::lock_guard lock(order_mutex);order.push_back("high");},{"gate"},10));release=true;deterministic.wait_all();check(order==std::vector<std::string>({"high","low"}),"deterministic priority order failed");
  std::cout<<"all gspl sprites job tests passed\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
