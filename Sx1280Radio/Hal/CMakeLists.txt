find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod)

register_fprime_module(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/LinuxSx1280Radio.cpp"
    DEPENDS
        Sx1280Radio_ThirdParty_sx128x
)

target_include_directories(
    ${FPRIME_CURRENT_MODULE}
    PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}"
        "${CMAKE_CURRENT_LIST_DIR}/../ThirdParty/sx128x"
)

target_link_libraries(
    ${FPRIME_CURRENT_MODULE}
    PUBLIC
        PkgConfig::GPIOD
)
