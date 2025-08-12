#' @keywords internal
"_PACKAGE"
NULL

.onUnload <- function(libpath) library.dynam.unload("blosc", libpath)

## usethis namespace: start
#' @useDynLib blosc, .registration = TRUE
## usethis namespace: end
NULL
