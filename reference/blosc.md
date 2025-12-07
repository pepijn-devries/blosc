# Compress and decompress with Blosc

Use the Blosc library to compress or decompress data.

## Usage

``` r
blosc_compress(
  x,
  compressor = "blosclz",
  level = 7L,
  shuffle = "noshuffle",
  typesize = 4L,
  ...
)

blosc_decompress(x, ...)
```

## Arguments

- x:

  In case of `blosc_decompress()`, `x` should always be `raw` data to be
  decompressed. Use `...` arguments to convert decompressed data to a
  specific data type.

  In case of `blosc_compress()`, `x` should either be `raw` data or a
  `vector` of data to be compressed. In the latter case, you need to
  specify `dtype` (see
  [`r_to_dtype()`](https://pepijn-devries.github.io/blosc/reference/dtype.md))
  in order to convert the data to `raw` information first. See
  [`vignette("blosc-compression")`](https://pepijn-devries.github.io/blosc/articles/blosc-compression.md)
  for more details.

- compressor:

  The compression algorithm to be used. Can be any of `"blosclz"`,
  `"lz4"`, `"lz4hc"`, `"zlib"`, or `"zstd"`.

- level:

  An `integer` indicating the required level of compression. Needs to be
  between `0` (no compression) and `9` (maximum compression).

- shuffle:

  A shuffle filter to be activated before compression. Should be one of
  `"noshuffle"`, `"shuffle"`, or `"bitshuffle"`.

- typesize:

  BLOSC compresses arrays of structured data. This argument specifies
  the size (`integer`) of the data structure / type in bytes. Default is
  `4L` bytes (i.e. 32 bits), which would be suitable for compressing 32
  bit integers.

- ...:

  Arguments passed to
  [`r_to_dtype()`](https://pepijn-devries.github.io/blosc/reference/dtype.md).

## Value

In case of `blosc_compress()` a vector of compressed `raw` data is
returned. In case of `blosc_decompress()` returns a vector of
decompressed `raw` data. Or in in case `dtype` (see
[`dtype_to_r()`](https://pepijn-devries.github.io/blosc/reference/dtype.md))
is specified, a vector of the specified type is returned.

## Examples

``` r
my_dat        <- as.raw(sample.int(2L, 10L*1024L, replace = TRUE) - 1L)
my_dat_out    <- blosc_compress(my_dat, typesize = 1L)
my_dat_decomp <- blosc_decompress(my_dat_out)

## After compressing and decompressing the data is the same as the original:
all(my_dat == my_dat_decomp)
#> [1] TRUE
```
