####################################################################################################
# Copyright (c) 2023 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
#
# Changelog:
#      2023.11.25 Initial version.
####################################################################################################
project(postgres-ep)

if (MSVC)
    message(FATAL_ERROR "Unsupported build system for ${PROJECT_NAME} (TODO: need to implement)")
else()
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
        include(ExternalProject)

        list(APPEND _postgres_configure_opts
            "--prefix=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}")

        list(APPEND _postgres_configure_opts --without-readline
            --with-pgport=5432
            --disable-debug
            --disable-cassert)

        if (DEBBY__BUILD_SHARED)
            list(APPEND _postgres_configure_opts "CFLAGS=-fPIC")
        endif()

        set(_bin_dir "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}")
        set(_src_dir "${CMAKE_CURRENT_LIST_DIR}/postgres")

        ExternalProject_Add(${PROJECT_NAME}
            PREFIX ${_bin_dir}
            SOURCE_DIR ${_src_dir}
            CONFIGURE_COMMAND "${_src_dir}/configure" ${_postgres_configure_opts}
            BUILD_IN_SOURCE FALSE
            BUILD_BYPRODUCTS ${_bin_dir}/lib/libpgcommon.a ${_bin_dir}/lib/libpgport.a ${_bin_dir}/lib/libpq-static.a
            BUILD_COMMAND make -C src/include
                && make -C src/common
                && make -C src/port
                && make -C src/interfaces/libpq
            INSTALL_COMMAND make -C src/include install
                && make -C src/common install
                && make -C src/port install
                && make -C src/interfaces/libpq install
                && ${CMAKE_COMMAND} -E copy_if_different ${_bin_dir}/lib/libpq.a ${_bin_dir}/lib/libpq-static.a)

        ExternalProject_Get_Property(${PROJECT_NAME} INSTALL_DIR)
        set(${PROJECT_NAME}_INSTALL_DIR ${INSTALL_DIR})

        file(MAKE_DIRECTORY "${${PROJECT_NAME}_INSTALL_DIR}/include")

        foreach (_lib pgcommon pgport pq-static)
            add_library(${_lib} STATIC IMPORTED GLOBAL)
            set_target_properties(${_lib} PROPERTIES
                IMPORTED_LOCATION "${${PROJECT_NAME}_INSTALL_DIR}/lib/lib${_lib}.a"
                INTERFACE_INCLUDE_DIRECTORIES "${${PROJECT_NAME}_INSTALL_DIR}/include")
            add_dependencies(${_lib} ${PROJECT_NAME})
        endforeach()
    else()
        message(FATAL_ERROR "Unsupported build system for ${PROJECT_NAME} (TODO: need to implement)")
    endif()
endif()

