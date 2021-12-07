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

namespace pfs {
namespace debby {
namespace sqlite3 {

inline std::string build_errstr (std::string const & caption
    , int rc
    , struct sqlite3 * dbh)
{
    if (rc != SQLITE_OK) {
        int extended_rc = dbh ? sqlite3_extended_errcode(dbh) : rc;

        if (extended_rc != rc) {
            return fmt::format("{}: {}: {}"
                , caption
                , sqlite3_errstr(rc)
                , sqlite3_errstr(extended_rc));
        } else {
            return fmt::format("{}: {}"
                , caption
                , sqlite3_errstr(rc));
        }
    }

    return std::string{};
}

inline std::string build_errstr (std::string const & caption
    , int rc
    , struct sqlite3_stmt * sth)
{
    return build_errstr(caption, rc, sth ? sqlite3_db_handle(sth)
        : reinterpret_cast<struct sqlite3 *>(0));
}

inline std::string build_errstr (std::string const & caption, int rc)
{
    return build_errstr(caption, rc, reinterpret_cast<struct sqlite3 *>(0));
}

}}} // namespace pfs::debby::sqlite3
