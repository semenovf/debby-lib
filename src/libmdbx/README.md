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

Switch off `CMAKE_INTERPROCEDURAL_OPTIMIZATION_AVAILABLE` while build APK for Android:

```cmake
#      Inserted
#  _______|_______
#  |             |
#  v             v
if(NOT ANDROID AND NOT CMAKE_VERSION VERSION_LESS 3.9)
  cmake_policy(SET CMP0068 NEW)
  cmake_policy(SET CMP0069 NEW)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT CMAKE_INTERPROCEDURAL_OPTIMIZATION_AVAILABLE)
...
```
