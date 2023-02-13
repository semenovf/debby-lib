After updating `libmdbx` from the release zip, some entries need to be commented
out to ignore checks that prevent building without `C++` support, man pages and
utilities.

```cmake
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" AND
    EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mdbx.c" AND
    #EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mdbx.c++" AND # <= Here
    EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    #AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/man1" AND # <= Here
    #EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mdbx_chk.c"   # <= And here
)
```
