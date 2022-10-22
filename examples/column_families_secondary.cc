    // Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
    //  This source code is licensed under both the GPLv2 (found in the
    //  COPYING file in the root directory) and Apache 2.0 License
    //  (found in the LICENSE.Apache file in the root directory).
    #include <cstdio>
    #include <string>
    #include <vector>
    #include <iostream>
    #include <unistd.h> 

    #include "rocksdb/db.h"
    #include "rocksdb/slice.h"
    #include "rocksdb/options.h"

    #if defined(OS_WIN)
    std::string kDBPath = "C:\\Windows\\TEMP\\rocksdb_column_families_example";
    #else
    std::string kDBPath = "/tmp/rocksdb_column_families_primary";
    std::string kDBSecondaryPath = "/tmp/rocksdb_column_families_secondary";
    #endif

    using ROCKSDB_NAMESPACE::ColumnFamilyDescriptor;
    using ROCKSDB_NAMESPACE::ColumnFamilyHandle;
    using ROCKSDB_NAMESPACE::ColumnFamilyOptions;
    using ROCKSDB_NAMESPACE::DB;
    using ROCKSDB_NAMESPACE::DBOptions;
    using ROCKSDB_NAMESPACE::Options;
    using ROCKSDB_NAMESPACE::ReadOptions;
    using ROCKSDB_NAMESPACE::FlushOptions;
    using ROCKSDB_NAMESPACE::Slice;
    using ROCKSDB_NAMESPACE::Status;
    using ROCKSDB_NAMESPACE::WriteBatch;
    using ROCKSDB_NAMESPACE::WriteOptions;

