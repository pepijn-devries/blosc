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