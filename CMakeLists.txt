set(UI_DIR ./ui)
file(GLOB_RECURSE UI_SOURCES ${UI_DIR}/*.c)
file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} "ui/*.c")

idf_component_register(
    SRCS "main.c" "lv_port.c" ${UI_SOURCES}
    INCLUDE_DIRS "." ${UI_DIR})
