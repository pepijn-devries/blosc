# Information about compressed data

Obtain information about raw data compressed with blosc.

## Usage

``` r
blosc_info(x, ...)
```

## Arguments

- x:

  Raw data compressed with
  [`blosc_compress()`](https://pepijn-devries.github.io/blosc/reference/blosc.md).

- ...:

  Ignored

## Value

Returns a named list with information about blosc compressed data `x`.

## Examples

``` r
data_compressed <-
  blosc_compress(volcano, typesize = 2, dtype = "<i2", compressor = "lz4",
                 shuffle = "bitshuffle")

blosc_info(data_compressed)
#> $Compressor
#> [1] "LZ4"
#> 
#> $`Blosc format version`
#> [1] 2
#> 
#> $`Internal compressor version`
#> [1] 1
#> 
#> $`Type size in bytes`
#> [1] 2
#> 
#> $`Block size in bytes`
#> [1] 10614
#> 
#> $`Uncompressed size in bytes`
#> [1] 10614
#> 
#> $`Compressed size in bytes`
#> [1] 6834
#> 
#> $Shuffle
#> [1] FALSE
#> 
#> $`Pure memcpy`
#> [1] FALSE
#> 
#> $`Bit shuffle`
#> [1] TRUE
#> 
#> attr(,"class")
#> [1] "blosc_info" "list"      
```
