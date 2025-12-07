# Convert from or to ZARR data types

Use [ZARR
V2.0](https://zarr-specs.readthedocs.io/en/latest/v2/v2.0.html) data
types to convert between R native types and raw data.

## Usage

``` r
r_to_dtype(x, dtype, na_value = NA, ...)

dtype_to_r(x, dtype, na_value = NA, ...)
```

## Arguments

- x:

  Object to be converted

- dtype:

  The data type used for encoding/decoding raw data. The `dtype` is a
  code consisting of at least 3 characters. The first character
  indicates the [endianness](https://en.wikipedia.org/wiki/Endianness)
  of the data: `'<'` (little-endian), `'>'` (big-endian), or `'|'`
  (endianness not relevant).

  The second character represents the main data type (`'b'` boolean
  (logical), `'i'` signed integer, `'u'` unsigned integer, `'f'`
  floating point number, `'c'` complex number). `'M'` is used for
  date-time objects and `'m'` for delta time (see
  [`difftime()`](https://rdrr.io/r/base/difftime.html)). `'S'` for UTF8
  encoded character strings and `'U'` for UTF32 encoded character
  strings.

  The following characters are numerical indicating the byte size of the
  data type. For example: `dtype = "<f4"` means a 32 bit floating point
  number; `dtype = "|b1"` means an 8 bit logical value. An exception is
  the main type `'U'`, where the number indicates the number of
  characters, where each character is represented by 4 bytes.

  The main types `'M'` and `'m'` should always be ended with the time
  unit between square brackets for storing the date time (difference). A
  valid code would be `"<M8[h]`.

  For more details about dtypes see [ZARR
  V2.0](https://zarr-specs.readthedocs.io/en/latest/v2/v2.0.html) or
  [`vignette("dtypes")`](https://pepijn-devries.github.io/blosc/articles/dtypes.md).

- na_value:

  When storing raw data, you may want to reserve a value to represent
  missing values. This is also what `R` does for `NA` values. Other
  software may use different values to represent missing values. Also,
  some data types have insufficient storage capacity to store `R` `NA`
  values.

  Therefore, you can use this argument to indicate which value should
  represent missing values. By default it uses `R` `NA`. When set to
  `NULL`, missing values are just processed as is, without any further
  notice or warning.

  For more details see
  [`vignette("dtypes")`](https://pepijn-devries.github.io/blosc/articles/dtypes.md).

- ...:

  Ignored

## Value

In case of `r_to_dtype()` a vector of encoded `raw` data is returned. In
case of `dtype_to_r()` a vector of an R type (appropriate for the
specified `dtype`) is returned if possible.

## Details

One of the applications of BLOSC compression is in ZARR, which is used
to store n-dimensional structured data. `r_to_dtype()` and
`dtype_to_r()` are convenience functions that allows you to convert most
common data types to R native types.

R natively only supports
[`logical()`](https://rdrr.io/r/base/logical.html) (actually stored as
32 bit integer in memory),
[`integer()`](https://rdrr.io/r/base/integer.html) (signed 32 bit
integers), [`numeric()`](https://rdrr.io/r/base/numeric.html) (64 bit
floating points) and [`complex()`](https://rdrr.io/r/base/complex.html)
(real and imaginary component both represented by a 64 bit floating
point). R also has some more complex classes, but those are generally
derivatives of the aforementioned types.

The functions documented here will attempt to convert raw data to R
types (or vice versa). As not all 'dtypes' have an appropriate R type
counterpart, some conversions will not be possible directly and will
result in an error.

For more details see
[`vignette("dtypes")`](https://pepijn-devries.github.io/blosc/articles/dtypes.md).

## Author

Pepijn de Vries

## Examples

``` r
## Encode volcano data to 16 bit floating point values
volcano_encoded <-
  r_to_dtype(volcano, dtype = "<f2")

## Decode the volcano format to its original
volcano_reconstructed <-
  dtype_to_r(volcano_encoded, dtype = "<f2")

## The reconstruction is the same as its original:
all(volcano_reconstructed == volcano)
#> [1] TRUE

## Encode a numeric sequence with a missing value represented by -999
r_to_dtype(c(1, 2, 3, NA, 4), dtype = "<i2", na_value = -999)
#>  [1] 01 00 02 00 03 00 19 fc 04 00
```
