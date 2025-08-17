#include <cpp11.hpp>
#include "blosc.h"

using namespace cpp11;

raws blosc_compress_internal(uint8_t *p, R_xlen_t s, std::string compressor,
                             int level, int doshuffle, int typesize) {
  writable::raws result(s + BLOSC_MAX_OVERHEAD);
  uint8_t *dest = (uint8_t *)(RAW(as_sexp(result)));
  int out = blosc_compress_ctx(level, doshuffle, typesize, s, p, dest, result.size(),
                               compressor.c_str(), 0, 1);
  if (out < 0) stop("BLOSC compressor failed!");
  result.resize(out);
  return result;
}

[[cpp11::register]]
raws blosc_compress_dat(raws data, std::string compressor, int level, int doshuffle,
                    int typesize) {
  uint8_t *src = (uint8_t *)(RAW(as_sexp(data)));
  return blosc_compress_internal(src, (R_xlen_t)data.size(), compressor,
                                 level, doshuffle, typesize);
}

[[cpp11::register]]
raws blosc_decompress_dat(raws data) {
  uint8_t *src = (uint8_t *)(RAW(as_sexp(data)));
  size_t decomp_size = 0;
  int validate = blosc_cbuffer_validate(src, data.size(), &decomp_size);
  if (validate < 0) stop("Unable to decompress data");
  writable::raws result((R_xlen_t)decomp_size);
  uint8_t *dest = (uint8_t *)(RAW(as_sexp(result)));
  
  int test = blosc_decompress_ctx(src, dest, decomp_size, 1);
  if (test < 0) stop("Failed to decompress data");
  return result;
}
