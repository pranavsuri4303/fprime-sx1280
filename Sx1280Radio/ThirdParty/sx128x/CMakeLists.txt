register_fprime_module(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/SX128x.cpp"
)

target_include_directories(
    ${FPRIME_CURRENT_MODULE}
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
)