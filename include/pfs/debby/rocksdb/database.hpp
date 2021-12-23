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
#include "pfs/optional.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/keyvalue_database.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>

namespace rocksdb {
class DB;
class ColumnFamilyHandle;
} // namespace rocksdb

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

    enum {
          INT_COLUMN_FAMILY_INDEX = 0
        , FP_COLUMN_FAMILY_INDEX
        , STR_COLUMN_FAMILY_INDEX
        , BLOB_COLUMN_FAMILY_INDEX
    };

    template <typename T, typename U = void>
    struct column_family_traits;

    template <typename T>
    struct column_family_traits<T, typename std::enable_if<
        std::is_same<T, std::string>::value>::type>
    {
        static constexpr int index = STR_COLUMN_FAMILY_INDEX;
    };

    template <typename T>
    struct column_family_traits<T, typename std::enable_if<
        std::is_integral<T>::value>::type>
    {
        static constexpr int index = INT_COLUMN_FAMILY_INDEX;
    };

    template <typename T>
    struct column_family_traits<T, typename std::enable_if<
        std::is_floating_point<T>::value>::type>
    {
        static constexpr int index = FP_COLUMN_FAMILY_INDEX;
    };

    template <typename T>
    struct column_family_traits<T, typename std::enable_if<
        std::is_same<T, blob_t>::value>::type>
    {
        static constexpr int index = BLOB_COLUMN_FAMILY_INDEX;
    };

private:
    pfs::filesystem::path _path;
    native_type _dbh {nullptr};
    std::vector<::rocksdb::ColumnFamilyHandle *> _type_column_families;

private:
    bool open (pfs::filesystem::path const & path
        , bool create_if_missing
        , error * err) noexcept;

    void close () noexcept;

    bool is_opened () const noexcept
    {
        return _dbh != nullptr;
    }

    bool clear_impl(error * err);

    /**
     * Writes @c data into database by @a key.
     *
     * Attempt to write @c nullptr data interpreted as delete operation for key.
     *
     * @return Zero error code on success, but on error one of the following:
     *         - errc::backend_error
     *
     * @throws debby::exception.
     */
    bool write (key_type const & key, int column_family_index
        , char const * data, std::size_t len, error * perr);

    /**
     * Reads value by @a key from database.
     *
     * @return Value associated with @a key, or @c nullopt if specified key not found.
     */
    bool read (key_type const & key, int & column_family_index
        , pfs::optional<std::string> & target
        , error * perr);

    value_type fetch_impl (key_type const & key
        , int column_family_index
        , error * perr);

    template <typename T>
    value_type fetch_typed_impl (key_type const & key, error * perr)
    {
        return fetch_impl(key
            , column_family_traits<T>::index
            , perr);
    }

    value_type fetch_impl (key_type const & key, error * perr)
    {
        return fetch_impl(key, -1, perr);
    }

    /**
     * Removes value for @a key.
     */
    bool remove_impl (key_type const & key, error * perr);

    template <typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    set_impl (key_type const & key, T value, error * perr)
    {
        fixed_packer<T> p;
        p.value = value;
        return write(key, INT_COLUMN_FAMILY_INDEX, p.bytes, sizeof(T), perr);
    }

    template <typename T>
    typename std::enable_if<std::is_floating_point<T>::value, bool>::type
    set_impl (key_type const & key, T value, error * perr)
    {
        fixed_packer<T> p;
        p.value = value;
        return write(key, FP_COLUMN_FAMILY_INDEX, p.bytes, sizeof(T), perr);
    }

    bool set_impl (key_type const & key, std::string const & value, error * perr)
    {
        return write(key, STR_COLUMN_FAMILY_INDEX
            , value.data(), value.size(), perr);
    }

    bool set_impl (key_type const & key, char const * value
        , std::size_t len, error * perr)
    {
        return write(key, STR_COLUMN_FAMILY_INDEX, value, len, perr);
    }

    bool set_impl (key_type const & key, blob_t const & value, error * perr)
    {
        return write(key, BLOB_COLUMN_FAMILY_INDEX
            , reinterpret_cast<char const *>(value.data()), value.size(), perr);
    }

//     /**
//      * Pulls arithmetic type value associated with @a key from database.
//      */
//     template <typename T>
//     typename std::enable_if<std::is_arithmetic<T>::value, bool>::type
//     pull_impl (key_type const & key, pfs::optional<T> & target, error * perr)
//     {
//         pfs::optional<std::string> opt;
//
//         if (!read(key, opt, perr))
//             return false;
//
//         if (!opt) {
//             target = pfs::nullopt;
//             return true;
//         }
//
//         if (sizeof(T) != opt->size()) {
//             auto ec = make_error_code(errc::bad_value);
//             auto err = error{ec, fmt::format("unsuitable value stored by key: {}"
//                 , key)};
//             if (perr) *perr = err; else DEBBY__THROW(err);
//             return false;
//         }
//
//         fixed_packer<T> p;
//         std::memcpy(p.bytes, opt->data(), opt->size());
//         target = p.value;
//         return true;
//     }
//
//     /**
//      * Pulls string value associated with @a key from database.
//      */
//     bool pull_impl (key_type const & key, pfs::optional<std::string> & target
//         , error * perr)
//     {
//         return read(key, STR_COLUMN_FAMILY_INDEX, target, perr);
//     }
//
//     /**
//      * Pulls string value associated with @a key from database.
//      */
//     bool pull_impl (key_type const & key, pfs::optional<blob_t> & target
//         , error * perr);
//     {
//         return read(key, STR_COLUMN_FAMILY_INDEX, target, perr);
//     }

public:
    database () = default;

    /**
     * Open database specified by @a path and create it if
     * missing when @a create_if_missing set to @c true.
     *
     * @param path Path to the database.
     * @param create_if_missing If @c true create database if it missing.
     * @param ec Store error code one of the following:
     *         - errc::database_not_found
     *         - errc::backend_error
     *
     * @throws debby::exception.
     */
    database (pfs::filesystem::path const & path, bool create_if_missing
        , error * perr = nullptr)
    {
        open(path, create_if_missing, perr);
    }

    database (pfs::filesystem::path const & path)
    {
        open(path, true, nullptr);
    }

    /**
     * Release database data
     */
    ~database ()
    {
        close();
    }

    database (database && other)
    {
        *this = std::move(other);
    }

    database & operator = (database && other)
    {
        close();
        _dbh = other._dbh;
        other._dbh = nullptr;
        return *this;
    }
};

}} // namespace debby::rocksdb
