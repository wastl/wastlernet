//
// Created by wastl on 08.07.25.
//

#include "metrics.h"

wastlernet::metrics::WastlernetMetrics::WastlernetMetrics()
    : registry(std::make_shared<prometheus::Registry>()),
      total_queries_family(prometheus::BuildCounter()
          .Name("wastlernet_queries_success")
          .Help("Number of successful queries to different services.")
          .Register(*registry)),
      failed_queries_family(prometheus::BuildCounter()
          .Name("wastlernet_queries_failed")
          .Help("Number of failed queries to different services.")
          .Register(*registry)),
      query_duration_family(prometheus::BuildHistogram()
          .Name("wastlernet_query_duration")
          .Help("Query duration for queries to different services.")
          .Register(*registry)),
      solvis_query_counter(total_queries_family.Add({{"service", "solvis"}})),
      solvis_error_counter(failed_queries_family.Add({{"service", "solvis"}})),
      solvis_duration_ms(query_duration_family.Add({{"service", "solvis"}},
          prometheus::Histogram::BucketBoundaries{1.0, 5.0, 10.0, 25.0, 50.0, 100.0, 250.0, 500.0, 1000.0, 2500.0, 5000.0, 10000.0})),
      senec_query_counter(total_queries_family.Add({{"service", "senec"}})),
      senec_error_counter(failed_queries_family.Add({{"service", "senec"}})),
      senec_duration_ms(query_duration_family.Add({{"service", "senec"}},
          prometheus::Histogram::BucketBoundaries{1.0, 5.0, 10.0, 25.0, 50.0, 100.0, 250.0, 500.0, 1000.0, 2500.0, 5000.0, 10000.0})),
      hafnertec_query_counter(total_queries_family.Add( {{"service", "hafnertec"}})),
      hafnertec_error_counter(failed_queries_family.Add({{"service", "hafnertec"}})),
      hafnertec_duration_ms(query_duration_family.Add({{"service", "hafnertec"}},
          prometheus::Histogram::BucketBoundaries{1.0, 5.0, 10.0, 25.0, 50.0, 100.0, 250.0, 500.0, 1000.0, 2500.0, 5000.0, 10000.0})),
      fronius_query_counter(total_queries_family.Add({{"service", "fronius"}})),
      fronius_error_counter(failed_queries_family.Add({{"service", "fronius"}})),
      fronius_duration_ms(query_duration_family.Add({{"service", "fronius"}},
          prometheus::Histogram::BucketBoundaries{1.0, 5.0, 10.0, 25.0, 50.0, 100.0, 250.0, 500.0, 1000.0, 2500.0, 5000.0, 10000.0})) {
    prometheus::BuildCounter()
        .Name("wastlernet_app_starts_total")
        .Help("Total application starts.")
        .Register(*registry)
        .Add({{"app", "wastlernet"}})
        .Increment();
}
