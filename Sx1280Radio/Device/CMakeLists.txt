register_fprime_module(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/Sx1280Device.cpp"
    DEPENDS
        Sx1280Radio_Hal
        Sx1280Radio_SX128x
)

target_include_directories(
    ${FPRIME_CURRENT_MODULE}
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/include"
        "${CMAKE_CURRENT_LIST_DIR}/../Hal/include"
        "${CMAKE_CURRENT_LIST_DIR}/../ThirdParty/sx128x"
)