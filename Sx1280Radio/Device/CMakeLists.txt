# File: Sx1280Radio/Device/CMakeLists.txt

register_fprime_library(
    Sx1280Radio_Device
  SOURCES
    Sx1280Device.cpp
  HEADERS
    include/Sx1280Config.hpp
    include/Sx1280Device.hpp
  DEPENDS
    Sx1280Radio_Hal
    Sx1280Radio_SX128x
)