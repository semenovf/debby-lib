////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.04 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "../keyvalue_database_common.hpp"
#include <pfs/assert.hpp>
#include <pfs/variant.hpp>
#include <mutex>

#if DEBBY__MAP_ENABLED
#   include <map>
#endif

#if DEBBY__UNORDERED_MAP_ENABLED
#   include <unordered_map>
#endif

DEBBY__NAMESPACE_BEGIN

using unified_value_t = pfs::variant<std::int64_t, double, std::string>;

struct lock_guard_stub
{
    using mutex_type = bool;
    lock_guard_stub (bool) {}
};

template <typename Container, typename Locker>
class keyvalue_database_impl
{
public:
    using key_type    = typename Container::key_type;
    using value_type  = typename Container::mapped_type;
    using native_type = Container;
    using lock_guard  = Locker;
    using mutex_type  = typename Locker::mutex_type;

protected:
    mutable mutex_type _mtx;
    native_type _dbh;

public:
    keyvalue_database_impl () noexcept = default;

    keyvalue_database_impl (keyvalue_database_impl && other) noexcept
    {
        lock_guard locker(other._mtx);
        _dbh = std::move(other._dbh);
    }

    keyvalue_database_impl & operator = (keyvalue_database_impl && other) noexcept
    {
        lock_guard locker(other._mtx);
        _dbh = std::move(other._dbh);
        return *this;
    }

public:
    void clear ()
    {
        _dbh.clear();
    }

    void set (key_type const & key, std::int64_t value, std::size_t, error *)
    {
        value_type uv {value};

        lock_guard locker{_mtx};
        _dbh[key] = std::move(uv);
    }

    void set (key_type const & key, double value, std::size_t, error *)
    {
        value_type uv {value};

        lock_guard locker{_mtx};
        _dbh[key] = std::move(uv);
    }

    void set (key_type const & key, char const * data, std::size_t size, error * perr)
    {
        lock_guard locker{_mtx};

        // Attempt to write `null` data interpreted as delete operation for key
        if (data == nullptr) {
            remove(key, perr);
        } else {
            _dbh[key] = value_type {std::string(data, size)};
        }
    }

    void remove (key_type const & key, error *)
    {
        lock_guard locker{_mtx};
        _dbh.erase(key);
    }

    template <typename T>
    T get (std::string const & key, error * perr)
    {
        lock_guard locker{_mtx};
        auto pos = _dbh.find(key);

        errc e = errc::success;

        if (pos != _dbh.end()) {
            if (pfs::holds_alternative<T>(pos->second)) {
                return pfs::get<T>(pos->second);
            } else {
                e = errc::bad_value;
            }
        } else {
            e = errc::key_not_found;
        }

        pfs::throw_or(perr, error {make_error_code(e)});
        return T{};
    }
};

#if DEBBY__MAP_ENABLED
template <>
class keyvalue_database<backend_enum::map_st>::impl
    : public keyvalue_database_impl<std::map<std::string, unified_value_t>, lock_guard_stub>
{};

template <>
class keyvalue_database<backend_enum::map_mt>::impl
    : public keyvalue_database_impl<std::map<std::string, unified_value_t>, std::lock_guard<std::mutex>>
{};
#endif

#if DEBBY__UNORDERED_MAP_ENABLED
template <>
class keyvalue_database<backend_enum::unordered_map_st>::impl
    : public keyvalue_database_impl<std::unordered_map<std::string, unified_value_t>, lock_guard_stub>
{};

template <>
class keyvalue_database<backend_enum::unordered_map_mt>::impl
    : public keyvalue_database_impl<std::unordered_map<std::string, unified_value_t>, std::lock_guard<std::mutex>>
{};
#endif

template <backend_enum Backend>
void keyvalue_database<Backend>::clear (error *)
{
    if (_d != nullptr)
        _d->clear();
}

namespace in_memory {

template <backend_enum Backend>
keyvalue_database<Backend> make_kv (error *)
{
    return keyvalue_database<Backend> {typename keyvalue_database<Backend>::impl{}};
}

template <backend_enum Backend>
bool wipe (error *)
{
    return true;
}

#if DEBBY__MAP_ENABLED
template DEBBY__EXPORT keyvalue_database<backend_enum::map_st> make_kv<backend_enum::map_st> (error *);
template DEBBY__EXPORT keyvalue_database<backend_enum::map_mt> make_kv<backend_enum::map_mt> (error *);
template DEBBY__EXPORT bool wipe<backend_enum::map_st> (error *);
template DEBBY__EXPORT bool wipe<backend_enum::map_mt> (error *);
#endif

#if DEBBY__UNORDERED_MAP_ENABLED
template DEBBY__EXPORT keyvalue_database<backend_enum::unordered_map_st> make_kv<backend_enum::unordered_map_st> (error *);
template DEBBY__EXPORT keyvalue_database<backend_enum::unordered_map_mt> make_kv<backend_enum::unordered_map_mt> (error *);
template DEBBY__EXPORT bool wipe<backend_enum::unordered_map_st> (error *);
template DEBBY__EXPORT bool wipe<backend_enum::unordered_map_mt> (error *);
#endif

} // namespace in_mamory

template <backend_enum Backend>
void keyvalue_database<Backend>::set_arithmetic (key_type const & key, std::int64_t value, std::size_t size, error * perr)
{
    if (_d != nullptr)
        _d->set(key, value, size, perr);
}

template <backend_enum Backend>
void keyvalue_database<Backend>::set_arithmetic (key_type const & key, double value, std::size_t size, error * perr)
{
    if (_d != nullptr)
        _d->set(key, value, size, perr);
}

template <backend_enum Backend>
void keyvalue_database<Backend>::set_chars (key_type const & key, char const * data, std::size_t size, error * perr)
{
    if (_d != nullptr)
        _d->set(key, data, size, perr);
}

template <backend_enum Backend>
void
keyvalue_database<Backend>::remove (key_type const & key, error * perr)
{
    if (_d != nullptr)
        _d->remove(key, perr);
}

template <backend_enum Backend>
std::int64_t keyvalue_database<Backend>::get_int64 (key_type const & key, error * perr)
{
    PFS__TERMINATE(_d != nullptr, "");
    return _d->template get<std::int64_t>(key, perr);
}

template <backend_enum Backend>
double keyvalue_database<Backend>::get_double (key_type const & key, error * perr)
{
    PFS__TERMINATE(_d != nullptr, "");
    return _d->template get<double>(key, perr);
}

template <backend_enum Backend>
std::string keyvalue_database<Backend>::get_string (key_type const & key, error * perr)
{
    PFS__TERMINATE(_d != nullptr, "");
    return _d->template get<std::string>(key, perr);
}

#if DEBBY__MAP_ENABLED
template class keyvalue_database<backend_enum::map_st>;
template class keyvalue_database<backend_enum::map_mt>;
#endif

#if DEBBY__UNORDERED_MAP_ENABLED
template class keyvalue_database<backend_enum::unordered_map_st>;
template class keyvalue_database<backend_enum::unordered_map_mt>;
#endif

DEBBY__NAMESPACE_END
