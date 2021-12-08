////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/expected.hpp"
#include "pfs/fmt.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/keyvalue_database.hpp"
#include <string>
#include <unordered_map>
#include <cstring>

namespace rocksdb {
class DB;
} // namespace rocksdb

namespace pfs {
namespace debby {
namespace rocksdb {

struct database_traits
{
    using key_type = std::string;
};

PFS_DEBBY__EXPORT class database: public keyvalue_database<database, database_traits>
{
    friend class basic_database<database>;
    friend class keyvalue_database<database, database_traits>;

public:
    using key_type = database_traits::key_type;

private:
    using base_class  = keyvalue_database<database, database_traits>;
    using native_type = ::rocksdb::DB *;

    template <typename T>
    union fixed_packer
    {
        T value;
        char bytes[sizeof(T)];
    };

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

    bool write (key_type const & key, char const * data, std::size_t len);
    expected<std::string, bool> read (key_type const & key);

    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, bool>::type
    set_impl (key_type const & key, T value)
    {
        fixed_packer<T> p;
        p.value = value;
        return write(key, p.bytes, sizeof(T));
    }

    inline bool set_impl (key_type const & key, std::string const & value)
    {
        return write(key, value.data(), value.size());
    }

    inline bool set_impl (key_type const & key, char const * value, std::size_t len)
    {
        return write(key, value, len);
    }

    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, expected<T, bool>>::type
    get_impl (key_type const & key)
    {
        auto res = read(key);

        if (res) {
            if (sizeof(T) != res->size()) {
                _last_error = fmt::format("unsuitable value stored by key: `{}`");
                return make_unexpected(true);
            }
        } else {
            if (res.error()) {
                return make_unexpected(true);
            } else {
                return make_unexpected(false);
            }
        }

        fixed_packer<T> p;
        std::memcpy(p.bytes, res->data(), res->size());
        return p.value;
    }

    template <typename T>
    typename std::enable_if<std::is_same<std::string, T>::value, expected<T, bool>>::type
    get_impl (key_type const & key)
    {
        return read(key);
    }

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
