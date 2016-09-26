# Description

This project attempts to take two separate projects, the ZipCPU and Xess.com's XuLA2-LX25, and merge them together into a single system on a chip implementation.  As currently implemented, this SoC offers the following peripherals to the ZipCPU within:

- External peripherals
  -- 14 GPIO inputs, 15 GPIO outputs
  -- PWM output (can be swapped for an FM transmitter ...)
  -- Rx and Tx UART ports
  -- 1MB SPI flash, together with a read/write controller
  -- 32MB SDRAM capable of non-stop pipeline reads and writes
  -- SD Card, sharing the SPI wires of the flash
- Internal peripherals
  -- Real-time clock and date4
  -- Access to the FPGA configuration port, for unattended updates
  -- ZipCPU debug/configuration port access from JTAG
- ZipCPU peripherals
  -- 3x timers, each of which can be programmed either in a one shot mode or as a repeating interval timer
  -- A watchdog timer, and a wishbone bus watchdog timer
  -- Two interrupt controllers
  -- Direct Memory Access (DMA) controller for unattended memory movement

# Current Status

The SoC is fully functional.  Keeping the project from being complete, however,
is the lack of an integrated specification document.  (Specification documents
do exist, however, for many of the peripheral components.)

# Unique Features

This System on a Chip (SoC) controller has some unique features associated with
it, above and beyond the peripherals listed above.  Primary among those is the 
JTAG to 32-bit wishbone master conversion.  This makes it possible for an
external entity to read  from or write to the wishbone bus.  Uses include
verifying whether or not peripherals work, as well as configuring the CPU, 
memory and flash for whatever purpose one might have.  This particular 
capability was designed so that host (i.e. FPGA control) programs (external 
to the FPGA) can call a common set of bus interface functions to communicate
with the FPGA, regardless of how the bus was implemented.

A second unique feature is a PWM driver that spreads its digital energy into
higher (non-auditory) frequencies which can then be filtered out easier
with a simple low-pass filter.  As an example, sending a zero, or half-pulse
width, will result in alternating digital ones and zeros from the driver.  While
I expect this will have a pleasing effect on the ear, especially since these
transactions will be outside of the normal hearing range, this is the first time
I have tried it and the jury's still out regarding whether or not it works or 
even works well.

Finally, while it may not really be that unique, this core does feature a fully
functional SDRAM controller capable of one read cycle (or write cycle) every
two clocks when pipelined.  Unlike many other dynamic memory controllers, this
one was _not_ created from a proprietary, closed source, memory interface
generation facility--so it is available for anyone to examine, study, and
even comment upon and improve--subject to the conditions of the GPL.
