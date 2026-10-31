#
# SPDX-License-Identifier: Apache-2.0
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ble_relays)

target_sources(app PRIVATE
  src/main.c
  src/parameters.c
  src/app_params.c
  src/param_service.c
  src/io.c
  src/logic.c
)

# Parameter description JSON: validated, minified and embedded into the firmware.
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated)
set(params_desc_src ${CMAKE_CURRENT_SOURCE_DIR}/src/params_desc.json)
set(params_desc_min ${gen_dir}/params_desc.min.json)
set(params_desc_hdr ${gen_dir}/params_desc_gen.h)

add_custom_command(
  OUTPUT ${params_desc_min} ${params_desc_hdr}
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_params_desc.py
          --input ${params_desc_src}
          --output-json ${params_desc_min}
          --output-header ${params_desc_hdr}
  DEPENDS ${params_desc_src} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_params_desc.py
  COMMENT "Generating parameter description"
)
add_custom_target(params_desc_gen DEPENDS ${params_desc_min} ${params_desc_hdr})
add_dependencies(app params_desc_gen)

generate_inc_file_for_target(app ${params_desc_min} ${gen_dir}/params_desc.json.inc)
