# debby-lib

Provides unified tools for access to SQL databases and Key-Value storages.
This is a lightweight wrapper around popular database/storage engines.

Currently implementation is done for:

* Sqlite3
* RocksDB
* libmdbx

The list will grow...

## Clone main repository

```sh
$ git clone https://github.com/semenovf/debby-lib.git
$ cd debby-lib
```

## Pull dependencies

Only `Sqlite3` backend dependency now embeded into project source tree.
To support other dependencies need to pull corresponding submodules.

If need to pull all dependencies just do

```sh
$ git submodule update --init
```

### RocksDB

Now supports the `6.x` version (`6.29.5` is the last known version as of February 2023)

```sh
$ git submodule update --init -- 3rdparty/rocksdb
```

### libmdbx

```sh
$ git submodule update --init -- 3rdparty/libmdbx
```

## Build library

```sh
# Build out of project tree.
$ mkdir ../build
$ cd ../build

# Generate build scripts using Ninja generator in this example and specify
# option to build tests (default is OFF). Besides that enable all supported
# backends.
$ cmake -G Ninja -DDEBBY__BUILD_TEST=ON \
    -DDEBBY__ENABLE_SQLITE3=ON \
    -DDEBBY__ENABLE_LIBMDBX=ON \
    -DDEBBY__ENABLE_ROCKSDB=ON \
    ../debby-lib

$ cmake --build .

# Run tests if option was specified.
$ ctest
```