int main() {
    // open DB
    Options options;
    options.create_if_missing = true;
    options.max_total_wal_size = 8192;
    
    DB* db;
    Status s = DB::Open(options, kDBPath, &db);
    assert(s.ok());

    // create column family
    ColumnFamilyHandle* cf1;
    ColumnFamilyHandle* cf2;
    s = db->CreateColumnFamily(ColumnFamilyOptions(), "cf1", &cf1);
    assert(s.ok());
    s = db->CreateColumnFamily(ColumnFamilyOptions(), "cf2", &cf2);
    assert(s.ok());

    // close DB
    s = db->DestroyColumnFamilyHandle(cf1);
    assert(s.ok());
    s = db->DestroyColumnFamilyHandle(cf2);
    assert(s.ok());
    delete db;

    // open DB with two column families
    std::vector<ColumnFamilyDescriptor> column_families;
    // have to open default column family
    column_families.push_back(ColumnFamilyDescriptor(
        ROCKSDB_NAMESPACE::kDefaultColumnFamilyName, ColumnFamilyOptions()));
    // open the new one, too
    column_families.push_back(ColumnFamilyDescriptor(
        "cf1", ColumnFamilyOptions()));
    column_families.push_back(ColumnFamilyDescriptor(
        "cf2", ColumnFamilyOptions()));
    
    // Primary DB
    std::vector<ColumnFamilyHandle*> handles;
    s = DB::Open(DBOptions(), kDBPath, column_families, &handles, &db);
    assert(s.ok());

    DB* secondary_db = nullptr;
    Options secondary_options;
    secondary_options.create_if_missing = false;
    secondary_options.max_open_files = -1;

    // put and get from non-default column family
    s = db->Put(WriteOptions(), handles[2], Slice("key1"), Slice("value1"));
    assert(s.ok());
    std::string value;
    s = db->Get(ReadOptions(), handles[2], Slice("key1"), &value);
    assert(s.ok());
    std::cout<<"Priary DB Value: " <<value<<std::endl;

    // s = db->Flush(FlushOptions());
    assert(s.ok());

    std::vector<ColumnFamilyDescriptor> secondary_column_families;
    secondary_column_families.push_back(ColumnFamilyDescriptor(
    ROCKSDB_NAMESPACE::kDefaultColumnFamilyName, ColumnFamilyOptions()));
    // open the new one, too
    secondary_column_families.push_back(ColumnFamilyDescriptor(
        "cf1", ColumnFamilyOptions()));
    secondary_column_families.push_back(ColumnFamilyDescriptor(
        "cf2", ColumnFamilyOptions()));

    // Secondary DB
    std::vector<ColumnFamilyHandle*> secondary_handles;
    s = DB::OpenAsSecondary(secondary_options, kDBPath, kDBSecondaryPath, secondary_column_families, &secondary_handles, &secondary_db);
    assert(s.ok());

    std::string secondary_value;
    // s = secondary_db->Get(ReadOptions(), secondary_handles[2], Slice("key1"), &secondary_value);
    // std::cout<<"Secondary DB Value (key1): " <<secondary_value<< s.ok()<<std::endl;
    // assert(s.ok());

    for (int i = 0; i<200000; i++) {
        // std::cout << "Writing the key " << "key" + std::to_string(i) << "with value: " << "value" + std::to_string(i) << std::endl;
        s = db->Put(WriteOptions(), handles[1], Slice("key-a" + std::to_string(i)), Slice("value-a" + std::to_string(i)));
        assert(s.ok());
        // assert(s.ok());
    }
    // sleep(30);
    // sleep(30);
    // s = db->Flush(FlushOptions(), handles[2]);
    s = db->Flush(FlushOptions(), handles[1]);
    // s = db->Flush(FlushOptions());
    // sleep(20);

    s = db->Put(WriteOptions(), handles[2], Slice("key3"), Slice("value3"));
    assert(s.ok());
    // s = db->Flush(FlushOptions());
    // assert(s.ok());

    s = db->Put(WriteOptions(), handles[2], Slice("key4"), Slice("value4"));
    assert(s.ok());
    // s = db->Flush(FlushOptions());
    // assert(s.ok());

    for (int i = 0; i<200000; i++) {
        // std::cout << "Writing the key " << "key" + std::to_string(i) << "with value: " << "value" + std::to_string(i) << std::endl;
        s = db->Put(WriteOptions(), handles[2], Slice("key-b" + std::to_string(i)), Slice("value-b" + std::to_string(i)));
        assert(s.ok());
        // s = db->Flush(FlushOptions());
        // assert(s.ok());
    }


    // // drop column family
    // s = db->DropColumnFamily(handles[2]);

    s = db->Put(WriteOptions(), handles[2], Slice("key5"), Slice("value5"));
    assert(s.ok());
    // s = db->Flush(FlushOptions());
    // assert(s.ok());


    // s = secondary_db->TryCatchUpWithPrimary(secondary_handles[1]);
    s = secondary_db->TryCatchUpWithPrimary(secondary_handles[2]);
    // s = secondary_db->TryCatchUpWithPrimary();
    assert(s.ok());
    // s = secondary_db->Get(ReadOptions(), secondary_handles[2], Slice("key3"), &secondary_value);
    // std::cout<<"Secondary DB Value (key3): " <<secondary_value<< s.ok() << std::endl;
    s = secondary_db->Get(ReadOptions(), secondary_handles[1], Slice("key-a10"), &secondary_value);
    std::cout<<"Secondary DB Value (key-a0): " <<secondary_value<< s.ok() << std::endl;
    s = secondary_db->Get(ReadOptions(), secondary_handles[2], Slice("key-b10"), &secondary_value);
    std::cout<<"Secondary DB Value (key-b0): " <<secondary_value<< s.ok() << std::endl;
    s = secondary_db->Get(ReadOptions(), secondary_handles[2], Slice("key4"), &secondary_value);
    std::cout<<"Secondary DB Value (key4): " <<secondary_value<< s.ok() << std::endl;
    s = secondary_db->Get(ReadOptions(), secondary_handles[2], Slice("key5"), &secondary_value);
    std::cout<<"Secondary DB Value (key5): " <<secondary_value<< s.ok() << std::endl;
    
    // close db
    for (auto handle : handles) {
        s = db->DestroyColumnFamilyHandle(handle);
        assert(s.ok());
    }
    delete db;

    return 0;
    }
