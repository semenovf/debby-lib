# debby-lib

Provides unified tools for access to SQL databases and Key-Value storages.
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

## Pull dependencies

`sqlite3`, `lmdb` and `libmdbx` backend dependencies embeded into project source tree.
To support other dependencies need to download corresponding projects (cd into `3dparty` and call
`download.sh`).

### RocksDB

Now supports the `6.x` version (`6.29.5` is the last known version as of February 2023)

## Build library

```sh
$ mkdir build
$ cd build

# Generate build scripts using Ninja generator in this example and specify
# option to build tests (default is OFF). Besides that enable all supported
# backends.
$ cmake -G Ninja -DDEBBY__BUILD_TESTS=ON \
    -DDEBBY__ENABLE_SQLITE3=ON \
    -DDEBBY__ENABLE_LMDB=ON \
    -DDEBBY__ENABLE_LIBMDBX=ON \
    -DDEBBY__ENABLE_ROCKSDB=ON \
    -DDEBBY__ENABLE_MAP=ON \
    -DDEBBY__ENABLE_UNORDERED_MAP=ON \
    ..

$ cmake --build .

# Run tests if option was specified.
$ ctest
```
