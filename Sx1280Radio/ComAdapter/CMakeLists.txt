set(SOURCE_FILES
  "${CMAKE_CURRENT_LIST_DIR}/LoRaRadioAdapter.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/LoRaRadioAdapter.cpp"
)

set(MOD_DEPS
  Sx1280Radio_Hal
  Sx1280Radio_Config
  Sx1280Radio_Node
)

register_fprime_module()
