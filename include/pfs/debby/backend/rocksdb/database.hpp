////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.07 Initial version.
//      2021.12.16 Reimplemented with new error handling.
//      2022.03.12 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/filesystem.hpp"
#include <vector>

namespace rocksdb {
class DB;
class ColumnFamilyHandle;
struct Options;
} // namespace rocksdb

namespace debby {
namespace backend {
namespace rocksdb {

struct database
{
    using key_type     = std::string;
    using native_type  = ::rocksdb::DB *;
    using options_type = ::rocksdb::Options;

    enum {
          INT_COLUMN_FAMILY_INDEX = 0
        , FP_COLUMN_FAMILY_INDEX
        , STR_COLUMN_FAMILY_INDEX
        , BLOB_COLUMN_FAMILY_INDEX
    };

    template <typename T>
    union fixed_packer
    {
        T value;
        char bytes[sizeof(T)];
    };

    struct rep_type
    {
        native_type dbh;
        pfs::filesystem::path path;
        std::vector<::rocksdb::ColumnFamilyHandle *> type_column_families;

        static result_status write (
              database::rep_type * rep
            , database::key_type const & key
            , int column_family_index
            , char const * data, std::size_t len);

        template <typename T>
        static typename std::enable_if<std::is_integral<T>::value, result_status>::type
        set (database::rep_type * rep, key_type const & key, T value)
        {
            fixed_packer<T> p;
            p.value = value;
            return write(rep, key, INT_COLUMN_FAMILY_INDEX, p.bytes, sizeof(T));
        }

        template <typename T>
        static typename std::enable_if<std::is_floating_point<T>::value, result_status>::type
        set (database::rep_type * rep, key_type const & key, T value)
        {
            fixed_packer<T> p;
            p.value = value;
            return write(rep, key, FP_COLUMN_FAMILY_INDEX, p.bytes, sizeof(T));
        }
    };

    /**
     * Open database specified by @a path and create it if
     * missing when @a create_if_missing set to @c true.
     *
     * @param path Path to the database.
     * @param opts RocksDB specific options or @c nullptr for default options.
     *
     * @throw debby::error().
     */
    static rep_type make (pfs::filesystem::path const & path
        , options_type * popts = nullptr);

    static rep_type make (pfs::filesystem::path const & path
        , bool create_if_missing);
};

}}} // namespace debby::backend::rocksdb
