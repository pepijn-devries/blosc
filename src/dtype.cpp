#include <cpp11.hpp>
#include "umHalf.h"

using namespace cpp11;

#define MAX_DTYPE_SIZE 255

typedef struct {
  bool needs_byteswap;
  char main_type;
  uint8_t byte_size;
} blosc_dtype;

typedef struct {
  float real;
  float imaginary;
} complex32;

typedef struct {
  double real;
  double imaginary;
} complex64;

union conversion_t {
  bool      b1;
  uint8_t   u1;
  int8_t    i1;
  uint16_t  u2;
  int16_t   i2;
  uint32_t  u4;
  int32_t   i4;
  uint64_t  u8;
  int64_t   i8;
  uint16_t  f2; // bit representation obtained from float16
  float     f4;
  double    f8;
  complex32 c8;
  complex64 c16;
};

void convert_data(uint8_t *input, int rtype, int n, blosc_dtype dtype, uint8_t *output);
void convert_data_inv(conversion_t *input, blosc_dtype dtype, int rtype, uint8_t *output);
void byte_swap(uint8_t * data, blosc_dtype dtype, uint32_t n);

blosc_dtype prepare_dtype(std::string dtype) {
  blosc_dtype dt;
  int dlen = dtype.length();
  if (dlen < 3 || dlen > 5) stop("'dtype should be between 3 and 5 character long!");
  char byte_order = dtype.c_str()[0];
  if (byte_order != '<' && byte_order != '>' && byte_order != '|')
    stop("Unknown byte order '%c'", byte_order);
  if (byte_order == '|')
    dt.needs_byteswap = false; else
#ifdef WORDS_BIGENDIAN
      dt.needs_byteswap = (byte_order == '<');
#else
    dt.needs_byteswap = (byte_order == '>');
#endif
    
    dt.main_type = dtype.c_str()[1];
    std::string accepted_types = "biufcmM";
    if (accepted_types.find(dt.main_type) == std::string::npos)
      stop("Datatype '%c' not known or implemented", dt.main_type);
    
    uint32_t bz = 0;
    for (int i = 2; i < (int)dtype.length(); i++) {
      int s = dtype.c_str()[i] - '0';
      if (s < 0 || s > 9) stop("Invalid byte size");
      bz *= 10;
      bz += s;
    }
    if (bz > MAX_DTYPE_SIZE) stop("Invalid byte size");
    dt.byte_size = (uint8_t)bz;
    
    if (dt.main_type == 'b' && dt.byte_size != 1)
      stop("Unknown data type '%s'", dtype.c_str());
    if ((dt.main_type == 'i' || dt.main_type == 'f' || dt.main_type == 'u') &&
        (dt.byte_size == 3 || (dt.byte_size > 4 && dt.byte_size < 8) ||
        dt.byte_size > 8))
      stop("Unknown data type '%s'", dtype.c_str());
    if ((dt.main_type == 'f'|| dt.main_type == 'c') && dt.byte_size == 1)
      stop("Unknown data type '%s'", dtype.c_str());
    if (dt.main_type == 'c' && dt.byte_size != 8 && dt.byte_size != 16)
      stop("Unknown data type '%s'", dtype.c_str());
    
    return dt;
}

