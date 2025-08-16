#include <cpp11.hpp>
#include <regex>
#include "umHalf.h"

using namespace cpp11;

// Careful : days_in_year is for base-0 years, days_in_month for base-1970.
#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)
#define days_in_year(year) (isleap(year) ? 366 : 365)
#define days_in_month(mon, yr) ((mon == 1 && isleap(1900+yr)) ? 29 : month_days[mon])
#define MAX_DTYPE_SIZE 255

static const int month_days[12] =
  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static const char *dt_units[] = {
 "W", "D", "h", "m", "s", "ms", "us", "μs", "ns", "ps", "fs", "as"
};

static const double to_seconds[] {
  604800, 86400, 3600, 60, 1, 1e-3, 1e-6, 1e-6, 1e-9, 1e-12, 1e-15, 1e-18
};

typedef struct {
  bool needs_byteswap;
  char main_type;
  uint8_t byte_size;
  std::string unit;
  double unit_conversion;
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

void convert_data(uint8_t *input, int rtype, int n, blosc_dtype dtype,
                  uint8_t *output, sexp na_value);
bool convert_data_inv(conversion_t *input, blosc_dtype dtype,
                      int rtype, uint8_t *output, sexp na_value);
void byte_swap(uint8_t * data, blosc_dtype dtype, uint32_t n);

void getYM(double d, int64_t &mon, int64_t &Y) {
  bool valid = R_FINITE(d) != 0;
  int64_t y = 1970, tmp;

  if(valid) {
    /* every 400 years is exactly 146097 days long and the
     pattern is repeated */
    double rounds = floor(floor(d) / 146097.0);
    int64_t day = (int64_t) (floor(d) - 146097.0 * rounds);

    /* year & day within year */
    if (day >= 0)
      for ( ; day >= (tmp = days_in_year(y)); day -= tmp, y++);
    else
      for ( ; day < 0; --y, day += days_in_year(y) );
    // Avoid overflows
    double year0 =  y - 1900 + rounds * 400;
    y = (int64_t)year0;
    /* month within year */
    for (mon = 0;
         day >= (tmp = days_in_month(mon, y));
         day -= tmp, mon++);
  } else {
    stop("Invalid date time");
  }
  mon++;
  Y = y + 1900;
}

blosc_dtype prepare_dtype(std::string dtype) {
  blosc_dtype dt;
  int dlen = dtype.length();
  if (dlen < 3 || dlen > 9) stop("'dtype should be between 3 and 9 character long!");
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
    int i;
    for (i = 2; i < (int)dtype.length(); i++) {
      int s = dtype.c_str()[i] - '0';
      if (s < 0 || s > 9) break;
      bz *= 10;
      bz += s;
    }
    if (bz < 1 || bz > MAX_DTYPE_SIZE) stop("Invalid byte size");
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
    if (dt.main_type == 'M' && dt.byte_size != 8)
      stop("Unknown data type '%s'", dtype.c_str());
    
    dt.unit = "";
    std::smatch match;
    std::regex rgx("\\[.*?\\]$");
    if (std::regex_search(dtype, match, rgx)) {
      dt.unit = match[0];
      if (dt.unit.length() < 3) stop("Invalid unit");
      dt.unit.erase(0, 1);
      dt.unit.erase(dt.unit.length() - 1, 1);
    }
    
    dt.unit_conversion = -1;
    if (dt.unit.length() > 0) {
      if (dt.main_type == 'M') {
        for (int j = 0; j < (int)std::size(dt_units); j++) {
          if (dt.unit == dt_units[j]) {
            dt.unit_conversion = to_seconds[j];
            break;
          }
        }
      } else {
        warning("Unknown unit");
      }
    }

    return dt;
}

[[cpp11::register]]
list dtype_to_list_(std::string dtype) {
  blosc_dtype dt = prepare_dtype(dtype);
  writable::integers bs((R_xlen_t)1);
  bs[0] = (int)dt.byte_size;
  writable::logicals nb((R_xlen_t)1);
  nb[0] = dt.needs_byteswap;
  writable::strings mt((R_xlen_t)1);
  mt[0] = (r_string)std::string(1, dt.main_type);
  writable::strings ut((R_xlen_t)1);
  ut[0] = dt.unit;
  writable::doubles uc((R_xlen_t)1);
  uc[0] = dt.unit_conversion;
  writable::list result({ bs, nb, mt, ut, uc });
  result.attr("names") = writable::strings({
    "byte_size",
    "needs_byteswap",
    "main_type",
    "unit",
    "unit_conversion"
  });
  return result;
}

int64_t numdays(int64_t y, int64_t m, int64_t d) {
  m = (m + 9) % 12;
  y = y - m/10;
  return 365*y + y/4 - y/100 + y/400 + (m*306 + 5)/10 + ( d - 1 );
}

[[cpp11::register]]
sexp dtype_to_r_(raws data, std::string dtype, sexp na_value) {
  blosc_dtype dt = prepare_dtype(dtype);
  if (data.size() % dt.byte_size != 0)
    stop("Raw data size needs to be multitude of data type size");
  conversion_t conv, empty;
  complex64 cempty;
  cempty.real = 0.0;
  cempty.imaginary = 0.0;
  empty.c16 = cempty;
  conv = empty;
  int64_t bigint = 0;
  int n = data.size() / dt.byte_size, mult_factor = 1;
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
  } else if((dt.main_type == 'f' || dt.main_type == 'M') &&
    dt.byte_size <= 8) {
    result = writable::doubles((R_xlen_t) n);
    dest = (uint8_t *)REAL(result);
  } else if(dt.main_type == 'c' && dt.byte_size <= 16) {
    mult_factor = 2;
    result = writable::doubles((R_xlen_t) 2 * n);
    dest = (uint8_t *)REAL(result);
  } else if (dt.main_type == 'M' && dt.byte_size == 8) {
    result = writable::doubles((R_xlen_t) n);
    dest = (uint8_t *)REAL(result);
  } else {
    stop("Cannot convert data type to an R type");
  }
  uint32_t out_size = 0;
  if (TYPEOF(result) == INTSXP || TYPEOF(result) == LGLSXP) {
    out_size = sizeof(int);
  } else if (TYPEOF(result) == REALSXP) {
    out_size = sizeof(double);
  } else {
    stop("Conversion not implemented");
  }
  
  bool warn = false;
  for (int i = 0; i < mult_factor * n; i++) {
    conv = empty;
    memcpy(&conv, src + i * dt.byte_size / mult_factor, dt.byte_size / mult_factor);
    bool should_warn =
      convert_data_inv(&conv, dt, TYPEOF(result), dest + i*out_size, na_value);
    if (should_warn) warn = true;
  }
  
  if (dt.main_type == 'c') {
    // in case of 'c' convert doubles to complex vector
    // This could be simplified once `cpp11` implements Rcomplex vectors
    sexp c = PROTECT(Rf_allocVector(CPLXSXP, n));
    uint8_t * cptr = (uint8_t *)COMPLEX(c);
    memcpy(cptr, dest, n * sizeof(Rcomplex));
    UNPROTECT(1);
    return c;
  }
  if (dt.main_type == 'M') {
    result.attr("class") = writable::strings({"POSIXct", "POSIXt"});
    result.attr("tzone") = writable::strings((r_string)"UTC");
    double *d = REAL(result);
      
    for (int j = 0; j < n; j++) {
      memcpy(&bigint, (int64_t *)(&(d[j])), sizeof(double));
      if (dt.unit_conversion > 0) {
        d[j] = ((double)bigint)*dt.unit_conversion;
      } else {
        
        if (dt.unit == "Y") {
          d[j] = (double)(numdays(1970 + bigint, 1, 1) - numdays(1970, 1, 1)) *
            86400;
        } else if (dt.unit == "M") {
          d[j] = (double)(numdays(1970 + bigint/12, bigint%12 + 1, 1) -
            numdays(1970, 1, 1)) * 86400;
          
        } else {
          stop("Unit conversion not possible/implemented");
        }
      }
    }
  }
  if (warn) warning("Data contains values equal to R's NA representation");
  return result;
}

