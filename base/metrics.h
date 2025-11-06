//
// Created by wastl on 08.07.25.
//
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/histogram.h>

#ifndef METRICS_H
#define METRICS_H
namespace wastlernet {
namespace metrics {
class WastlernetMetrics {
public:
    // Get the singleton instance
    static WastlernetMetrics& GetInstance() {
        static WastlernetMetrics instance;
        return instance;
    }

    // Prometheus Registry: holds all your metrics
    std::shared_ptr<prometheus::Registry> registry;

    prometheus::Family<prometheus::Counter>& total_queries_family;
    prometheus::Family<prometheus::Counter>& failed_queries_family;
    prometheus::Family<prometheus::Histogram>& query_duration_family;

    prometheus::Counter& solvis_query_counter;
    prometheus::Counter& solvis_error_counter;
    prometheus::Histogram& solvis_duration_ms;

    prometheus::Counter& senec_query_counter;
    prometheus::Counter& senec_error_counter;
    prometheus::Histogram& senec_duration_ms;

    prometheus::Counter& hafnertec_query_counter;
    prometheus::Counter& hafnertec_error_counter;
    prometheus::Histogram& hafnertec_duration_ms;

    prometheus::Counter& fronius_query_counter;
    prometheus::Counter& fronius_error_counter;
    prometheus::Histogram& fronius_duration_ms;

    prometheus::Counter& weather_query_counter;
    prometheus::Counter& weather_error_counter;


private:
    // Private constructor to enforce singleton pattern
    WastlernetMetrics();

    // Prevent copy and assignment
    WastlernetMetrics(const WastlernetMetrics&) = delete;
    WastlernetMetrics& operator=(const WastlernetMetrics&) = delete;
};
}
}
#endif //METRICS_H
