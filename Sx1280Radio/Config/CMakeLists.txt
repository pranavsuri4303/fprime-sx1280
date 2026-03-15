register_fprime_module(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/NodeConfigYaml.cpp"
    DEPENDS
        Sx1280Radio_Hal
)

target_include_directories(
    ${FPRIME_CURRENT_MODULE}
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/../Hal"
)
