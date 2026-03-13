# File: Sx1280Radio/Hal/CMakeLists.txt

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod)

register_fprime_library(
    Sx1280Radio_Hal
  SOURCES
    LinuxSx1280Radio.cpp
  HEADERS
    include/LinuxSx1280Radio.hpp
  DEPENDS
    Sx1280Radio_SX128x
)

target_link_libraries(Sx1280Radio_Hal PUBLIC PkgConfig::GPIOD)