////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.10.28 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include <map>
#include <string>

inline std::map<std::string, std::string> psql_conninfo ()
{
    std::map<std::string, std::string> conninfo {
          {"host", "localhost"}
        , {"port", "5432"} // Default port
        , {"user", "test"}
        , {"password", "12345678"}
        , {"dbname", "postgres"}
    };

    return conninfo;
}

inline std::string preconditions_notice ()
{
    return std::string("\n"
        "=======================================================================================\n"
        "Perhaps it was not possible to connect to the database\n"
        "For testing purposes, the following prerequisites must be met:\n"
        "\t* PostgresSQL instance must be started on localhost on port 5432;\n"
        "\t* `test` login must be available with roles ...;\n"
        "\t* password for test login must be `12345678`.\n"
        "\n"
        "Below instructions can help to create `test` user/login\n"
        "$ psql --host=localhost --port=5432 --user=postgres\n"
        "postgres=# CREATE ROLE test WITH LOGIN NOSUPERUSER CREATEDB NOCREATEROLE NOINHERIT NOREPLICATION CONNECTION LIMIT -1 PASSWORD '12345678'\n"
        "postgres=# \\q\n\n"
        "Check connection:\n"
        "$ psql --host=localhost --port=5432 --user=test --database=postgres\n"
        "=======================================================================================\n");
}
