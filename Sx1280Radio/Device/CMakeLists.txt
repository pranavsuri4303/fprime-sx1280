register_fprime_module(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/Sx1280Device.cpp"
    DEPENDS
        lib_fprime-sx1280_Sx1280Radio_Hal
        lib_fprime-sx1280_Sx1280Radio_ThirdParty_sx128x
)

target_include_directories(
    ${FPRIME_CURRENT_MODULE}
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/include"
        "${CMAKE_CURRENT_LIST_DIR}/../Hal/include"
        "${CMAKE_CURRENT_LIST_DIR}/../ThirdParty/sx128x"
)
