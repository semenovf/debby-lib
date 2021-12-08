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

    template <typename T, typename E>
    using expected_type = expected<T, E>;

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
    template <typename T>
    inline T unpack_fixed (char const * s, std::size_t count, bool & ok)
    {
        ok = true;

        if (sizeof(T) != count) {
            ok = false;
            return T{};
        }

        fixed_packer<T> p;
        std::memcpy(p.bytes, s, count);
        return p.value;
    }

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

    bool write (key_type const & key, char const * data, std::size_t len);

    /**
     * Read value by @a key.
     *
     * @return One of this values:
     *      - expected value if @a key contains any valid value;
     *      - unexpected value @c false if value not found by @a key;
     *      - unexpected value @c true if an error occurred while searching
     *        value by @a key.
     */
    expected_type<std::string, bool> read (key_type const & key);

    template <typename T, bool = std::is_arithmetic<T>::value>
    inline bool set_impl (key_type const & key, T value)
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
