# debby-lib

Provides unified tools for access to SQL relational databases and Key-Value storages.
This is a lightweight wrapper around popular database/storage engines.

Currently implementation is done for:

* `sqlite3`
* `RocksDB`
* `PostgreSQL`
* `libmdbx`
* `lmdb`
* in-memory based on `std::map` and `std::unordered_map` (thread safe and unsafe)

The list can grow...

## Clone main repository

```sh
$ git clone https://github.com/semenovf/debby-lib.git
$ cd debby-lib
```

## Build library

### RocksDB notes

Now supports the `6.x` version only (`6.29.5` is the last known version as of February 2023 that 
requires C++11). Since version `7.0` (20.02.2022) (unsupported yet by `debby`) `RocksDB` requires 
C++17 compatible compiler (GCC >= 7, Clang >= 5, Visual Studio >= 2017). Since version `10.7.0` 
(19.09.2025) (unsupported yet by `debby`) `RocksDB` requires `C++20` compatible compiler 
(GCC >= 11, Clang >= 10, Visual Studio >= 2019).

### Build sample

```sh
$ mkdir build
$ cd build

# Generate build scripts using Ninja generator in this example and specify
# option to build tests (default is OFF). Besides that enable all supported
# backends.
$ cmake -G Ninja -DDEBBY__BUILD_TESTS=ON \
    -DDEBBY__ENABLE_SQLITE3=ON \
    -DDEBBY__ENABLE_LMDB=ON \
    -DDEBBY__ENABLE_MDBX=ON \
    -DDEBBY__ENABLE_ROCKSDB_CXX11=ON \
    -DDEBBY__ENABLE_MAP=ON \
    -DDEBBY__ENABLE_UNORDERED_MAP=ON \
    ..

$ cmake --build .

# Run tests if option was specified.
$ ctest
```
