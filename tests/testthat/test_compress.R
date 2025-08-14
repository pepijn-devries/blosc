test_that("Data can be compressed and decompressed", {
  expect_true({
    volcano_compressed <- blosc_compress(volcano, typesize = 2, dtype = "<f2")
    volcano_reconstruct <- blosc_decompress(volcano_compressed, dtype = "<f2")
    object.size(volcano) > object.size(volcano_compressed) &&
      all(volcano_reconstruct == volcano)
  })
})