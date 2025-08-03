#include <cpp11.hpp>

using namespace cpp11;

typedef struct {
  bool needs_byteswap;
  char main_type;
  uint8_t byte_size;
} blosc_dtype;

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
  Rprintf("TODO byte size %i\n", bz);
  if (bz > 255) stop("Invalid byte size");
  dt.byte_size = (uint8_t)bz;
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

void convert_data(uint8_t *input, int in_size, int n, blosc_dtype dtype, uint8_t *output) {
  Rprintf("TODO converting %i %i\n", in_size, n);
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < in_size; j++) {
      Rprintf("byte '%i' ", (input + i*in_size + j)[0]);
    }
    Rprintf(" TODO\n");
  }
  // uint32_t current_type = TYPEOF(input);
  // if (current_type == LGLSXP) {
  //   Rprintf("TODO cur logical\n");
  // } else if (current_type == INTSXP) {
  //   Rprintf("TODO cur int\n");
  // } else if (current_type == REALSXP) {
  //   Rprintf("TODO cur real\n");
  // } else if (current_type == CPLXSXP) {
  //   Rprintf("TODO cur complex\n");
  // }
  
}

[[cpp11::register]]
raws r_to_dtype_(sexp data, std::string dtype, sexp na_value) {
  blosc_dtype dt = prepare_dtype(dtype);
  //TODO implement
  // https://zarr.readthedocs.io/en/stable/user-guide/data_types.html
  // https://numpy.org/doc/stable/reference/arrays.interface.html#the-array-interface-protocol

  if (!Rf_isVector(data)) stop("Input data is not a vector!");
  
  int n = LENGTH(data);
  int in_size = 0;
  uint8_t *ptr_in;
  if (dt.main_type == 'b' && dt.byte_size == 1) {
    data = Rf_coerceVector(data, LGLSXP);
    ptr_in = (uint8_t *)LOGICAL(data);
    in_size = sizeof(int);
  } else if (dt.main_type == 'i' && dt.byte_size <= 4) {
    data = Rf_coerceVector(data, INTSXP);
    ptr_in = (uint8_t *)INTEGER(data);
    in_size = sizeof(int);
  } else if(dt.main_type == 'i' && dt.byte_size >4 && dt.byte_size <= 8) {
    data = Rf_coerceVector(data, REALSXP);
    ptr_in = (uint8_t *)REAL(data);
    in_size = sizeof(double);
  } else if(dt.main_type == 'u' && dt.byte_size <= 3) {
    data = Rf_coerceVector(data, INTSXP);
    ptr_in = (uint8_t *)INTEGER(data);
    in_size = sizeof(int);
  } else if(dt.main_type == 'u' && dt.byte_size <= 7) {
    data = Rf_coerceVector(data, REALSXP);
    ptr_in = (uint8_t *)REAL(data);
    in_size = sizeof(double);
  } else if(dt.main_type == 'f' && dt.byte_size <= 8) {
    data = Rf_coerceVector(data, REALSXP);
    ptr_in = (uint8_t *)REAL(data);
    in_size = sizeof(double);
  } else if(dt.main_type == 'c' && dt.byte_size <= 8) {
    data = Rf_coerceVector(data, CPLXSXP);
    ptr_in = (uint8_t *)COMPLEX(data);
    in_size = sizeof(Rcomplex);
  } else {
    stop("Cannot convert data type to an R type");
  }
  writable::raws result((R_xlen_t)n*dt.byte_size);
  // TODO fill the result
  uint8_t * ptr = (uint8_t *)(RAW(as_sexp(result)));
  convert_data(ptr_in, in_size, n, dt, ptr);
  return result;
}