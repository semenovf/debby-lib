////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/fmt.hpp"
#include "pfs/debby/error.hpp"

namespace debby {

char const * error_category::name () const noexcept
{
    return "debby::error_category";
}

std::string error_category::message (int ev) const
{
    switch (ev) {
        case static_cast<int>(errc::success):
            return std::string{"no error"};

        case static_cast<int>(errc::bad_alloc):
            return std::string{"bad alloc"};

        case static_cast<int>(errc::backend_error):
            return std::string{"backend error"};

        case static_cast<int>(errc::database_not_found):
            return std::string{"database not found"};

        case static_cast<int>(errc::key_not_found):
            return std::string{"key not found"};

        case static_cast<int>(errc::bad_value):
            return std::string{"bad/unsuitable value"};

        case static_cast<int>(errc::sql_error):
            return std::string{"sql error"};

        case static_cast<int>(errc::unsupported):
            return std::string{"unsupported"};

        default: return std::string{"unknown debby error"};
    }
};

std::error_category const & get_error_category ()
{
    static error_category instance;
    return instance;
}

} // namespace debby