[[cpp11::register]]
sexp dtype_to_r_(raws data, std::string dtype, double na_value) {
  blosc_dtype dt = prepare_dtype(dtype);
  if (data.size() % dt.byte_size != 0)
    stop("Raw data size needs to be multitude of data type size");
  conversion_t conv, empty;
  complex64 cempty;
  cempty.real = 0.0;
  cempty.imaginary = 0.0;
  empty.c16 = cempty;
  conv = empty;
  int n = data.size() / dt.byte_size;
  uint8_t *src;
  if (dt.needs_byteswap) {
    writable::raws data_copy(data.size());
    src = (uint8_t *)(RAW(data_copy));
    memcpy(src, RAW(data), data.size());
    byte_swap(src, dt, n);
  } else {
    src = (uint8_t *)(RAW(data));
  }
  
  uint8_t *dest;

  sexp result;
  
  if (dt.main_type == 'b' && dt.byte_size == 1) {
    result = writable::logicals((R_xlen_t) n);
    dest = (uint8_t *)LOGICAL(result);
  } else if (dt.main_type == 'i' && dt.byte_size <= 4) {
    result = writable::integers((R_xlen_t) n);
    dest = (uint8_t *)INTEGER(result);
  } else if(dt.main_type == 'i' && dt.byte_size >4 && dt.byte_size <= 8) {
    result = writable::doubles((R_xlen_t) n);
    dest = (uint8_t *)REAL(result);
  } else if(dt.main_type == 'u' && dt.byte_size <= 3) {
    result = writable::integers((R_xlen_t) n);
    dest = (uint8_t *)INTEGER(result);
  } else if(dt.main_type == 'u' && dt.byte_size <= 7) {
    result = writable::doubles((R_xlen_t) n);
    dest = (uint8_t *)REAL(result);
  } else if(dt.main_type == 'f' && dt.byte_size <= 8) {
    result = writable::doubles((R_xlen_t) n);
    dest = (uint8_t *)REAL(result);
  } else if(dt.main_type == 'c' && dt.byte_size <= 16) {
    stop ("TODO writable::complex does not exist yet");
    //SET_COMPLEX_ELT()
    result = Rf_allocVector(CPLXSXP, (R_xlen_t)n);
    dest = (uint8_t *)COMPLEX(result);
  } else {
    stop("Cannot convert data type to an R type");
  }
  uint32_t out_size = 0;
  if (TYPEOF(result) == INTSXP || TYPEOF(result) == LGLSXP) {
    out_size = sizeof(int);
  } else if (TYPEOF(result) == REALSXP) {
    out_size = sizeof(double);
  } else if (TYPEOF(result) == CPLXSXP) {
    out_size = sizeof(Rcomplex);
  } else {
    stop("Conversion not implemented");
  }
  
  for (int i = 0; i < n; i++) {
    conv = empty;
    memcpy(&conv, src + i * dt.byte_size, dt.byte_size);
    convert_data_inv(&conv, dt, TYPEOF(result), dest + i*out_size);
  }
  
  return result;
}

void convert_data_inv(conversion_t *input, blosc_dtype dtype, int rtype, uint8_t *output) {
  if (rtype == LGLSXP) {
    if (dtype.main_type == 'b' && dtype.byte_size == 1) {
      int b = (int)(*input).b1;
      memcpy(output, &b, sizeof(int));
    } else {
      stop("Conversion not implemented");
    }
  } else if (rtype == INTSXP) {
    int i = 0;
    if (dtype.main_type == 'i' && dtype.byte_size == 1) {
      i = (int)(*input).i1;
    } else if (dtype.main_type == 'i' && dtype.byte_size == 2) {
      i = (int)(*input).i2;
    } else if (dtype.main_type == 'i' && dtype.byte_size == 4) {
      i = (int)(*input).i4;
    } else if (dtype.main_type == 'u' && dtype.byte_size == 1) {
      i = (int)(*input).u1;
    } else if (dtype.main_type == 'u' && dtype.byte_size == 2) {
      i = (int)(*input).u2;
    } else {
      stop("Conversion not implemented");
    }

    memcpy(output, &i, sizeof(int));
  } else if (rtype == REALSXP) {
    double d;
    if (dtype.main_type == 'i' && dtype.byte_size == 8) {
      d = (double)(*input).i8;
    } else if (dtype.main_type == 'u' && dtype.byte_size == 4) {
      d = (double)(*input).u4;
    } else if (dtype.main_type == 'u' && dtype.byte_size == 8) {
      d = (int)(*input).u8;
    } else if (dtype.main_type == 'f' && dtype.byte_size == 2) {
      float16 f = 0.0;
      memcpy((uint16_t *)&f, &(*input).f2, sizeof(float16));
      d = double(f);
    } else if (dtype.main_type == 'f' && dtype.byte_size == 4) {
      d = (double)(*input).f4;
    } else if (dtype.main_type == 'f' && dtype.byte_size == 8) {
      d = (*input).f8;
    } else {
      stop("Conversion not implemented");
    }
    memcpy(output, &d, sizeof(double));
  } else if (rtype == CPLXSXP) {
    stop("TODO");
  } else {
    stop("Conversion method not available");
  }
}

