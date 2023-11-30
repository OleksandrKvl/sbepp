# A helper function to integrate sbepp-files generation into cmake-project.
#
# sbepp_generate(
#     # Override schema name (optional).
#     # Value must be a valid C identiier.
#     [SCHEMA_NAME <string>]
#
#     # XML-file containing sbe-schema definition.
#     SCHEMA_FILE <path>
#
#     # Where the generated file will be created (usually relative to
#     # PROJECT_BINARY_DIR or to CMAKE_CURRENT_BINARY_DIR).
#     OUT_DIR <path>
#
#     # Target name for custom target that is created to rune generation
#     # over specified schema definition.
#     # The value of TARGET_NAME can be the same for multiple calls
#     # of `sbepp_generate()` function, thus a single target
#     # can be used to generate sbepp files for multiple schemas.
#     TARGET_NAME <target-name>
# )
function(sbepp_generate)
  include(CMakeParseArguments)

  set(_singleargs SCHEMA_NAME SCHEMA_FILE OUT_DIR TARGET_NAME)

  cmake_parse_arguments(sbepp_generate "" "${_singleargs}" "" "${ARGN}")

  # Check mandatory agrs:
  #   * SCHEMA_FILE;
  #   * OUT_DIR;
  #   * TARGET_NAME.
  if ("${sbepp_generate_SCHEMA_FILE}" STREQUAL "")
    message(FATAL_ERROR "[sbepp_generate] error: missing SCHEMA_FILE parameter")
  endif ()
  if ("${sbepp_generate_OUT_DIR}" STREQUAL "")
    message(FATAL_ERROR "[sbepp_generate] error: missing OUT_DIR parameter")
  endif ()
  if ("${sbepp_generate_TARGET_NAME}" STREQUAL "")
    message(FATAL_ERROR "[sbepp_generate] error: missing TARGET_NAME parameter")
  endif ()

  # Check that SCHEMA_NAME is valid C identifier.
  if (NOT "${sbepp_generate_SCHEMA_NAME}" STREQUAL "")
    string(MAKE_C_IDENTIFIER "${sbepp_generate_SCHEMA_NAME}" _schema_c_identifier)

    if (NOT "${sbepp_generate_SCHEMA_NAME}" STREQUAL "${_schema_c_identifier}")
      message(FATAL_ERROR "[sbepp_generate] error: SCHEMA_NAME parameter must be valid C identiier")
    endif ()
  endif ()

  message(STATUS "[sbepp_generate] parameters:"
            "\n                    SCHEMA_NAME=${sbepp_generate_SCHEMA_NAME}"
            "\n                    SCHEMA_FILE=${sbepp_generate_SCHEMA_FILE}"
            "\n                    OUT_DIR=${sbepp_generate_OUT_DIR}"
            "\n                    TARGET_NAME=${sbepp_generate_TARGET_NAME}")

  # Create generator anchor file.
  # This file would be used for a empty file that will
  # allow to skip generation on every build having no changes
  # in SCHEMA_FILE or sbeppc.
  get_filename_component(schema_file_name "${sbepp_generate_SCHEMA_FILE}" NAME)
  cmake_path(APPEND generator_anchor_file ${sbepp_generate_OUT_DIR} ${schema_file_name}.gen)
  message(STATUS "[sbepp_generate] generator_anchor_file: ${generator_anchor_file}")

  add_custom_command(
      VERBATIM

      # Main generate command.
      COMMAND
          $<TARGET_FILE:sbepp::sbeppc>
          $<IF:$<STREQUAL:"${sbepp_generate_SCHEMA_NAME}","">,,--schema-name>
          "${sbepp_generate_SCHEMA_NAME}"
          "--output-dir"
          "${sbepp_generate_OUT_DIR}"
          "${sbepp_generate_SCHEMA_FILE}"

      # Create a aux file the name of which is fixed.
      # Because the names of generated files might be defined within XML
      # we cannot get a reliable names bosed on parameters of this function.
      # This file would be used as output of this custom command.
      # It would be used as DEPENDS parameter for custom target.
      COMMAND
          ${CMAKE_COMMAND} -E touch ${generator_anchor_file}
      DEPENDS
          sbepp::sbeppc
          "${sbepp_generate_SCHEMA_FILE}"

      OUTPUT ${generator_anchor_file}
  )

  if (NOT TARGET ${sbepp_generate_TARGET_NAME})
    message(STATUS "[sbepp_generate] create custom target '${sbepp_generate_TARGET_NAME}'")
    add_custom_target(
      ${sbepp_generate_TARGET_NAME}
      # We don't know all series of generated sbepp-files in advance.
      # Because if sbepp_generate function as called multiple times
      # we cannot set DEPENDS parameter using hardcoded list.
      # So a generator expression is used here to take a final array
      # from target's SBEPPC_ANCHORS propery.
      # SBEPPC_ANCHORS is used to accumulate all the generate-command outputs.
      DEPENDS $<TARGET_PROPERTY:${sbepp_generate_TARGET_NAME},SBEPPC_ANCHORS>)
  endif ()

  message(STATUS "[sbepp_generate] append SBEPPC_ANCHORS property of '${sbepp_generate_TARGET_NAME}' target: ${generator_anchor_file}")
  # Add yet another DEPENDS parameter to our custom target that forces
  # generation commands to run.
  set_property(TARGET ${sbepp_generate_TARGET_NAME}
    APPEND PROPERTY SBEPPC_ANCHORS ${generator_anchor_file}
  )
endfunction()
