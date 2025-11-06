//
// Created by wastl on 08.07.25.
//

#include "metrics.h"

namespace wastlernet { namespace metrics {

WastlernetMetrics::WastlernetMetrics()
    : registry_(std::make_shared<prometheus::Registry>()) {
  // Initialize families
  queries_total_family_ = &prometheus::BuildCounter()
      .Name("wastlernet_queries_total")
      .Help("Number of queries to different services, labeled by result (ok/error).")
      .Register(*registry_);

  query_latency_seconds_family_ = &prometheus::BuildHistogram()
      .Name("wastlernet_query_latency_seconds")
      .Help("Query latency for different services in seconds.")
      .Register(*registry_);

  // Increment app starts gauge/counter
  prometheus::BuildCounter()
      .Name("wastlernet_app_starts_total")
      .Help("Total application starts.")
      .Register(*registry_)
      .Add({{"app", "wastlernet"}})
      .Increment();
}

void WastlernetMetrics::StartExposer(const std::string& bind_address) {
  if (exposer_) return; // already started
  exposer_ = std::make_unique<prometheus::Exposer>(bind_address);
  exposer_->RegisterCollectable(registry_);
}

std::shared_ptr<prometheus::Registry> WastlernetMetrics::registry() { return registry_; }

void WastlernetMetrics::SetBuckets(const BucketsConfig& cfg) {
  absl::MutexLock lock(&mu_);
  buckets_ = cfg;
  // Note: existing histograms keep their original buckets (prometheus rules). New services will use new buckets.
}

void WastlernetMetrics::ExportBuildInfo(const std::string& version,
                                        const std::string& git_sha,
                                        const std::string& build_time) {
  // Simple info gauge set to 1 with build labels
  auto& family = prometheus::BuildGauge()
      .Name("wastlernet_build_info")
      .Help("Build info; constant 1 labeled with version/git/build_time")
      .Register(*registry_);
  family.Add({{"version", version}, {"git_sha", git_sha}, {"build_time", build_time}}).Set(1);
}

WastlernetMetrics::QueryChildren& WastlernetMetrics::GetOrCreateChildren(const std::string& service) {
  absl::MutexLock lock(&mu_);
  auto it = by_service_.find(service);
  if (it != by_service_.end()) return it->second;

  QueryChildren children;
  children.ok_counter = &queries_total_family_->Add({{"service", service}, {"result", "ok"}});
  children.error_counter = &queries_total_family_->Add({{"service", service}, {"result", "error"}});
  children.latency = &query_latency_seconds_family_->Add({{"service", service}}, buckets_.latency_seconds);

  auto [insert_it, _] = by_service_.emplace(service, children);
  return insert_it->second;
}

void WastlernetMetrics::RecordQueryResult(const std::string& service, bool ok) {
  auto& c = GetOrCreateChildren(service);
  if (ok) c.ok_counter->Increment(); else c.error_counter->Increment();
}

void WastlernetMetrics::ObserveQueryLatency(const std::string& service, double seconds) {
  auto& c = GetOrCreateChildren(service);
  c.latency->Observe(seconds);
}

WastlernetMetrics::ScopedQueryTimer::~ScopedQueryTimer() {
  const auto end = Clock::now();
  const double seconds = std::chrono::duration<double>(end - start_).count();
  mx_.ObserveQueryLatency(service_, seconds);
  if (ok_.has_value()) mx_.RecordQueryResult(service_, *ok_);
}

}} // namespace
