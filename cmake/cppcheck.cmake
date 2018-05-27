find_program(
  CPPCKECK_BIN
  NAMES color-cppcheck cppcheck
)

add_custom_target(
  cppcheck
  "${CPPCKECK_BIN}" -q --enable=all --project=compile_commands.json
)
