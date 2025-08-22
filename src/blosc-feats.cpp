#include <cpp11.hpp>
#include "blosc.h"

using namespace cpp11;

[[cpp11::register]]
list blosc_info_(raws data) {
  uint8_t *src = (uint8_t *)(RAW(as_sexp(data)));
  size_t decomp_size = 0;
  int version = -1, compversion = -1, flags = -1;
  size_t nbytes = 0, cbytes = 0, typesize = 0, bsize = 0;
  bool shuffle, memcop, bitshuf;
  
  int validate = blosc_cbuffer_validate(src, data.size(), &decomp_size);
  if (validate < 0) stop("Invalid blosc data");
  std::string cstr = blosc_cbuffer_complib(src);
  blosc_cbuffer_versions(src, &version, &compversion);
  blosc_cbuffer_metainfo(src, &typesize, &flags);
  blosc_cbuffer_sizes(src, &nbytes, &cbytes, &bsize);
  shuffle  = (flags & 0x1) != 0;
  memcop   = (flags & 0x2) != 0;
  bitshuf  = (flags & 0x4) != 0;
  writable::logicals sh((R_xlen_t)1);
  writable::logicals mc((R_xlen_t)1);
  writable::logicals bs((R_xlen_t)1);
  sh[0] = shuffle;
  mc[0] = memcop;
  bs[0] = bitshuf;
  
  writable::list result({
    writable::strings({cstr}),
    writable::integers({version}),
    writable::integers({compversion}),
    writable::integers({(int)typesize}),
    writable::integers({(int)bsize}),
    writable::integers({(int)nbytes}),
    writable::integers({(int)cbytes}),
    sh, mc, bs
  });
  result.attr("names") = writable::strings({
    "Compressor",
    "Blosc format version",
    "Internal compressor version",
    "Type size in bytes",
    "Block size in bytes",
    "Uncompressed size in bytes",
    "Compressed size in bytes",
    "Shuffle",
    "Pure memcpy",
    "Bit shuffle"
  });
  result.attr("class") = writable::strings({
    "blosc_info",
    "list"
  });
  return result;
}
