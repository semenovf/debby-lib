////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024-2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.04 Initial version.
//      2025.09.29 Changed set/get implementation.
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

using unified_value_t = pfs::variant<
      bool
    , char
    , signed char
    , unsigned char
    , short int
    , unsigned short int
    , int
    , unsigned int
    , long int
    , unsigned long int
    , long long int
    , unsigned long long int
    , float
    , double
    , std::string>;

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

    void remove (key_type const & key, error *)
    {
        lock_guard locker{_mtx};
        _dbh.erase(key);
    }

    template <typename T>
    std::enable_if_t<std::is_arithmetic<T>::value, void>
    set (key_type const & key, T value, error * /*perr*/ = nullptr)
    {
        value_type uv(value);

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

    template <typename T>
    T get (std::string const & key, error * perr) const
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

template <backend_enum Backend>
void keyvalue_database<Backend>::remove (key_type const & key, error * perr)
{
    if (_d != nullptr)
        _d->remove(key, perr);
}

template <backend_enum Backend>
void keyvalue_database<Backend>::set (key_type const & key, char const * value, std::size_t len
    , error * perr)
{
    _d->set(key, value, len, perr);
}

template <backend_enum Backend>
template <typename T>
std::enable_if_t<std::is_arithmetic<T>::value, void>
keyvalue_database<Backend>::set (key_type const & key, T value, error * perr)
{
    _d->template set<T>(key, value, perr);
}

template <backend_enum Backend>
template <typename T>
std::enable_if_t<std::is_arithmetic<T>::value || std::is_same<std::decay_t<T>, std::string>::value, std::decay_t<T>>
keyvalue_database<Backend>::get (key_type const & key, error * perr) const
{
    return _d->template get<std::decay_t<T>>(key, perr);
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

} // namespace in_memory

#if DEBBY__MAP_ENABLED
template class keyvalue_database<backend_enum::map_st>;
template class keyvalue_database<backend_enum::map_mt>;

#define DEBBY__MAP_ST_SET(t) \
    template void keyvalue_database<backend_enum::map_st>::set<t> (key_type const & key, t value, error * perr);

#define DEBBY__MAP_ST_GET(t) \
    template t keyvalue_database<backend_enum::map_st>::get<t> (key_type const & key, error * perr) const;

DEBBY__MAP_ST_SET(bool)
DEBBY__MAP_ST_SET(char)
DEBBY__MAP_ST_SET(signed char)
DEBBY__MAP_ST_SET(unsigned char)
DEBBY__MAP_ST_SET(short int)
DEBBY__MAP_ST_SET(unsigned short int)
DEBBY__MAP_ST_SET(int)
DEBBY__MAP_ST_SET(unsigned int)
DEBBY__MAP_ST_SET(long int)
DEBBY__MAP_ST_SET(unsigned long int)
DEBBY__MAP_ST_SET(long long int)
DEBBY__MAP_ST_SET(unsigned long long int)
DEBBY__MAP_ST_SET(float)
DEBBY__MAP_ST_SET(double)

DEBBY__MAP_ST_GET(bool)
DEBBY__MAP_ST_GET(char)
DEBBY__MAP_ST_GET(signed char)
DEBBY__MAP_ST_GET(unsigned char)
DEBBY__MAP_ST_GET(short int)
DEBBY__MAP_ST_GET(unsigned short int)
DEBBY__MAP_ST_GET(int)
DEBBY__MAP_ST_GET(unsigned int)
DEBBY__MAP_ST_GET(long int)
DEBBY__MAP_ST_GET(unsigned long int)
DEBBY__MAP_ST_GET(long long int)
DEBBY__MAP_ST_GET(unsigned long long int)
DEBBY__MAP_ST_GET(float)
DEBBY__MAP_ST_GET(double)
DEBBY__MAP_ST_GET(std::string)

#define DEBBY__MAP_MT_SET(t) \
    template void keyvalue_database<backend_enum::map_mt>::set<t> (key_type const & key, t value, error * perr);

#define DEBBY__MAP_MT_GET(t) \
    template t keyvalue_database<backend_enum::map_mt>::get<t> (key_type const & key, error * perr) const;

DEBBY__MAP_MT_SET(bool)
DEBBY__MAP_MT_SET(char)
DEBBY__MAP_MT_SET(signed char)
DEBBY__MAP_MT_SET(unsigned char)
DEBBY__MAP_MT_SET(short int)
DEBBY__MAP_MT_SET(unsigned short int)
DEBBY__MAP_MT_SET(int)
DEBBY__MAP_MT_SET(unsigned int)
DEBBY__MAP_MT_SET(long int)
DEBBY__MAP_MT_SET(unsigned long int)
DEBBY__MAP_MT_SET(long long int)
DEBBY__MAP_MT_SET(unsigned long long int)
DEBBY__MAP_MT_SET(float)
DEBBY__MAP_MT_SET(double)

DEBBY__MAP_MT_GET(bool)
DEBBY__MAP_MT_GET(char)
DEBBY__MAP_MT_GET(signed char)
DEBBY__MAP_MT_GET(unsigned char)
DEBBY__MAP_MT_GET(short int)
DEBBY__MAP_MT_GET(unsigned short int)
DEBBY__MAP_MT_GET(int)
DEBBY__MAP_MT_GET(unsigned int)
DEBBY__MAP_MT_GET(long int)
DEBBY__MAP_MT_GET(unsigned long int)
DEBBY__MAP_MT_GET(long long int)
DEBBY__MAP_MT_GET(unsigned long long int)
DEBBY__MAP_MT_GET(float)
DEBBY__MAP_MT_GET(double)
DEBBY__MAP_MT_GET(std::string)

#endif

#if DEBBY__UNORDERED_MAP_ENABLED
template class keyvalue_database<backend_enum::unordered_map_st>;
template class keyvalue_database<backend_enum::unordered_map_mt>;

#define DEBBY__UNORDEREDMAP_ST_SET(t) \
    template void keyvalue_database<backend_enum::unordered_map_st>::set<t> (key_type const & key, t value, error * perr);

#define DEBBY__UNORDEREDMAP_ST_GET(t) \
    template t keyvalue_database<backend_enum::unordered_map_st>::get<t> (key_type const & key, error * perr) const;

DEBBY__UNORDEREDMAP_ST_SET(bool)
DEBBY__UNORDEREDMAP_ST_SET(char)
DEBBY__UNORDEREDMAP_ST_SET(signed char)
DEBBY__UNORDEREDMAP_ST_SET(unsigned char)
DEBBY__UNORDEREDMAP_ST_SET(short int)
DEBBY__UNORDEREDMAP_ST_SET(unsigned short int)
DEBBY__UNORDEREDMAP_ST_SET(int)
DEBBY__UNORDEREDMAP_ST_SET(unsigned int)
DEBBY__UNORDEREDMAP_ST_SET(long int)
DEBBY__UNORDEREDMAP_ST_SET(unsigned long int)
DEBBY__UNORDEREDMAP_ST_SET(long long int)
DEBBY__UNORDEREDMAP_ST_SET(unsigned long long int)
DEBBY__UNORDEREDMAP_ST_SET(float)
DEBBY__UNORDEREDMAP_ST_SET(double)

DEBBY__UNORDEREDMAP_ST_GET(bool)
DEBBY__UNORDEREDMAP_ST_GET(char)
DEBBY__UNORDEREDMAP_ST_GET(signed char)
DEBBY__UNORDEREDMAP_ST_GET(unsigned char)
DEBBY__UNORDEREDMAP_ST_GET(short int)
DEBBY__UNORDEREDMAP_ST_GET(unsigned short int)
DEBBY__UNORDEREDMAP_ST_GET(int)
DEBBY__UNORDEREDMAP_ST_GET(unsigned int)
DEBBY__UNORDEREDMAP_ST_GET(long int)
DEBBY__UNORDEREDMAP_ST_GET(unsigned long int)
DEBBY__UNORDEREDMAP_ST_GET(long long int)
DEBBY__UNORDEREDMAP_ST_GET(unsigned long long int)
DEBBY__UNORDEREDMAP_ST_GET(float)
DEBBY__UNORDEREDMAP_ST_GET(double)
DEBBY__UNORDEREDMAP_ST_GET(std::string)

#define DEBBY__UNORDEREDMAP_MT_SET(t) \
    template void keyvalue_database<backend_enum::unordered_map_mt>::set<t> (key_type const & key, t value, error * perr);

#define DEBBY__UNORDEREDMAP_MT_GET(t) \
    template t keyvalue_database<backend_enum::unordered_map_mt>::get<t> (key_type const & key, error * perr) const;

DEBBY__UNORDEREDMAP_MT_SET(bool)
DEBBY__UNORDEREDMAP_MT_SET(char)
DEBBY__UNORDEREDMAP_MT_SET(signed char)
DEBBY__UNORDEREDMAP_MT_SET(unsigned char)
DEBBY__UNORDEREDMAP_MT_SET(short int)
DEBBY__UNORDEREDMAP_MT_SET(unsigned short int)
DEBBY__UNORDEREDMAP_MT_SET(int)
DEBBY__UNORDEREDMAP_MT_SET(unsigned int)
DEBBY__UNORDEREDMAP_MT_SET(long int)
DEBBY__UNORDEREDMAP_MT_SET(unsigned long int)
DEBBY__UNORDEREDMAP_MT_SET(long long int)
DEBBY__UNORDEREDMAP_MT_SET(unsigned long long int)
DEBBY__UNORDEREDMAP_MT_SET(float)
DEBBY__UNORDEREDMAP_MT_SET(double)

DEBBY__UNORDEREDMAP_MT_GET(bool)
DEBBY__UNORDEREDMAP_MT_GET(char)
DEBBY__UNORDEREDMAP_MT_GET(signed char)
DEBBY__UNORDEREDMAP_MT_GET(unsigned char)
DEBBY__UNORDEREDMAP_MT_GET(short int)
DEBBY__UNORDEREDMAP_MT_GET(unsigned short int)
DEBBY__UNORDEREDMAP_MT_GET(int)
DEBBY__UNORDEREDMAP_MT_GET(unsigned int)
DEBBY__UNORDEREDMAP_MT_GET(long int)
DEBBY__UNORDEREDMAP_MT_GET(unsigned long int)
DEBBY__UNORDEREDMAP_MT_GET(long long int)
DEBBY__UNORDEREDMAP_MT_GET(unsigned long long int)
DEBBY__UNORDEREDMAP_MT_GET(float)
DEBBY__UNORDEREDMAP_MT_GET(double)
DEBBY__UNORDEREDMAP_MT_GET(std::string)

#endif

DEBBY__NAMESPACE_END
