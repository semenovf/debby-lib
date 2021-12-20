////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
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

        case static_cast<int>(errc::bad_value):
            return std::string{"bad/unsuitable value"};

        case static_cast<int>(errc::sql_error):
            return std::string{"sql error"};

        case static_cast<int>(errc::invalid_argument):
            return std::string{"invalid argument"};

        default: return std::string{"unknown debby error"};
    }
};

std::string error::what () const noexcept
{
    if (!_ec)
        return _ec.message();

    std::string result;

    if (_ec.value() != static_cast<int>(errc::backend_error))
        result += _ec.message();

    if (!_description.empty())
        result += std::string{(result.empty() ? "" : ": ")} + _description;

    if (!_cause.empty())
        result += std::string{(result.empty() ? "" : " ")} + '(' + _cause + ')';

    return result;
}

} // namespace debby
