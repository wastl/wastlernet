/// \file module.h
/// \brief Base interfaces and helpers for WastlerNet modules.
///
/// This header defines the generic module API used throughout WastlerNet and
/// two base template classes that help implementing concrete modules which
/// write time-series data to TimescaleDB.
///
/// Contents:
/// - `IModule`: a minimal lifecycle interface for all modules
/// - `Module<Data>`: a helper providing DB connectivity and a cached state
/// - `PollingModule<Data>`: a periodic polling loop built on top of `Module`
///
/// Notes on thread-safety
/// - Unless stated otherwise, classes in this header are not thread-safe for
///   concurrent Start/Abort/Wait calls. Call from a single controlling thread.
/// - `PollingModule` runs its worker in a dedicated thread created by `Start()`.
///
/// State cache
/// Each module may publish its latest serialized protobuf payload into a shared
/// `StateCache`, keyed by the module `Name()`. This is intended for lightweight
/// in-memory introspection and should not be treated as a durable store.
///
/// Created by wastl on 07.04.23. Updated on 2025-11-06 12:51.
#pragma once
#include <chrono>
#include <thread>
#include <type_traits>
#include <functional>   // for std::function used in PollingModule::Query
#include <absl/status/status.h>
#include <absl/container/flat_hash_map.h>

#include "config/config.pb.h"
#include "timescaledb/timescaledb-client.h"

#ifndef WASTLERNET_MODULE_H
#define WASTLERNET_MODULE_H
namespace wastlernet {
    /// Alias for a shared in-memory state cache used by modules to expose their
    /// latest serialized protobuf payload (value) keyed by module name (key).
    typedef absl::flat_hash_map<std::string, std::string> StateCache;

    /// Minimal lifecycle interface implemented by all modules.
    class IModule {
    public:
        virtual ~IModule() = default;

        /// Perform any setup required before `Start()`. May establish network
        /// connections, validate configuration, or create database schema.
        /// Implementations should be idempotent.
        ///
        /// Returns: OK on success; a descriptive error otherwise.
        virtual absl::Status Init() = 0;

        /// Start the module's work. Implementations should return quickly and
        /// perform work asynchronously (e.g., by spawning threads) rather than
        /// blocking. It is valid to call `Wait()` right after `Start()`.
        virtual void Start() = 0;

        /// Request the module to stop as soon as practical. Implementations
        /// should be cooperative, set internal flags, and avoid long blocking
        /// operations here. It is allowed to call `Abort()` multiple times.
        virtual void Abort() = 0;

        /// Block the caller until the module has fully stopped. Typically this
        /// joins any worker threads started by `Start()`.
        virtual void Wait() = 0;

        /// A stable, human-readable name identifying the module. This is used
        /// as the key in the shared `StateCache`.
        virtual std::string Name() = 0;
    };

    /// Base helper for modules that publish protobuf `Data` to TimescaleDB.
    ///
    /// Template parameter:
    /// - Data: a protobuf message type providing `SerializeAsString()` and a
    ///   corresponding `timescaledb::TimescaleWriter<Data>` implementation.
    template<class Data>
    class Module : public IModule {
    protected:
        /// Connection wrapper handling schema and batched writes.
        timescaledb::TimescaleConnection<Data> conn_;

        /// Not owned. Shared pointer to the global state cache that receives
        /// the module's latest serialized payload under `Name()`.
        StateCache* current_state_;

        /// Write a single `data` sample to the state cache and TimescaleDB.
        ///
        /// Thread-safety: Not thread-safe for concurrent callers; `PollingModule`
        /// ensures single-writer semantics.
        virtual absl::Status Update(const Data& data) {
            (*current_state_)[Name()] = data.SerializeAsString();
            return conn_.Update(data);
        }

    public:
        /// Construct the module helper.
        ///
        /// Parameters:
        /// - config: TimescaleDB connection configuration (host, port, ...)
        /// - writer: Timescale writer for `Data` (not owned)
        /// - current_state: shared state cache (not owned)
        Module(const TimescaleDB& config, timescaledb::TimescaleWriter<Data>* writer, StateCache* current_state)
                : conn_(writer, config.database(), config.host(), config.port(), config.user(), config.password()), current_state_(current_state) {
        }

        virtual ~Module() = default;

        /// Initialize the underlying Timescale connection/writer.
        /// Safe to call multiple times; subsequent calls are no-ops if already
        /// initialized.
        virtual absl::Status Init() {
            return conn_.Init();
        }
    };

    /// A convenience base class for modules that need to poll external systems
    /// at a fixed interval and write results to the database.
    ///
    /// Behavior:
    /// - `Start()` spawns a background thread that wakes up every
    ///   `poll_interval` seconds (wall-clock) and performs `Query()`.
    /// - For each polled sample, `Query()` should invoke the provided handler
    ///   with a `Data` instance to store; the default handler writes to the DB
    ///   and updates the shared `StateCache` via `Module::Update()`.
    /// - `Abort()` requests the loop to stop and then joins the thread.
    /// - `Wait()` simply joins the worker thread.
    ///
    /// Time semantics: The wake-up schedule uses `std::chrono::system_clock`
    /// and accumulates the next tick by adding `poll_interval` each iteration,
    /// which helps avoid drift but may skip cycles after long pauses.
    template<class Data>
    class PollingModule : public Module<Data> {
    private:
        /// Polling period in seconds.
        int poll_interval;
        /// Set to true by `Abort()` to stop the worker loop.
        bool aborted_;
        /// Worker thread running the polling loop.
        std::thread t_;

    protected:
        /// Implemented by concrete modules to fetch new data and deliver it to
        /// the supplied `handler` once per sample. The typical implementation is
        /// to obtain one `Data` object and call `handler(data)`; the handler will
        /// then write to the DB and update the state cache.
        ///
        /// Return OK on success; any error is logged by the caller.
        virtual absl::Status Query(std::function<absl::Status(const Data& data)> handler) = 0;

    public:
        /// Construct a polling module.
        /// - `config`: TimescaleDB connection settings
        /// - `writer`: `TimescaleWriter<Data>` (not owned)
        /// - `current_state`: shared cache for the latest sample (not owned)
        /// - `poll_interval`: period in seconds between polls
        PollingModule(const TimescaleDB& config, timescaledb::TimescaleWriter<Data>* writer, StateCache* current_state, int poll_interval)
        : Module<Data>(config, writer, current_state), poll_interval(poll_interval), aborted_(false) { }

        /// Start the background polling thread.
        void Start() override {
            t_ = std::thread([this]() {
                auto next = std::chrono::system_clock::now();
                while (!aborted_) {
                    next += std::chrono::seconds(poll_interval);
                    std::this_thread::sleep_until(next);
                    auto st = Query([this](const Data& data){
                        auto st = Module<Data>::Update(data);
                        if (!st.ok()) {
                            LOG(ERROR) << "Error writing to database: " << st;
                        }
                        return st;
                    });
                    if (!st.ok()) {
                        LOG(ERROR) << "Error: " << st;
                    }
                }
            });
        }

        /// Ask the polling loop to stop and wait until it exits.
        void Abort() override {
            aborted_ = true;
            t_.join();
        }

        /// Wait until the polling thread has finished.
        void Wait() override {
            t_.join();
        }
    };
}
#endif //WASTLERNET_MODULE_H
