// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "rocksdb_wrapper.h"

#include "rocksdb/cache.h"
#include "rocksdb/table.h"
#include "rocksdb/db.h"
#include "rocksdb/utilities/backupable_db.h"

#include "database_errors.h"

using namespace mevacoin;
using namespace logging;

namespace
{
    const std::string DB_NAME = "DB";
    const std::string TESTNET_DB_NAME = "testnet_DB";
}

RocksDBWrapper::RocksDBWrapper(std::shared_ptr<logging::ILogger> logger) : logger(logger, "RocksDBWrapper"), state(NOT_INITIALIZED)
{
}

RocksDBWrapper::~RocksDBWrapper()
{
}

void RocksDBWrapper::init(const DataBaseConfig &config)
{
    if (state.load() != NOT_INITIALIZED)
    {
        throw std::system_error(make_error_code(mevacoin::error::DataBaseErrorCodes::ALREADY_INITIALIZED));
    }

    std::string dataDir = getDataDir(config);

    logger(INFO) << "Opening DB in " << dataDir;

    rocksdb::DB *dbPtr;

    rocksdb::Options dbOptions = getDBOptions(config);
    rocksdb::Status status = rocksdb::DB::Open(dbOptions, dataDir, &dbPtr);
    if (status.ok())
    {
        logger(INFO) << "DB opened in " << dataDir;
    }
    else if (!status.ok() && status.IsInvalidArgument())
    {
        logger(INFO) << "DB not found in " << dataDir << ". Creating new DB...";
        dbOptions.create_if_missing = true;
        rocksdb::Status status = rocksdb::DB::Open(dbOptions, dataDir, &dbPtr);
        if (!status.ok())
        {
            logger(ERROR) << "DB Error. DB can't be created in " << dataDir << ". Error: " << status.ToString();
            throw std::system_error(make_error_code(mevacoin::error::DataBaseErrorCodes::INTERNAL_ERROR));
        }
    }
    else if (status.IsIOError())
    {
        logger(ERROR) << "DB Error. DB can't be opened in " << dataDir << ". Error: " << status.ToString();
        throw std::system_error(make_error_code(mevacoin::error::DataBaseErrorCodes::IO_ERROR));
    }
    else
    {
        logger(ERROR) << "DB Error. DB can't be opened in " << dataDir << ". Error: " << status.ToString();
        throw std::system_error(make_error_code(mevacoin::error::DataBaseErrorCodes::INTERNAL_ERROR));
    }

    db.reset(dbPtr);
    state.store(INITIALIZED);
}

void RocksDBWrapper::shutdown()
{
    if (state.load() != INITIALIZED)
    {
        throw std::system_error(make_error_code(mevacoin::error::DataBaseErrorCodes::NOT_INITIALIZED));
    }

    logger(INFO) << "Closing DB.";
    db->Flush(rocksdb::FlushOptions());
    db->SyncWAL();
    db.reset();
    state.store(NOT_INITIALIZED);
}

void RocksDBWrapper::destroy(const DataBaseConfig &config)
{
    if (state.load() != NOT_INITIALIZED)
    {
        throw std::system_error(make_error_code(mevacoin::error::DataBaseErrorCodes::ALREADY_INITIALIZED));
    }

    std::string dataDir = getDataDir(config);

    logger(WARNING) << "Destroying DB in " << dataDir;

    rocksdb::Options dbOptions = getDBOptions(config);
    rocksdb::Status status = rocksdb::DestroyDB(dataDir, dbOptions);

    if (status.ok())
    {
        logger(WARNING) << "DB destroyed in " << dataDir;
    }
    else
    {
        logger(ERROR) << "DB Error. DB can't be destroyed in " << dataDir << ". Error: " << status.ToString();
        throw std::system_error(make_error_code(mevacoin::error::DataBaseErrorCodes::INTERNAL_ERROR));
    }
}

std::error_code RocksDBWrapper::write(IWriteBatch &batch)
{
    if (state.load() != INITIALIZED)
    {
        throw std::system_error(make_error_code(mevacoin::error::DataBaseErrorCodes::NOT_INITIALIZED));
    }

    return write(batch, false);
}

