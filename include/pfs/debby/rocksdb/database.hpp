////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/fmt.hpp"
#include "pfs/optional.hpp"
#include "pfs/debby/error.hpp"
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
    static std::string const ERROR_DOMAIN;

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

private:
    // throws runtime_error
    bool open_impl (filesystem::path const & path, bool create_if_missing);

    void close_impl ();

    bool is_opened_impl () const noexcept
    {
        return _dbh != nullptr;
    }

    // throws runtime_error
    bool write (key_type const & key, char const * data, std::size_t len);

    // throws runtime_error
    optional<std::string> read (key_type const & key);

    // throws runtime_error
    bool remove_impl (key_type const & key);

    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, bool>::type
    set_impl (key_type const & key, T value)
    {
        fixed_packer<T> p;
        p.value = value;
        return write(key, p.bytes, sizeof(T));
    }

    // throws runtime_error
    inline bool set_impl (key_type const & key, std::string const & value)
    {
        return write(key, value.data(), value.size());
    }

    // throws runtime_error
    inline bool set_impl (key_type const & key, char const * value, std::size_t len)
    {
        return write(key, value, len);
    }

    // throws runtime_error
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, optional<T>>::type
    get_impl (key_type const & key)
    {
        try {
            auto res = read(key);

            if (res) {
                if (sizeof(T) != res->size()) {
                    PFS_DEBBY_THROW((runtime_error{
                        ERROR_DOMAIN
                        , fmt::format("unsuitable value stored by key: '{}'", key)
                    }));
                }

                fixed_packer<T> p;
                std::memcpy(p.bytes, res->data(), res->size());
                return p.value;
            }
        } catch (...) {
            std::rethrow_exception(std::current_exception());
        }

        return nullopt;
    }

    template <typename T>
    typename std::enable_if<std::is_same<std::string, T>::value, optional<T>>::type
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
