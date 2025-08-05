#include <cpp11.hpp>

using namespace cpp11;

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
  //  half      f2; //Not supported by default libraries
  float     f4;
  double    f8;
  complex32 c8;
  complex64 c16;
};

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
    
    dt.main_type = dtype.c_str()[1]; // TODO check main_type
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
    if (bz > 255) stop("Invalid byte size");
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
  if (dtype == "<f4") {
    float *src = (float *)(RAW(as_sexp(data)));
    size_t len = data.size()/sizeof(float);
    writable::doubles d((R_xlen_t)len);
    for (uint32_t i = 0; i < len; i++) {
      double s = (double)src[i];
      // TODO what to do if the actual value equals NA_REAL but does not represent an empty value?
      // option 1 warn that unwanted NA's are introduced;
      // option 2 replace value with a very close value not equalling NA_REAL;
      if (src[i] == (float)na_value) s = NA_REAL;
      d[(int)i] = s;
    }
    return d;
  } else {
    stop("The 'dtype' '%s' is not implemented", dtype.c_str());
  }
  return R_NilValue;
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
  //TODO implement
  // https://zarr.readthedocs.io/en/stable/user-guide/data_types.html
  // https://numpy.org/doc/stable/reference/arrays.interface.html#the-array-interface-protocol
  
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
  // TODO fill the result
  uint8_t * ptr = (uint8_t *)(RAW(as_sexp(result)));
  convert_data(ptr_in, TYPEOF(data), n, dt, ptr);
  //TODO perform byte swapping if required
  return result;
}