sexp check_na(sexp na_value, int rtype) {
  sexp result;
  if (!Rf_isNull(na_value)) {
    if (!Rf_isVector(na_value)) stop("Invalid NA value");
    int rt = rtype;
    if (rtype == LGLSXP) rt = INTSXP;
    result = Rf_coerceVector(na_value, rt);
    if (LENGTH(result) != 1) stop("Invalid NA value");
  }
  return result;
}

bool convert_data_inv(conversion_t *input, blosc_dtype dtype,
                      int rtype, uint8_t *output, sexp na_value) {
  bool ignore_na = Rf_isNull(na_value), warn_na = false;
  na_value = check_na(na_value, rtype);
  
  if (rtype == LGLSXP) {
    if (dtype.main_type == 'b' && dtype.byte_size == 1) {
      int b = (int)(*input).b1;
      if (!ignore_na) {
        int nval = INTEGER(na_value)[0];
        if (nval != NA_INTEGER) nval = 0xff & nval;
        if (b == NA_INTEGER && !ISNA(na_value)) warn_na = true;
        if (b ==(0xff & INTEGER(na_value)[0])) b = NA_LOGICAL;
      }
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
    
    if (!ignore_na) {
      int nval = INTEGER(na_value)[0];
      if (i == NA_INTEGER && nval != NA_INTEGER) warn_na = true;
      if (i == nval) i = NA_INTEGER;
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
    } else if ((dtype.main_type == 'f' || dtype.main_type == 'M') &&
      dtype.byte_size == 8) {
      d = (*input).f8;
    } else if (dtype.main_type == 'c' && dtype.byte_size == 8) {
      d = (double)(*input).f4;
    } else if (dtype.main_type == 'c' && dtype.byte_size == 16) {
      d = (*input).f8;
    } else {
      stop("Conversion not implemented");
    }
    
    if (!ignore_na) {
      double nval = REAL(na_value)[0];
      if (dtype.main_type == 'M') {
        int64_t na_cor;
        memcpy(&na_cor, (int64_t *)(&NA_REAL), sizeof(double));
        
        int64_t bigint = nval / dtype.unit_conversion;
        memcpy(&nval, (double *)(&bigint), sizeof(double));
        if ((*input).i8 == na_cor && bigint != na_cor) warn_na = true;
        if ((*input).i8 == bigint) d = NA_REAL;
      } else {
        if (R_IsNA(d) && !R_IsNA(nval)) warn_na = true;
        if (d == nval) d = NA_REAL;
      }
    }
    
    memcpy(output, &d, sizeof(double));
  } else if (rtype == CPLXSXP) {
    stop("This type should not occure as it is coded as REALs at this stage");
  } else {
    stop("Conversion method not available");
  }
  return warn_na;
}

void convert_data(uint8_t *input, int rtype, int n,
                  blosc_dtype dtype, uint8_t *output, sexp na_value) {
  na_value = check_na(na_value, rtype);
  bool warn_na = false, ignore_na = Rf_isNull(na_value);
  conversion_t empty, conv;
  complex64 cempty;
  cempty.real = 0.0;
  cempty.imaginary = 0.0;
  empty.c16 = cempty; // <== an empty conversion type (all bits set to zero)
  int64_t bigint = 0;
  for (int i = 0; i < n; i++) {
    conv = empty;
    if (rtype == LGLSXP) {
      if (dtype.main_type == 'b') {
        
        if (!ignore_na && ((int *)input)[i] == NA_LOGICAL)
          conv.i1 = 0xff & INTEGER(na_value)[0]; else
            conv.b1 = (bool)((int *)input)[i];
          if (!ignore_na && ((int *)input)[i] == (0xff & INTEGER(na_value)[0]))
            warn_na = true;
          
      } else {
        stop("Failed to convert data");
      }
    } else if (rtype == INTSXP) {
      if (dtype.main_type == 'i') {
        
        if (!ignore_na && ((int *)input)[i] == NA_INTEGER)
          conv.i8 = (int64_t)INTEGER(na_value)[0]; else {
            conv.i8 = (int64_t)((int *)input)[i];
            if (!ignore_na && ((int *)input)[i] == INTEGER(na_value)[0])
              warn_na = true;
          }
          
      } else {
        stop("Failed to convert data");
      }
    } else if (rtype == REALSXP) {
      if (dtype.main_type == 'i') {
        
        if (!ignore_na && R_IsNA(((double *)input)[i]))
          conv.i8 = (int64_t)REAL(na_value)[0]; else {
            conv.i8 = (int64_t)((double *)input)[i];
            if (!ignore_na && ((double *)input)[i] == REAL(na_value)[0])
              warn_na = true;
          }
          
      } else if (dtype.main_type == 'f' && dtype.byte_size == 2) {
        
        float16 f;
        
        if (!ignore_na && R_IsNA(((double *)input)[i]))
          f = REAL(na_value)[0]; else {
            f = ((double *)input)[i];
            if (!ignore_na && ((double *)input)[i] == REAL(na_value)[0])
              warn_na = true;
          }
          
          conv.f2 = f.GetBits();
          
      } else if (dtype.main_type == 'f' && dtype.byte_size == 4) {
        
        if (!ignore_na && R_IsNA(((double *)input)[i]))
          conv.f4 = (float)REAL(na_value)[0]; else {
            conv.f4 = (float)((double *)input)[i];
            if (!ignore_na && ((double *)input)[i] == REAL(na_value)[0])
              warn_na = true;
          }
          
      } else if (dtype.main_type == 'f' && dtype.byte_size == 8) {
        
        if (!ignore_na && R_IsNA(((double *)input)[i]))
          conv.f8 = REAL(na_value)[0]; else {
            conv.f8 = ((double *)input)[i];
            if (!ignore_na && ((double *)input)[i] == REAL(na_value)[0])
              warn_na = true;
          }
      } else if (dtype.main_type == 'M' && dtype.byte_size == 8) {
        
        if (!ignore_na && R_IsNA(((double *)input)[i])) {
          conv.f8 = REAL(na_value)[0];
        } else {
          conv.f8 = ((double *)input)[i];
        }
        if (dtype.unit_conversion > 0) {
          bigint = conv.f8/dtype.unit_conversion;
          memcpy(&conv.f8, (double *)(&bigint), sizeof(double));
        } else {
          int64_t mon, yr;
          getYM(conv.f8/86400, mon, yr);
          yr = yr - 1970;
          if (dtype.unit == "Y") {
            memcpy(&conv.f8, (double *)(&yr), sizeof(double));
          } else if (dtype.unit == "M") {
            mon = yr*12 + mon - 1;
            memcpy(&conv.f8, (double *)(&mon), sizeof(double));

          } else {
            stop("Unable to convert unit");
          }
        }
        if (!ignore_na && conv.f8 == REAL(na_value)[0])
          warn_na = true;
        
          
      } else {
        stop("Failed to convert data");
      }
    } else if (rtype == CPLXSXP) {
      if (dtype.main_type == 'c') {
        if (dtype.byte_size == 8) {
          
          // In R a complex number is a type consisting of two doubles (r(eal) and i(maginary))
          double re = ((double *)input)[2*i];
          double im = ((double *)input)[2*i + 1];
          Rcomplex c;
          if (!ignore_na) c = COMPLEX(na_value)[0];
          if (!ignore_na && (R_IsNA(re) || R_IsNA(im))) {
            conv.c8.real = (float)c.r;
            conv.c8.imaginary = (float)c.i;
          } else {
            conv.c8.real      = (float)re;
            conv.c8.imaginary = (float)im;
            if (!ignore_na && re == c.r && im == c.i)
              warn_na = true;
          }
          
        } else if (dtype.byte_size == 16) {
          
          double re = ((double *)input)[2*i];
          double im = ((double *)input)[2*i + 1];
          Rcomplex c;
          if (!ignore_na) c = COMPLEX(na_value)[0];
          if (!ignore_na && (R_IsNA(re) || R_IsNA(im))) {
            conv.c16.real = c.r;
            conv.c16.imaginary = c.i;
          } else {
            conv.c16.real      = re;
            conv.c16.imaginary = im;
            if (!ignore_na && re == c.r && im == c.i)
              warn_na = true;
          }
          
        } else {
          stop("Failed to convert data");
        }
      } else {
        stop("Failed to convert data");
      }
    }
    memcpy(output + i * dtype.byte_size, &conv, dtype.byte_size);
  }
  if (warn_na) warning("Data contains values equal to the value representing missing values!");
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
  } else if((dt.main_type == 'f' || dt.main_type == 'M') &&
    dt.byte_size <= 8) {
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
  convert_data(ptr_in, TYPEOF(data), n, dt, ptr, na_value);
  if (dt.needs_byteswap) byte_swap(ptr, dt, n);
  return result;
}

void byte_swap(uint8_t * data, blosc_dtype dtype, uint32_t n) {
  int bs = dtype.byte_size;
  uint32_t n2 = n;
  if (dtype.main_type == 'c') {
    // Real and imaginary components need to be swapped individually
    bs = bs/2;
    n2 = n*2;
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
