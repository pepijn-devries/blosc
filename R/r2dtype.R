#' Convert from or to ZARR data types
#' 
#' Use [ZARR V2.0](https://zarr-specs.readthedocs.io/en/latest/v2/v2.0.html) data types to
#' convert between R native types and raw data.
#' 
#' One of the applications of BLOSC compression is in ZARR, which is used to store
#' n-dimensional structured data. `r_to_dtype()` and `dtype_to_r()` are convenience functions
#' that allows you to convert most common data types to R native types.
#' 
#' R natively only supports [`logical()`] (actually stored as 32 bit integer in memory),
#' [`integer()`] (signed 32 bit integers), [`numeric()`] (64 bit floating points) and [`complex()`]
#' (real and imaginary component both represented by a 64 bit floating point). R also has some
#' more complex classes, but those are generally derivatives of the aforementioned types.
#' 
#' The functions documented here will attempt to convert raw data to R types (or vice versa).
#' As not all 'dtypes' have an appropriate R type counterpart, some conversions will not
#' be possible directly and will result in an error.
#' @param x Object to be converted
#' @param dtype TODO
#' @param na_value TODO
#' @param ... Ignored
#' @returns In case of `r_to_dtype()` a vector of encoded `raw` data is returned.
#' In case of `dtype_to_r()` a vector of an R type (appropriate for the specified `dtype`)
#' is returned if possible.
#' @rdname dtype
#' @author Pepijn de Vries
#' @examples
#' ## TODO include better examples
#' r_to_dtype(1:100, "<i4")
#' 
#' r_to_dtype(c(TRUE, FALSE), "|b1")
#' dtype_to_r(as.raw(0:1), "|b1")
#' dtype_to_r(as.raw(0:20), "|i1")
#' @export
r_to_dtype <- function(x, dtype, na_value = NA, ...) {
  r_to_dtype_(x, dtype, na_value)
}

#' @rdname dtype
#' @export
dtype_to_r <- function(x, dtype, na_value = NA, ...) {
  dtype_to_r_(x, dtype, na_value)
}