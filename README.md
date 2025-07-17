
# blosc

<!-- badges: start -->

[![R-CMD-check](https://github.com/pepijn-devries/blosc/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/pepijn-devries/blosc/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

*Status: experimental*. A package that ports BLOSC compressors and
decompressors to R.

## Installation

You can install the development version of blosc from
[GitHub](https://github.com/) with:

``` r
# install.packages("pak")
pak::pak("pepijn-devries/blosc")
```

## Example

TODO

``` r
library(blosc)

chunk_dat <-
  "https://s3.waw3-1.cloudferro.com/mdl-arco-geo-014/arco/GLOBAL_ANALYSISFORECAST_PHY_001_024/cmems_mod_glo_phy_anfc_0.083deg_PT1H-m_202406/geoChunked.zarr/thetao/8.0.99.137" |>
  httr2::request() |>
  httr2::req_perform() |>
  httr2::resp_body_raw()

## decompress downloaded data:
chunk_decomp <- blosc:::blosc_decompress(chunk_dat)
## recompress it:
chunk_recomp <- blosc_compress(chunk_decomp)

zarray <-
  "https://s3.waw3-1.cloudferro.com/mdl-arco-geo-014/arco/GLOBAL_ANALYSISFORECAST_PHY_001_024/cmems_mod_glo_phy_anfc_0.083deg_PT1H-m_202406/geoChunked.zarr/thetao/.zarray" |>
  httr2::request() |>
  httr2::req_perform() |>
  httr2::resp_body_json(check_type = FALSE)

zattrs <-
  "https://s3.waw3-1.cloudferro.com/mdl-arco-geo-014/arco/GLOBAL_ANALYSISFORECAST_PHY_001_024/cmems_mod_glo_phy_anfc_0.083deg_PT1H-m_202406/geoChunked.zarr/thetao/.zattrs" |>
  httr2::request() |>
  httr2::req_perform() |>
  httr2::resp_body_json(check_type = FALSE)

## decode zarr dtype to R types
chunk_data <- blosc:::dtype_to_r(chunk_decomp, "<f4", zarray$fill_value)
hist(chunk_data)
```

<img src="man/figures/README-example-1.png" width="100%" />

``` r

zarr_dims <-
  structure(zarray$chunks |> unlist(), names = zattrs$`_ARRAY_DIMENSIONS` |> unlist())
if (zarray$order == "C") zarr_dims <- rev(zarr_dims)

ar <- array(chunk_data, dim = zarr_dims)

## plot spatial slice at first timestamp
image(ar[,,1,1])
```

<img src="man/figures/README-example-2.png" width="100%" />

``` r

## plot time-series
plot(ar[1,1,1,], type = "l")
```

<img src="man/figures/README-example-3.png" width="100%" />
