// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <json.hpp>
#include <string>
#include <config/mevacoin_config.h>
#include <logging/ilogger.h>

using nlohmann::json;

namespace payment_service
{
    struct WalletServiceConfiguration
    {
        WalletServiceConfiguration()
        {
            daemonAddress = "127.0.0.1";
            bindAddress = "127.0.0.1";
            logFile = "service.log";
            daemonPort = mevacoin::RPC_DEFAULT_PORT;
            bindPort = mevacoin::SERVICE_DEFAULT_PORT;
            logLevel = logging::INFO;
            legacySecurity = false;
            help = false;
            version = false;
            dumpConfig = false;
            generateNewContainer = false;
            daemonize = false;
            registerService = false;
            unregisterService = false;
            printAddresses = false;
            syncFromZero = false;
            initTimeout = 10;
        }

        std::string daemonAddress;
        std::string bindAddress;
        std::string rpcPassword;
        std::string containerFile;
        std::string containerPassword;
        std::string serverRoot;
        std::string corsHeader;
        std::string logFile;

        int daemonPort;
        int bindPort;
        int logLevel;
        int initTimeout;

        bool legacySecurity;

        // Runtime online options
        bool help;
        bool version;
        bool dumpConfig;
        std::string configFile;
        std::string outputFile;

        std::string secretViewKey;
        std::string secretSpendKey;
        std::string mnemonicSeed;

        bool generateNewContainer;
        bool daemonize;
        bool registerService;
        bool unregisterService;
        bool printAddresses;
        bool syncFromZero;

        uint64_t scanHeight;
    };

    bool updateConfigFormat(const std::string configFile, WalletServiceConfiguration &config);
    void handleSettings(int argc, char *argv[], WalletServiceConfiguration &config);
    void handleSettings(const std::string configFile, WalletServiceConfiguration &config);
    json asJSON(const WalletServiceConfiguration &config);
    std::string asString(const WalletServiceConfiguration &config);
    void asFile(const WalletServiceConfiguration &config, const std::string &filename);
}
