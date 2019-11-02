find_program(
  CLANG_TIDY_BIN
  NAMES run-clang-tidy run-clang-tidy.py
)

add_custom_target(
  clang-tidy
  "${CLANG_TIDY_BIN}" -quiet
)