std::error_code RocksDBWrapper::write(IWriteBatch &batch, bool sync)
{
    rocksdb::WriteOptions writeOptions;
    writeOptions.sync = sync;

    rocksdb::WriteBatch rocksdbBatch;
    std::vector<std::pair<std::string, std::string>> rawData(batch.extractRawDataToInsert());
    for (const std::pair<std::string, std::string> &kvPair : rawData)
    {
        rocksdbBatch.Put(rocksdb::Slice(kvPair.first), rocksdb::Slice(kvPair.second));
    }

    std::vector<std::string> rawKeys(batch.extractRawKeysToRemove());
    for (const std::string &key : rawKeys)
    {
        rocksdbBatch.Delete(rocksdb::Slice(key));
    }

    rocksdb::Status status = db->Write(writeOptions, &rocksdbBatch);

    if (!status.ok())
    {
        logger(ERROR) << "Can't write to DB. " << status.ToString();
        return make_error_code(mevacoin::error::DataBaseErrorCodes::INTERNAL_ERROR);
    }
    else
    {
        return std::error_code();
    }
}

std::error_code RocksDBWrapper::read(IReadBatch &batch)
{
    if (state.load() != INITIALIZED)
    {
        throw std::runtime_error("Not initialized.");
    }

    rocksdb::ReadOptions readOptions;

    std::vector<std::string> rawKeys(batch.getRawKeys());
    std::vector<rocksdb::Slice> keySlices;
    keySlices.reserve(rawKeys.size());
    for (const std::string &key : rawKeys)
    {
        keySlices.emplace_back(rocksdb::Slice(key));
    }

    std::vector<std::string> values;
    values.reserve(rawKeys.size());
    std::vector<rocksdb::Status> statuses = db->MultiGet(readOptions, keySlices, &values);

    std::error_code error;
    std::vector<bool> resultStates;
    for (const rocksdb::Status &status : statuses)
    {
        if (!status.ok() && !status.IsNotFound())
        {
            return make_error_code(mevacoin::error::DataBaseErrorCodes::INTERNAL_ERROR);
        }
        resultStates.push_back(status.ok());
    }

    batch.submitRawResult(values, resultStates);
    return std::error_code();
}

rocksdb::Options RocksDBWrapper::getDBOptions(const DataBaseConfig &config)
{
    rocksdb::DBOptions dbOptions;
    dbOptions.IncreaseParallelism(config.getBackgroundThreadsCount());
    dbOptions.info_log_level = rocksdb::InfoLogLevel::WARN_LEVEL;
    dbOptions.max_open_files = config.getMaxOpenFiles();

    rocksdb::ColumnFamilyOptions fOptions;
    fOptions.write_buffer_size = static_cast<size_t>(config.getWriteBufferSize());
    // merge two memtables when flushing to L0
    fOptions.min_write_buffer_number_to_merge = 2;
    // this means we'll use 50% extra memory in the worst case, but will reduce
    // write stalls.
    fOptions.max_write_buffer_number = 6;
    // start flushing L0->L1 as soon as possible. each file on level0 is
    // (memtable_memory_budget / 2). This will flush level 0 when it's bigger than
    // memtable_memory_budget.
    fOptions.level0_file_num_compaction_trigger = 20;

    fOptions.level0_slowdown_writes_trigger = 30;
    fOptions.level0_stop_writes_trigger = 40;

    // doesn't really matter much, but we don't want to create too many files
    fOptions.target_file_size_base = config.getWriteBufferSize() / 10;
    // make Level1 size equal to Level0 size, so that L0->L1 compactions are fast
    fOptions.max_bytes_for_level_base = config.getWriteBufferSize();
    fOptions.num_levels = 10;
    fOptions.target_file_size_multiplier = 2;
    // level style compaction
    fOptions.compaction_style = rocksdb::kCompactionStyleLevel;

    fOptions.compression_per_level.resize(fOptions.num_levels);
    for (int i = 0; i < fOptions.num_levels; ++i)
    {
        fOptions.compression_per_level[i] = rocksdb::kNoCompression;
    }

    rocksdb::BlockBasedTableOptions tableOptions;
    tableOptions.block_cache = rocksdb::NewLRUCache(config.getReadCacheSize());
    std::shared_ptr<rocksdb::TableFactory> tfp(NewBlockBasedTableFactory(tableOptions));
    fOptions.table_factory = tfp;

    return rocksdb::Options(dbOptions, fOptions);
}

std::string RocksDBWrapper::getDataDir(const DataBaseConfig &config)
{
    if (config.getTestnet())
    {
        return config.getDataDir() + '/' + TESTNET_DB_NAME;
    }
    else
    {
        return config.getDataDir() + '/' + DB_NAME;
    }
}
