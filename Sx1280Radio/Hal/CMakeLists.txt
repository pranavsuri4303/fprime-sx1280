# File: Sx1280Radio/Hal/CMakeLists.txt

register_fprime_module(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/LinuxSx1280Radio.cpp"
    DEPENDS
        Sx1280Radio_SX128x
)

target_include_directories(
    ${FPRIME_CURRENT_MODULE}
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/include"
        "${CMAKE_CURRENT_LIST_DIR}/../ThirdParty/sx128x"
)