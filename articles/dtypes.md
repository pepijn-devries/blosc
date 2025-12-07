# Data Types

## What are Data Types (and Why Should You Care?)

A data type is a description of how information is stored digitally and
in which format. In the context of BLOSC compression this is relevant as
it is used to compress arrays of structured data. How this data is
structured is described by the data type.

For the use of BLOSC in R, this is also relevant, because `R` (by
design) provides access to a limited number of data types, most
importantly: [`raw()`](https://rdrr.io/r/base/raw.html),
[`logical()`](https://rdrr.io/r/base/logical.html),
[`integer()`](https://rdrr.io/r/base/integer.html),
[`numeric()`](https://rdrr.io/r/base/numeric.html) and
[`complex()`](https://rdrr.io/r/base/complex.html). Below you will find
a table of typical storage formats and how these are converted to `R`
types.

Therefore, you probably need to convert the data type of stored data to
something that can be handled in `R` (or vice versa). For your
convenience the functions
[`r_to_dtype()`](https://pepijn-devries.github.io/blosc/reference/dtype.md)
and
[`dtype_to_r()`](https://pepijn-devries.github.io/blosc/reference/dtype.md)
handle such conversions. Note that these functions do not provide
exhaustive features, but are meant to handle most common conversions.

## Specification Version

The package at hand uses [version
2](https://zarr-specs.readthedocs.io/en/latest/v2/v2.0.html) of data
type specifications, while they are superseded by [version
3](https://zarr-specs.readthedocs.io/en/latest/v3/data-types/index.html).
Why is this?

The old version is used as it still includes the endianness in its
encoding and is more compact. You can use dedicated
[Zarr](https://cran.r-project.org/package=zarr) interpreters to handle
more recent format specifications.

## Overview of Data Types

Data types are represented by a code where the first character reflects
the byte order of the data (see Wikipedia article about
[Endianness](https://en.wikipedia.org/wiki/Endianness)). The second
character reflects the main type of the data (such as integer, or
floating point). The following numerical characters indicate the size
(in bytes) of each element. For data types `M` (date time) and `m`
(delta time), the specification also includes the unit of time used to
store the information.

The table below shows an overview of common types, how the are converted
from and to `R` types, and some important notes to consider while
converting data.

|                                           |                                           |     |                                                          |                                                                                                                                                                                                                        |
|-------------------------------------------|-------------------------------------------|-----|----------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **dtype code**                            | **Alternative notation**                  |     | **R type**                                               | **Notes**                                                                                                                                                                                                              |
| `|b1`                                     | 8 bit boolean                             |     | [`logical()`](https://rdrr.io/r/base/logical.html)       | In `R` logical values are actually stored as a 32 bit integer.                                                                                                                                                         |
| `|i1`, `<i2`, `>i2`, `<i4`, `>i4`         | 8 bit, 16 bit and 32 bit signed integers. |     | [`integer()`](https://rdrr.io/r/base/integer.html)       |                                                                                                                                                                                                                        |
| `|u1`, `<u2`, `>u2`                       | 8 bit and 16 bit unsigned integers        |     | [`integer()`](https://rdrr.io/r/base/integer.html)       |                                                                                                                                                                                                                        |
| `<u4`, `>u4`, `<u8`, `>u8`                | 32 and 64 bit unsigned integers           |     | [`numeric()`](https://rdrr.io/r/base/numeric.html)       | Not all numbers of these types can be adequately represented by neither R’s [`numeric()`](https://rdrr.io/r/base/numeric.html) nor [`integer()`](https://rdrr.io/r/base/integer.html). Handle these types with caution |
| `<i8`, `>i8`                              | 64 bit signed integers                    |     | [`numeric()`](https://rdrr.io/r/base/numeric.html)       | Not all numbers of these types can be adequately represented by neither R’s [`numeric()`](https://rdrr.io/r/base/numeric.html) nor [`integer()`](https://rdrr.io/r/base/integer.html). Handle these types with caution |
| `<f2`, `>f2`, `<f4`, `>f4`, `<f8`, `>f8`, | 16, 32 and 64 bit floating point numbers  |     | [`numeric()`](https://rdrr.io/r/base/numeric.html)       |                                                                                                                                                                                                                        |
| `<c8`, `>c8`, `<c16`, `>c16`,             | 64 bit and 128 bit complex numbers        |     | [`complex()`](https://rdrr.io/r/base/complex.html)       |                                                                                                                                                                                                                        |
| `<M8[*]` `>M8[*]` where \*=unit           | 64 bit date time object                   |     | [`as.POSIXct()`](https://rdrr.io/r/base/as.POSIXlt.html) | Note that the `dtype` stores the time unit as a 64 bit integer, whereas POSIXct stores the object as a `double`. Use with caution                                                                                      |
| `<m8[*]` `>m8[*]` where \*=unit           | 64 bit delta time object                  |     | [`difftime()`](https://rdrr.io/r/base/difftime.html)     | Note that the `dtype` stores the time unit as a 64 bit integer, whereas `difftime` stores the object as a `double`. Use with caution                                                                                   |
| `|S*`                                     | UTF8 string                               |     | [`character()`](https://rdrr.io/r/base/character.html)   |                                                                                                                                                                                                                        |
| `|U*`                                     | UTF32 string                              |     | [`character()`](https://rdrr.io/r/base/character.html)   | Note that UTF8 encoding is used in the conversion process. Hence, information may get lost. Use with caution                                                                                                           |

Some examples of encoding r data to dtypes

``` r
library(blosc)
r_to_dtype(c(TRUE, FALSE), "|b1")
#> [1] 01 00
r_to_dtype(1L:4L, "|u1")
#> [1] 01 02 03 04
r_to_dtype(c(1.4, 9.8e-6), "<f8")
#>  [1] 66 66 66 66 66 66 f6 3f 33 76 78 be 55 8d e4 3e
r_to_dtype(1+1i, "<c16")
#>  [1] 00 00 00 00 00 00 f0 3f 00 00 00 00 00 00 f0 3f
r_to_dtype(as.POSIXct("2023-06-23 15:32:19", tz = "UTC"), "<M8[ms]")
#> [1] b8 83 e2 e8 88 01 00 00
r_to_dtype(as.difftime(1, units = "weeks"), "<m8[D]")
#> [1] 07 00 00 00 00 00 00 00
r_to_dtype(c("foo", "bar", "foobar"), "|S6")
#>  [1] 66 6f 6f 00 00 00 62 61 72 00 00 00 66 6f 6f 62 61 72
```

## Precision of Types

Beware that when encoding `R` types to a `dtype`, you may lose
precision. You will receive no notification, so it is your own
responsibility. Loss of precision happens when the data type you use for
encoding is less precise than `R`’s native type. For instance when you
encode an `R` `numeric` (64 bit floating point) to dtype `"<f2"` (16 bit
floating point). This will become apparent when you convert your dtype
back to an `R` type.

Some examples where you will lose precision:

``` r
## Encoding numeric (64 bit) as a 16 bit float:
r_to_dtype(0.123, "<f2") |>
  dtype_to_r("<f2")
#> [1] 0.1229858

## Encoding a date-time object in whole hours
## as opposed to a floating point of seconds
r_to_dtype(as.POSIXct("2024-05-31 19:58:01", tz = "UTC"), "<M8[h]") |>
  dtype_to_r("<M8[h]")
#> [1] "2024-05-31 19:00:00 UTC"
```

Note that you will always need to use an identical `dtype` to
back-transform encoded data. Otherwise you will get nonsensical results.

## Missing Values

When storing raw data, you may want to reserve a value to represent
missing values. This is also what `R` does for `NA` values. Other
software may use different values to represent missing values. Also,
some data types have insufficient storage capacity to store `R` `NA`
values

The examples below show how you can use custom values to represent
missing values.

``` r
## As `na_value` is not specified for `dtype_to_r()`
## and the NA value is masked to 8 bit, the `NA`
## value is mistakenly interpreted as `TRUE`
r_to_dtype(c(TRUE, NA, FALSE, TRUE), "|b1") |>
  dtype_to_r("|b1", na_value = NA_integer_)
#> [1]  TRUE  TRUE FALSE  TRUE

## This can be fixed by specifying `na_value`
r_to_dtype(c(TRUE, NA, FALSE, TRUE), "|b1", na_value = -1) |>
  dtype_to_r("|b1", na_value = -1)
#> [1]  TRUE    NA FALSE  TRUE

## If the `na_value` is not specified for `dtype_to_r()`,
## it will be taken literally
r_to_dtype(c(1, NA, 4, 5), "<i4", na_value = -999) |>
  dtype_to_r("<i4")
#> [1]    1 -999    4    5

## If the `na_value` is specified for `dtype_to_r()`,
## it will be interpreted as NA
r_to_dtype(c(1, NA, 4, 5), "<i4", na_value = -999) |>
  dtype_to_r("<i4", na_value = -999)
#> [1]  1 NA  4  5
```
