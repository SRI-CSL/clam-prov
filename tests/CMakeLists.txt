# find lit. Assume we are using external lit installed from pip
find_program(LIT_COMMAND lit REQUIRED)

set(LLVM_EXTERNAL_LIT ${LIT_COMMAND})

add_lit_testsuite(test-all "Run tests to check basic functionality"
  -v
  ${CMAKE_CURRENT_SOURCE_DIR}  ## where .cfg file is located
  PARAMS
  test_dir=${CMAKE_CURRENT_BINARY_DIR}
  ARGS
  --path=${CMAKE_INSTALL_PREFIX}/bin
  DEPENDS clam-prov install
)
