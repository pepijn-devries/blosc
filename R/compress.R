#' Compress and decompress with Blosc
#' 
#' Use the Blosc library to compress or decompress data.
#' @param x In case of `blosc_decompress()`, `x` should always be `raw` data
#' to be decompressed. Use `...` arguments to convert decompressed data
#' to a specific data type.
#' 
#' In case of `blosc_compress()`, `x` should either be `raw` data or a
#' `vector` of data to be compressed. In the latter case, you need to specify
#' `dtype` (see `r_to_dtype()`) in order to convert the data to `raw` information
#' first.
#' @param compressor The compression algorithm to be used. Can be any of
#' `"blosclz"`, `"lz4"`, `"lz4hc"`, `"zlib"`, or `"zstd"`.
#' @param level An `integer` indicating the required level of compression.
#' Needs to be between `0` (no compression) and `9` (maximum compression).
#' @param shuffle A shuffle filter to be activated before compression.
#' Should be one of `"noshuffle"`, `"shuffle"`, or `"bitshuffle"`.
#' @param typesize BLOSC compresses arrays of structured data. This argument
#' specifies the size (`integer`) of the data structure / type in bytes.
#' Default is `4L` bytes (i.e. 32 bits), which would be suitable for compressing
#' 32 bit integers.
#' @param ... Arguments passed to `r_to_dtype()`.
#' @returns In case of `blosc_compress()` a vector of compressed `raw`
#' data is returned. In case of `blosc_decompress()` returns a vector of
#' decompressed `raw` data. Or in in case `dtype` (see `dtype_to_r()`) is
#' specified, a vector of the specified type is returned.
#' @examples
#' my_dat        <- as.raw(sample.int(2L, 10L*1024L, replace = TRUE) - 1L)
#' my_dat_out    <- blosc_compress(my_dat, typesize = 1L)
#' my_dat_decomp <- blosc_decompress(my_dat_out)
#' 
#' ## After compressing and decompressing the data is the same as the original:
#' all(my_dat == my_dat_decomp)
#' @rdname blosc
#' @export
blosc_compress <- function(x, compressor = "blosclz", level = 7L,
                           shuffle = "noshuffle", typesize = 4L, ...) {
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
  
  if (!inherits(x, "raw")) x <- r_to_dtype(x, ...)
  blosc_compress_dat(x, compressor, level, shuffle, typesize)
}

#' @export
#' @rdname blosc
blosc_decompress <- function(x, ...) {
  blosc_decompress_dat(x)
  #TODO convert to specific type if dtype is specified
}
