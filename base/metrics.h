//
// Created by wastl on 08.07.25.
//
#pragma once
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/histogram.h>
#include <prometheus/gauge.h>
#include <absl/synchronization/mutex.h>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef METRICS_H
#define METRICS_H

namespace wastlernet::metrics {

struct BucketsConfig {
    // Prometheus base unit: seconds
    std::vector<double> latency_seconds{0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10};
};

class WastlernetMetrics {
public:
    // Get the singleton instance
    static WastlernetMetrics& GetInstance() {
        static WastlernetMetrics instance;
        return instance;
    }

    // Prevent copy and assignment
    WastlernetMetrics(const WastlernetMetrics&) = delete;
    WastlernetMetrics& operator=(const WastlernetMetrics&) = delete;

    // Optional: start /metrics endpoint (can also be done in main)
    void StartExposer(const std::string& bind_address = "0.0.0.0:9090");

    // Record a query outcome (increments counter with result label)
    void RecordQueryResult(const std::string& service, bool ok);

    // Observe a query latency in seconds (histogram)
    void ObserveQueryLatency(const std::string& service, double seconds);

    // Record that we received an update from a specific device under a service
    // Exposes Prometheus counter: wastlernet_device_updates_total{service="...", device="..."}
    void RecordDeviceUpdate(const std::string& service, const std::string& device);

    // RAII helper to time a scope and optionally record result on destruction
    class ScopedQueryTimer {
    public:
        explicit ScopedQueryTimer(WastlernetMetrics& mx, std::string service)
            : mx_(mx), service_(std::move(service)), start_(Clock::now()) {}
        void SetResult(bool ok) { ok_ = ok; }
        ~ScopedQueryTimer();
    private:
        using Clock = std::chrono::steady_clock;
        WastlernetMetrics& mx_;
        std::string service_;
        Clock::time_point start_;
        std::optional<bool> ok_;
        friend class WastlernetMetrics;
    };

    // Config
    void SetBuckets(const BucketsConfig& cfg);

    // Optional: build info gauge set to 1 with version labels
    void ExportBuildInfo(const std::string& version,
                         const std::string& git_sha,
                         const std::string& build_time);

    // Access registry for external exposer registration
    std::shared_ptr<prometheus::Registry> registry();

private:
    // Private constructor to enforce singleton pattern
    WastlernetMetrics();

    // Internal helpers
    struct QueryChildren {
        prometheus::Counter* ok_counter = nullptr;    // queries_total{result="ok"}
        prometheus::Counter* error_counter = nullptr; // queries_total{result="error"}
        prometheus::Histogram* latency = nullptr;     // query_latency_seconds
    };

    QueryChildren& GetOrCreateChildren(const std::string& service);

    absl::Mutex mu_;
    std::unordered_map<std::string, QueryChildren> by_service_ ABSL_GUARDED_BY(mu_);

    std::shared_ptr<prometheus::Registry> registry_;
    std::unique_ptr<prometheus::Exposer> exposer_;
    BucketsConfig buckets_;

    prometheus::Family<prometheus::Counter>* queries_total_family_; // labels: service, result
    prometheus::Family<prometheus::Histogram>* query_latency_seconds_family_; // label: service
    prometheus::Family<prometheus::Counter>* device_updates_total_family_; // labels: service, device

    // Cache for device counters to avoid duplicate Add() calls with same labels
    std::unordered_map<std::string, prometheus::Counter*> device_updates_counters_ ABSL_GUARDED_BY(mu_);
};
}

#endif //METRICS_H
