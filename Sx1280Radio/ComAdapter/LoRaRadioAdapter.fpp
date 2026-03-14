module Sx1280Radio {

  # Component implementing the Svc.Com (communication adapter) interface
  # on top of a LoRaNode.
  #
  # This is a skeleton; behavior is implemented in LoRaRadioAdapter.cpp.

  passive component LoRaRadioAdapter {

    # ----------------------------------------------------------------------
    # F´ Com adapter ports (Svc.Com)
    # ----------------------------------------------------------------------

    # Downlink: framed data from Com stack to radio
    sync input port dataIn: Svc.ComDataWithContext

    # Uplink: framed data from radio to Com stack
    output port dataOut: Svc.ComDataWithContext

    # Downlink buffer return: give ownership back to upstream
    output port dataReturnOut: Svc.ComDataWithContext

    # Uplink buffer return: buffer that was sent on dataOut is now released
    sync input port dataReturnIn: Svc.ComDataWithContext

    # Com status: readiness for more downlink data
    output port comStatusOut: Fw.SuccessCondition

    # ----------------------------------------------------------------------
    # Buffer management (for uplink receive buffers)
    # ----------------------------------------------------------------------

    output port allocate: Fw.BufferGet
    output port deallocate: Fw.BufferSend

    # ----------------------------------------------------------------------
    # Scheduling (to poll the underlying LoRaNode)
    # ----------------------------------------------------------------------

    sync input port run: Svc.Sched
  }
}
