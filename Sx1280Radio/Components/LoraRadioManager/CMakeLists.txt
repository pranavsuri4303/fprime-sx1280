# File: Sx1280Radio/Components/LoraRadioManager/CMakeLists.txt

register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/LoraRadioManager.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/LoraRadioManager.cpp"
    DEPENDS
        Sx1280Radio_Device
)