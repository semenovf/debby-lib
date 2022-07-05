#include <iostream>
#include <unordered_map>

enum CompressionType : unsigned char {
    // NOTE: do not change the values of existing entries, as these are
    // part of the persistent format on disk.
    kNoCompression = 0x0,
    kSnappyCompression = 0x1,
    kZlibCompression = 0x2,
    kBZip2Compression = 0x3,
    kLZ4Compression = 0x4,
    kLZ4HCCompression = 0x5,
    kXpressCompression = 0x6,
    kZSTD = 0x7,

    // Only use kZSTDNotFinalCompression if you have to use ZSTD lib older than
    // 0.8.0 or consider a possibility of downgrading the service or copying
    // the database files to another service running with an older version of
    // RocksDB that doesn't have kZSTD. Otherwise, you should use kZSTD. We will
    // eventually remove the option from the public API.
    kZSTDNotFinalCompression = 0x40,

    // kDisableCompressionOption is used to disable some compression options.
    kDisableCompressionOption = 0xff,
};

struct A
{
    static std::unordered_map<std::string, CompressionType> compression_type_string_map;
};

std::unordered_map<std::string, CompressionType>
A::compression_type_string_map = {
    {"kNoCompression", kNoCompression},
    {"kSnappyCompression", kSnappyCompression},
    {"kZlibCompression", kZlibCompression},
    {"kBZip2Compression", kBZip2Compression},
    {"kLZ4Compression", kLZ4Compression},
    {"kLZ4HCCompression", kLZ4HCCompression},
    {"kXpressCompression", kXpressCompression},
    {"kZSTD", kZSTD},
    {"kZSTDNotFinalCompression", kZSTDNotFinalCompression},
    {"kDisableCompressionOption", kDisableCompressionOption} };

int main ()
{
    A a;

    auto opt = A::compression_type_string_map["kBZip2Compression"];

    if (opt == kBZip2Compression)
        std::cout << "OK!\n";
    else
        std::cout << "NOK!\n";

    return 0;
}