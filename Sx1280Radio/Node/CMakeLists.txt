register_fprime_module(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/LoRaNode.cpp"
    DEPENDS
        Sx1280Radio_Hal
        Sx1280Radio_Config
)

target_include_directories(
    ${FPRIME_CURRENT_MODULE}
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
        "${CMAKE_CURRENT_LIST_DIR}/../Config"
        "${CMAKE_CURRENT_LIST_DIR}/../Hal"
)
