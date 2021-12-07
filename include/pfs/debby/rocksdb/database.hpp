////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/debby/exports.hpp"
#include "pfs/debby/keyvalue_database.hpp"
#include <string>
#include <unordered_map>

namespace rocksdb {
class DB;
} // namespace rocksdb

namespace pfs {
namespace debby {
namespace rocksdb {

PFS_DEBBY__EXPORT class database: public keyvalue_database<database>
{
    friend class basic_database<database>;
    friend class keyvalue_database<database>;

private:
    using base_class  = keyvalue_database<database>;
    using native_type = ::rocksdb::DB *;

private:
    native_type _dbh {nullptr};
    std::string _last_error;

private:
    std::string last_error_impl () const noexcept
    {
        return _last_error;
    }

    bool open_impl (filesystem::path const & path);
    void close_impl ();

    bool is_opened_impl () const noexcept
    {
        return _dbh != nullptr;
    }

//     bool clear_impl ();
//     std::vector<std::string> tables_impl (std::string const & pattern = std::string{});
//     bool exists_impl (std::string const & name);
//     statement prepare_impl (std::string const & sql, bool cache);
//
//     bool query_impl (std::string const & sql);
//     bool begin_impl ();
//     bool commit_impl ();
//     bool rollback_impl ();

public:
    database () {}

    ~database ()
    {
        close_impl();
    }

    database (database const &) = delete;
    database & operator = (database const &) = delete;

    database (database && other)
    {
        *this = std::move(other);
    }

    database & operator = (database && other)
    {
        close_impl();
        _dbh = other._dbh;
        other._dbh = nullptr;
        return *this;
    }
};

}}} // namespace pfs::debby::rocksdb
