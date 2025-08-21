test_that("dtypes cannot out of range", {
  expect_error({
    blosc_compress(volcano, typesize = 256L)
  })
})

test_that("dtypes should match typesize", {
  expect_error({
    blosc_compress(volcano, typesize = 2L, dtype = "<f4")
  })
})

test_that("compression level is in range", {
  expect_error({
    blosc_compress(volcano, level = 10L, dtype = "<f4")
  })
})

test_that("byte order should be known", {
  expect_error({
    blosc:::dtype_to_list_("*f8")
  })
})

test_that("unknown types are not accepted", {
  expect_error({
    blosc:::dtype_to_list_("<q8")
  })
})

test_that("logical dtype should always be 1 byte", {
  expect_error({
    blosc:::dtype_to_list_("|b2")
  })
})

test_that("numerical dtype cannot be 3 bytes", {
  expect_error({
    blosc:::dtype_to_list_("<i3")
  })
})

test_that("complex dtype cannot be 4 bytes", {
  expect_error({
    blosc:::dtype_to_list_("<c4")
  })
})

test_that("size of raw data should be multitude of dtype", {
  expect_error({
    dtype_to_r(raw(5), "<f2")
  })
})

test_that("difftime should always have type 'm'", {
  expect_error({
    r_to_dtype(as.difftime(1, units = "hours"), dtype = "<f2")
  })
})

test_that("float cannot have length 1", {
  expect_error({
     r_to_dtype(1, dtype = "<f1")
  })
})

test_that("Date time cannot have length other than 8", {
  expect_error({
    r_to_dtype(1, dtype = "<M1[s]")
  })
})

test_that("Only date time use units", {
  expect_warning({
    blosc:::dtype_to_list_("<f2[s]")
  })
})