void convert_data(uint8_t *input, int rtype, int n, blosc_dtype dtype, uint8_t *output) {
  conversion_t empty, conv;
  complex64 cempty;
  cempty.real = 0.0;
  cempty.imaginary = 0.0;
  empty.c16 = cempty; // <== an empty conversion type (all bits set to zero)
  for (int i = 0; i < n; i++) {
    conv = empty;
    if (rtype == LGLSXP) {
      if (dtype.main_type == 'b') {
        conv.b1 = (bool)((int *)input)[i];
      } else {
        stop("Failed to convert data");
      }
    } else if (rtype == INTSXP) {
      if (dtype.main_type == 'i') {
        conv.i8 = (int64_t)((int *)input)[i];
      } else {
        stop("Failed to convert data");
      }
    } else if (rtype == REALSXP) {
      if (dtype.main_type == 'i') {
        conv.i8 = (int64_t)((double *)input)[i];
      } else if (dtype.main_type == 'f' && dtype.byte_size == 2) {
        float16 f = ((double *)input)[i];
        conv.f2 = f.GetBits();
      } else if (dtype.main_type == 'f' && dtype.byte_size == 4) {
        conv.f4 = (float)((double *)input)[i];
      } else if (dtype.main_type == 'f' && dtype.byte_size == 8) {
        conv.f8 = ((double *)input)[i];
      } else {
        stop("Failed to convert data");
      }
    } else if (rtype == CPLXSXP) {
      if (dtype.main_type == 'c') {
        if (dtype.byte_size == 8) {
          // In R a complex number is a type consisting of two doubles (r(eal) and i(maginary))
          conv.c8.real      = (float)((double *)input)[2*i];
          conv.c8.imaginary = (float)((double *)input)[2*i + 1];
        } else if (dtype.byte_size == 16) {
          conv.c16.real      = ((double *)input)[2*i];
          conv.c16.imaginary = ((double *)input)[2*i + 1];
        } else {
          stop("Failed to convert data");
        }
      } else {
        stop("Failed to convert data");
      }
    }
    memcpy(output + i * dtype.byte_size, &conv, dtype.byte_size);
  }
}

[[cpp11::register]]
raws r_to_dtype_(sexp data, std::string dtype, sexp na_value) {
  blosc_dtype dt = prepare_dtype(dtype);

  if (!Rf_isVector(data)) stop("Input data is not a vector!");
  
  int n = LENGTH(data);
  uint8_t *ptr_in;
  if (dt.main_type == 'b' && dt.byte_size == 1) {
    data = Rf_coerceVector(data, LGLSXP);
    ptr_in = (uint8_t *)LOGICAL(data);
  } else if (dt.main_type == 'i' && dt.byte_size <= 4) {
    data = Rf_coerceVector(data, INTSXP);
    ptr_in = (uint8_t *)INTEGER(data);
  } else if(dt.main_type == 'i' && dt.byte_size >4 && dt.byte_size <= 8) {
    data = Rf_coerceVector(data, REALSXP);
    ptr_in = (uint8_t *)REAL(data);
  } else if(dt.main_type == 'u' && dt.byte_size <= 3) {
    data = Rf_coerceVector(data, INTSXP);
    ptr_in = (uint8_t *)INTEGER(data);
  } else if(dt.main_type == 'u' && dt.byte_size <= 7) {
    data = Rf_coerceVector(data, REALSXP);
    ptr_in = (uint8_t *)REAL(data);
  } else if(dt.main_type == 'f' && dt.byte_size <= 8) {
    data = Rf_coerceVector(data, REALSXP);
    ptr_in = (uint8_t *)REAL(data);
  } else if(dt.main_type == 'c' && dt.byte_size <= 16) {
    data = Rf_coerceVector(data, CPLXSXP);
    ptr_in = (uint8_t *)COMPLEX(data);
  } else {
    stop("Cannot convert data type to an R type");
  }
  writable::raws result((R_xlen_t)n*dt.byte_size);
  uint8_t * ptr = (uint8_t *)(RAW(as_sexp(result)));
  convert_data(ptr_in, TYPEOF(data), n, dt, ptr);
  if (dt.needs_byteswap) byte_swap(ptr, dt, n);
  return result;
}

void byte_swap(uint8_t * data, blosc_dtype dtype, uint32_t n) {
  warning("TODO Byte swapping need more testing");
  int bs = dtype.byte_size;
  uint32_t n2 = n;
  if (dtype.main_type == 'c') {
    // Real and imaginary components need to be swapped individually
    bs = bs/2;
    n = n*2;
  }
  if (bs == 1) return; // Nothing to swap
  uint8_t buffer[MAX_DTYPE_SIZE];
  for (uint32_t i = 0; i < n2; i++) {
    for (int j = 0; j < bs; j++) {
      buffer[j] = data[(i + 1) * bs - j - 1];
    }
    memcpy(data + i * bs, buffer, bs);
  }
  return;
}
