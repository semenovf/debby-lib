////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
//      2021.12.16 Reimplemented with new error handling.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/fmt.hpp"
#include "pfs/expected.hpp"
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

class database: public keyvalue_database<database, database_traits>
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

private:
    /**
     * Open database specified by @a path and create it if
     * missing when @a create_if_missing set to @c true.
     *
     * @return Zero error code on success, but on error one of the following:
     *         - errc::database_already_open
     *         - errc::database_not_found
     *         - errc::backend_error
     *
     * @throws pfs::debby::exception.
     */
    std::error_code open_impl (filesystem::path const & path, bool create_if_missing);

    /**
     * Close database.
     *
     * @return Zero error code.
     */
    std::error_code close_impl ();

    bool is_opened_impl () const noexcept
    {
        return _dbh != nullptr;
    }

    /**
     * Writes @c data into database by @a key.
     *
     * Attempt to write @c nullptr data interpreted as delete operation for key.
     *
     * @return Zero error code on success, but on error one of the following:
     *         - errc::backend_error
     *
     * @throws pfs::debby::exception.
     */
    std::error_code write (key_type const & key, char const * data, std::size_t len);

    /**
     * Reads value by @a key from database.
     *
     * @return Value associated with @a key, or @c nullopt if specified key not found.
     *         On error one of the following error code stored at @a ec:
     *         - errc::backend_error
     *
     * @throws pfs::debby::exception @c errc::backend_error if any backend error occured.
     */
    optional<std::string> read (key_type const & key, std::error_code & ec);

    /**
     * Removes value for @a key.
     *
     * @return Zero error code on success, but on error one of the following:
     *         - errc::backend_error
     *
     * @throws pfs::debby::exception.
     */
    std::error_code remove_impl (key_type const & key);

    /**
     * Stores arithmetic type @a value associated with @a key into database.
     *
     * @return Zero error code on success, but on error one of the following:
     *         - errc::backend_error
     *
     * @throws pfs::debby::exception.
     */
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, std::error_code>::type
    set_impl (key_type const & key, T value)
    {
        fixed_packer<T> p;
        p.value = value;
        return write(key, p.bytes, sizeof(T));
    }

    inline std::error_code set_impl (key_type const & key
        , std::string const & value)
    {
        return write(key, value.data(), value.size());
    }

    inline std::error_code set_impl (key_type const & key
        , char const * value
        , std::size_t len)
    {
        return write(key, value, len);
    }

    /**
     * Fetch arithmetic type value associated with @a key from database.
     *
     * @return Value or @c nullopt (value not found by @a key ) success and
     *         zero error code, but on error one of the following error codes:
     *         - errc::backend_error
     *         - errc::bad_value
     *
     * @throws pfs::debby::exception.
     */
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, expected_result<optional<T>>>::type
    get_impl (key_type const & key)
    {
        std::error_code ec;
        auto opt = read(key, ec);

        if (ec) {
#if !PFS_DEBBY__EXCEPTIONS_ENABLED
            return make_unexpected(ec);
#endif
        }

        if (!opt)
            return nullopt;

        if (sizeof(T) != opt->size()) {
            auto rc = make_error_code(errc::bad_value);
            exception::failure(exception{rc
                , fmt::format("unsuitable value stored by key: '{}'", key)
            });
        }

        fixed_packer<T> p;
        std::memcpy(p.bytes, opt->data(), opt->size());
        return p.value;
    }

    /**
     * Fetch string value associated with @a key from database.
     *
     * @return Value or @c nullopt (value not found by @a key ) success and
     *         zero error code, but on error one of the following error codes:
     *         - errc::backend_error
     *         - errc::bad_value
     *
     * @throws pfs::debby::exception.
     */
    template <typename T>
    typename std::enable_if<std::is_same<std::string, T>::value, expected_result<optional<T>>>::type
    get_impl (key_type const & key)
    {
        std::error_code ec;
        auto opt = read(key, ec);

        if (ec) {
#if !PFS_DEBBY__EXCEPTIONS_ENABLED
            return make_unexpected(ec);
#endif
        }

        return opt;
    }

public:
    database () = default;

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
