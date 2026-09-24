cmake_minimum_required(VERSION 3.20.0)
set(KCONFIG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../Kconfig")
set(CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../prj.conf;${CMAKE_CURRENT_SOURCE_DIR}/prj.conf")
set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../boards/native_sim_native_64.overlay")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(panel_tests)
target_include_directories(app PRIVATE ../../include ../../ui "${ZEPHYR_BASE}/../modules/lib/cjson")
target_sources(app PRIVATE src/test_app.c ../../src/sensor_json.c ../../src/response_buffer.c
  ../../src/sensor_model.c ../../ui/ui.c "${ZEPHYR_BASE}/../modules/lib/cjson/cJSON.c")

target_compile_definitions(app PRIVATE CJSON_NESTING_LIMIT=16)
