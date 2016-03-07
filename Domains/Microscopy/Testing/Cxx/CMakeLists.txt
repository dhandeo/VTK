
# OpenSlideReader Tests

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestOpenSlideReader.cxx
  "DATA{${VTK_TEST_INPUT_DIR}/Microscopy/small2.ndpi}"
  )

vtk_add_test_cxx(${vtk-module}CxxTests  tests
  TestOpenSlideReaderPartial.cxx
  "DATA{${VTK_TEST_INPUT_DIR}/Microscopy/small2.ndpi}"
  )

# PTIFWriter tests
vtk_add_test_cxx(${vtk-module}CxxTests tests
    TestPTIFWriter.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  )

  vtk_add_test_cxx(${vtk-module}CxxTests tests
      TestPTIFWriterExtra.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  )


vtk_test_cxx_executable(${vtk-module}CxxTests tests RENDERING_FACTORY)
