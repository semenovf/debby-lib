////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "sqlite3.h"
#include "pfs/fmt.hpp"
#include <string>
#include <cassert>

namespace debby {
namespace sqlite3 {

inline std::string build_errstr (int rc, struct sqlite3 * dbh)
{
    if (rc != SQLITE_OK) {
        int extended_rc = dbh ? sqlite3_extended_errcode(dbh) : rc;

        if (extended_rc != rc) {
            return fmt::format("{} [code={}]: {} [extended code={}]"
                , sqlite3_errstr(rc)
                , rc
                , sqlite3_errstr(extended_rc)
                , extended_rc);
        } else {
            return fmt::format("{} [code={}]", sqlite3_errstr(rc), rc);
        }
    }

    return std::string{};
}

inline std::string build_errstr (int rc, struct sqlite3_stmt * sth)
{
    return build_errstr(rc, sth ? sqlite3_db_handle(sth)
        : reinterpret_cast<struct sqlite3 *>(0));
}

inline std::string build_errstr (int rc)
{
    return build_errstr(rc, reinterpret_cast<struct sqlite3 *>(0));
}

inline std::string current_sql (struct sqlite3_stmt * sth) noexcept
{
    assert(sth);

    char const * sql {nullptr};

#ifdef SQLITE_ENABLE_NORMALIZE
    sql = sqlite3_normalized_sql(sth);
#endif

    if (!sql)
        sql = sqlite3_expanded_sql(sth);

    if (!sql)
        sql = sqlite3_sql(sth);

    return sql ? std::string(sql) : std::string{};
}

}} // namespace debby::sqlite3
