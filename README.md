# fft\_from\_mss

## Info

This code was written to test start/stop signals to a FFT block instantiated in logic.
The FFT block was added along with an AXI4 master and basic VHDL code to handle the signals
to the implementation included in icicle-kit-reference-design release 2022.02 on Libero SoC
v2021.3. In my modified implementation, net MSS\_GPIO\_2\_17\_OR\_COREGPIO\_C0\_GPIO\_OUT\_1\_Y
was drives the start signal instead of LED1 (hence using that GPIO value for start) and the fft\_done
signal is connected to input port GPIO\_2\_F2M\_30 (which leaves SW2 unconnected).
