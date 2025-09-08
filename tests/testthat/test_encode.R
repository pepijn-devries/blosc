load(file.path(testthat::test_path(), "testdata", "testdata.rdata"))

test_that("Encoding works as expected", {
  expect_true({
    result <- FALSE
    for (i in 1:nrow(data_types)) {
      result <- result || !(
        all(
          r_to_dtype(data_types$r_representation[[i]],
                     data_types$dtype[[i]], na = -1) ==
            data_types$raw_representation[[i]]
        )
      )
    }
    !result
  })
})

test_that("Decoding works as expected", {
  expect_true({
    result <- FALSE
    for (i in 1:nrow(data_types)) {
      result <- result || !(
        all(
          dtype_to_r(data_types$raw_representation[[i]],
                     data_types$dtype[[i]], na = -1) ==
            data_types$r_representation[[i]]
        )
      )
    }
    !result
  })
})

test_that("POSXlt is accepted", {
  expect_true({
    all(r_to_dtype(as.POSIXlt("2000-01-01", tz = "UTC"), "<M8[h]") == 
          as.raw(c(0x38, 0x3, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0)))
  })
})

test_that("difftime is accepted", {
  expect_true({
    all(r_to_dtype(as.difftime(1, units = "days"), dtype = "<m8[D]") == 
          as.raw(c(0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0)))
  })
})

test_that("difftime is converted when unit is not known by R", {
  expect_true({
    abs(1 - as.numeric(
      dtype_to_r(
        as.raw(c(0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0)), "<m8[ns]"),
      units = "secs") /
        1e+9) < 1e-6
  })
})
