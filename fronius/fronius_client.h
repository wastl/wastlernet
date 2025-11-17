//
// Created by wastl on 05.11.25.
//
// This header declares simple HTTP clients for querying a Fronius Solar API.
// It provides two concrete clients to retrieve realtime power flow and
// battery (storage) information and parse responses into protobuf messages.
//
// See also:
//   - fronius/fronius_client.cpp for the implementation details
//   - fronius/fronius.proto for protobuf message definitions
//
/**
 * @file fronius_client.h
 * @brief Lightweight clients for the Fronius Solar API.
 *
 * The classes in this file are thin wrappers around HttpConnection that
 * perform GET requests against the Fronius local API endpoints, parse the
 * returned JSON and expose the essential data through a callback provided
 * by the caller. Errors are reported using absl::Status.
 */
#pragma once

#include <functional>
#include <absl/status/status.h>

#include "base/http_connection.h"
#include "fronius/fronius.pb.h"

#ifndef WASTLERNET_FRONIUS_CLIENT_H
#define WASTLERNET_FRONIUS_CLIENT_H

namespace fronius {

    /**
     * @brief Base class for Fronius API clients.
     *
     * This class specializes wastlernet::HttpConnection for simple GET
     * requests against the local Fronius API. Derived classes provide the
     * concrete endpoint path and parsing logic.
     */
    class FroniusBaseClient : public wastlernet::HttpConnection {
    public:
        /**
         * @brief Construct a FroniusBaseClient.
         * @param base_url Base URL to the Fronius device, e.g. "http://192.168.1.10".
         * @param path Endpoint path to query on the device.
         */
        explicit FroniusBaseClient(const std::string &base_url, const std::string &path)
            : HttpConnection(base_url, path, GET) {}

    protected:
        /**
         * @brief Provide HTTP client configuration for cpprestsdk.
         *
         * The configuration typically sets reasonable timeouts and may relax
         * certificate checks for local connections if necessary.
         */
        web::http::client::http_client_config ClientConfig() override;
    };

    /**
     * @brief Retrieve live power flow information.
     *
     * Endpoint: `/solar_api/v1/GetPowerFlowRealtimeData.fcgi`
     * JSON root: `Body.Data.Site`
     *
     * The response is parsed into protobuf messages:
     *   - `Leistung` (power metrics)
     *   - `Quellen`  (sources/flows)
     */
    class FroniusPowerFlowClient : public FroniusBaseClient {
    public:
        /**
         * @brief Construct a client for the realtime power flow endpoint.
         * @param base_url Base URL of the Fronius device.
         */
        explicit FroniusPowerFlowClient(const std::string &base_url)
            : FroniusBaseClient(base_url, "/solar_api/v1/GetPowerFlowRealtimeData.fcgi") {}

        /**
         * @brief Query the device and deliver parsed data via callback.
         *
         * Performs a GET request to the configured endpoint and, on success,
         * calls `handler` with the parsed `Leistung` and `Quellen` messages.
         *
         * @param handler Callback invoked on successful parsing.
         *                It receives references to `Leistung` and `Quellen`.
         * @return `absl::OkStatus()` on success; appropriate error otherwise
         *         (e.g., network/HTTP errors, parse errors, missing fields).
         */
        absl::Status Query(const std::function<void(const Leistung&, const Quellen&)> &handler);

    protected:
        /** @brief Human-readable client name for logging/debugging. */
        std::string Name() override { return "FroniusPowerFlowClient"; }

    };


    /**
     * @brief Retrieve live battery (storage) information.
     *
     * Endpoint: `/solar_api/v1/GetStorageRealtimeData.cgi`
     * JSON path: `Body.Data.0.Controller`
     *
     * The response is parsed into the protobuf message `Batterie`.
     */
    class FroniusBatteryClient : public FroniusBaseClient {
    public:
        /**
         * @brief Construct a client for the realtime storage endpoint.
         * @param base_url Base URL of the Fronius device.
         */
        explicit FroniusBatteryClient(const std::string &base_url)
            : FroniusBaseClient(base_url, "/solar_api/v1/GetStorageRealtimeData.cgi") {}

        /**
         * @brief Query the device and deliver parsed battery data via callback.
         *
         * Performs a GET request to the storage endpoint and, on success,
         * calls `handler` with the parsed `Batterie` message.
         *
         * @param handler Callback invoked on successful parsing.
         * @return `absl::OkStatus()` on success; an error status otherwise.
         */
        absl::Status Query(const std::function<void(const Batterie&)> &handler);

    protected:
        /** @brief Human-readable client name for logging/debugging. */
        std::string Name() override { return "FroniusBatteryClient"; }
   };

    /**
     * @brief Retrieve live energy meter information and compute house consumption.
     *
     * Endpoint: `/solar_api/v1/GetMeterRealtimeData.cgi`
     * JSON path: `Body.Data.0`
     *
     * The client computes total house consumption in Watts as the sum of the
     * positive per-phase real powers: `sum(max(0, PowerReal_P_Phase_i))`.
     * If per-phase fields are missing, it falls back to `max(0, PowerReal_P_Sum)`.
     */
    class FroniusEnergyMeterClient : public FroniusBaseClient {
    public:
        explicit FroniusEnergyMeterClient(const std::string &base_url)
            : FroniusBaseClient(base_url, "/solar_api/v1/GetMeterRealtimeData.cgi") {}

        /**
         * @brief Query the device and deliver computed consumption via callback.
         * @param handler Callback receiving total house consumption in Watts.
         */
        absl::Status Query(const std::function<void(double consumption_watts)> &handler);

    protected:
        std::string Name() override { return "FroniusEnergyMeterClient"; }
    };
}

#endif // WASTLERNET_FRONIUS_CLIENT_H
