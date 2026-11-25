# ZedBoard Zynq-7000 SYCL and Custom PL Accelerator

This project sets up a Zynq-7000 SoC design on the Digilent ZedBoard (xc7z020clg484-1) and offloads vector addition from SYCL to a custom AXI4-Lite hardware accelerator in Programmable Logic (PL).

## Folders
- 01_zynq_ps_bringup: Vivado block design, exported .hdf, and SDK Hello World UART test (115200 baud).
- 02_oneapi_sycl_arm: Basic SYCL 2020 header and vector addition compiled for ARMv7 (arm-linux-gnueabihf).
- 03_custom_pl_accelerator: Custom Verilog AXI4-Lite slave module (pl_vec_add_axi.v) and testbench.
- 04_sycl_pl_offload: C++ SYCL host code with /dev/mem MMIO wrapper mapped to PL base address 0x43C00000.

## AXI4-Lite Registers (Base: 0x43C00000)
- 0x00 (CTRL): Write 1 to start, 0 to reset
- 0x04 (STATUS): Bit 0 = 1 when done
- 0x08 (OPERAND_A): Input A
- 0x0C (OPERAND_B): Input B
- 0x10 (RESULT): Output (A + B)
