module Sx1280Radio {

    @ LoRa radio manager implementing the F´ Communication Adapter Interface
    @ over a dual-radio SX1280/SX1281 transport.
    active component LoraRadioManager {

        @ Standard F´ communication adapter interface
        import Svc.Com

        @ Periodic scheduler hook used to poll radios for TX/RX completion
        sync input port schedIn: Svc.Sched

        @ Standard command interface
        command recv port cmdIn
        command reg port cmdRegOut
        command resp port cmdRespOut

        @ Standard event / telemetry / time interfaces
        event port eventOut
        text event port textEventOut
        telemetry port tlmOut
        time get port timeGetOut

        @ Start the radio manager and begin RX operation
        async command START

        @ Stop the radio manager and place radios in standby
        async command STOP

        @ Radio manager started successfully
        event LinkStarted severity activity high format "LoRa radio manager started"

        @ Radio manager stopped successfully
        event LinkStopped severity activity high format "LoRa radio manager stopped"

        @ Radio manager failed to start
        event LinkStartFailed severity warning high format "LoRa radio manager failed to start"

        @ Outgoing transmission failed
        event TxFailed severity warning low format "Transmit failed"

        @ Incoming reception failed
        event RxFailed severity warning low format "Receive failed"

        @ Received payload exceeded available buffer capacity
        event RxPayloadTooLarge(payloadSize: U32) severity warning low format "Received payload too large: size {} bytes"

        @ Number of successfully transmitted frames
        telemetry TxFrames: U32

        @ Number of successfully received frames
        telemetry RxFrames: U32

        @ Number of transmit failures
        telemetry TxFailures: U32

        @ Number of receive failures
        telemetry RxFailures: U32

        @ Size of last transmitted frame in bytes
        telemetry LastTxBytes: U32

        @ Size of last received frame in bytes
        telemetry LastRxBytes: U32

        @ Whether the adapter is currently running (0 = stopped, 1 = running)
        telemetry LinkRunning: U32
    }

}