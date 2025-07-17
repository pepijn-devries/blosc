#' Compress with BLOSC
#' 
#' TODO
#' @param x TODO
#' @param compressor todo 
#' @param level TODO
#' @param shuffle TODO
#' @param typesize TODO
#' @param ... Ignored
#' @examples
#' my_dat      <- as.raw(sample.int(255, 10*1024, replace = TRUE))
#' con         <- rawConnection(my_dat)
#' my_dat_out1 <- blosc_compress(my_dat)
#' #my_dat_out2 <- blosc_compress(con) #TODO
#' 
#' @export
blosc_compress <- function(x, compressor = "lz4", level = 7,
                           shuffle = "shuffle", typesize = 4, ...) {
  compressor_args <- c("blosclz", "lz4", "lz4hc", "zlib", "zstd")
  compressor <- match.arg(compressor, compressor_args)
  
  shuffle_args <- c("noshuffle", "shuffle", "bitshuffle")
  shuffle <- match.arg(shuffle, shuffle_args)
  shuffle <- match(shuffle, shuffle_args) - 1
  typesize <- as.integer(typesize)
  if (typesize < 1L || typesize > 255L)
    stop("Argument 'typesize' out of range (1-255)")
  level <- as.integer(level)
  if (level < 0L || level > 9L)
    stop("Compression level should be between 0 (no compression) and 9 (max compression)")
  
  if (inherits(x, "raw")) {
    blosc_compress_dat(x, compressor, level, shuffle, typesize)
  } else if (inherits(x, "connection")) {
    ## TODO make sure connection is readable and binary
    blosc_compress_con(x, compressor, level, shuffle, typesize)
  } else {
    stop("Cannot compress data of type '%s'", typeof(x))
  }
}