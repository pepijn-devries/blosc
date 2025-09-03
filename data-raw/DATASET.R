## This script generates different raw data types for testing
## It uses the Python numpy package to generate the data
## types and stores it in a data.frame for testing
library(reticulate)
library(dplyr)

## Test if the Python environment exists. If not create it
if (!"py_env" %in% list.dirs(Sys.getenv("RETICULATE_VIRTUALENV_ROOT"),
                            full.names = FALSE, recursive = FALSE)) {
  ## install Python (could be skipped if Python is already installed):
  install_python()
  ## Set up virtual environment with Python:
  virtualenv_create(envname = "py_env")
  ## Install the required Python package:
  virtualenv_install("py_env", packages = c("numpy"))
}

use_virtualenv("py_env")
data_types <-
  dplyr::tibble(
  string_representation =
    c("127, 126, 123, 1",
      "127, 128, 129, 4432",
      "127, 128, 129, 4432",
      "-127, 126, 123, 1",
      "-127, 128, 129, 4432",
      "-127, 128, 129, 4432",
      "-127, 128, 129, 67305985",
      "-127, 128, 129, 67305985",
      "-1.43e-6, -9871., -12.",
      "-1.43e-6, -9871., -12.",
      "-1.43e-6, -9871., -12.",
      "-1.43e-6, -9871., -12.",
      "-1.43e-6, -9871., -12.",
      "-1.43e-6, -9871., -12.",
      "-1.43e-9+1.2j, -9871.+1.2j, -12.+1.2j",
      "-1.43e-9+1.2j, -9871.+1.2j, -12.+1.2j",
      "-1.43e-9+1.2j, -9871.+1.2j, -12.+1.2j",
      "-1.43e-9+1.2j, -9871.+1.2j, -12.+1.2j",
      "0, 1",
      "'2024-08-01 23:13:38', '2024-08-15', '2024-09-01'",
      "'2024-08-01 23:13:38', '2024-08-15', '2024-09-01'",
      "'2024-08-01 23:13:38', '2024-08-15', '2024-09-01'",
      "'2024-08-01 23:13:38', '2024-08-15', '2024-09-01'",
      "'2024-08-01 23:13:38', '2024-08-15', '2024-09-01'",
      "'2024-08-01 23:13:38', '2024-08-15', '2024-09-01'",
      "0, 1, 20",
      "0, 1, 30",
      "\"foo\", \"bar\", \"foobar\"",
      "\"foo\", \"bar\", \"foobar\""),
  dtype = c("|u1", ">u2", "<u2", "|i1", ">i2", "<i2", ">i4", "<i4",
            ">f2", "<f2", ">f4", "<f4", ">f8", "<f8",
            ">c8", "<c8", ">c16", "<c16",
            "|b1", "<M8[ms]", ">M8[ms]", "<M8[Y]", ">M8[Y]", "<M8[M]", ">M8[M]", "<m8[D]",
            ">m8[D]", "|S9", "|U9")
)

dtypes <-
  py_run_string(
    paste(
      c(
        "import numpy as np",
        sprintf("val%02i = np.array([%s], dtype=\"%s\")",
                seq_len(nrow(data_types)),
                data_types$string_representation,
                data_types$dtype)),
      collapse = "\n"
    ),
    convert = FALSE
)

data_types <-
  dplyr::bind_cols(
    data_types,
    dplyr::tibble(
      raw_representation = lapply(seq_len(nrow(data_types)),
                                  \(i) dtypes[[sprintf("val%02i", i)]]$tobytes() |> as.raw()),
      r_representation   = lapply(sprintf("val%02i", seq_len(nrow(data_types))),
                                  \(i) dtypes[[i]] |> py_to_r())
    ) |>
      dplyr::mutate(
        r_representation   = lapply(r_representation, \(x) {
          if (inherits(x, "POSIXt")) {
            as.POSIXct(x, tz = "UTC")
          } else if (inherits(x, "numpy.ndarray")) {
            x$astype("double")
          } else x
        })
      )
  )

save(data_types,
     file = file.path(testthat::test_path(), "testdata", "testdata.rdata"),
     compress = TRUE)
