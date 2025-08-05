#' Convert from or to ZARR dtypes
#' 
#' TODO description
#' @param x TODO
#' @param dtype TODO
#' @param na_value TODO
#' @param ... Ignored
#' @returns TODO
#' @rdname dtype
#' @author Pepijn de Vries
#' @examples
#' r_to_dtype(1:100, "<i4")
#' r_to_dtype(c(TRUE, FALSE), "|b1")
#' # dtype_to_r(as.raw(0:1), "|b1") #TODO implement
#' @export
r_to_dtype <- function(x, dtype, na_value = NA, ...) {
  r_to_dtype_(x, dtype, na_value)
}

#' @rdname dtype
#' @export
dtype_to_r <- function(x, dtype, na_value = NA, ...) {
  dtype_to_r_(x, dtype, na_value)
}