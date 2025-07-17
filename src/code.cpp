#include <cpp11.hpp>
#include "blosc.h"

using namespace cpp11;

[[cpp11::register]]
std::string blosc_version() {
  return std::string(BLOSC_VERSION_STRING);
}

[[cpp11::register]]
void blosc_test() {
#ifdef BLOSC_EXPORT
  Rprintf("Yes functions exported!\n");
#else
  Rprintf("Nope not exported");
#endif
}

[[cpp11::register]]
int nthreads() {
  return blosc_get_nthreads();
}

[[cpp11::register]]
raws blosc_compress(raws data, int level, int doshuffle,
                    int typesize) {
  uint8_t *src = (uint8_t *)(RAW(as_sexp(data)));
  writable::raws result((R_xlen_t)data.size() + BLOSC_MAX_OVERHEAD);
  uint8_t *dest = (uint8_t *)(RAW(as_sexp(result)));
  int out = blosc_compress_ctx(level, doshuffle, typesize, data.size(), src, dest, result.size(),
                               "zlib", 0, 1);
  if (out < 0) stop("BLOSC compressor failed!");
  result.resize(out);
  Rprintf("Out size %i\n", out);
  return result;
}

[[cpp11::register]]
raws blosc_decompress(raws data) {
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