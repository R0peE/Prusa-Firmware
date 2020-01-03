This is a fork of the PrusaResearch i3 firmware
===

The goal of this fork is to make most of the firmware operational on MKS GEN L V2.0 board with TMC2130 StepStick drivers.

The EINSY Rambo board has more capabilities, so some non-critical functionality has been disabled.

See DisabledFeatures.txt for details what is not currently supported on the MKS GEN L board.

For compile the firmware you have to install the Prusa Rambo board variant as described in step 1b in the Windows section in the original [README.md](https://github.com/prusa3d/Prusa-Firmware#using-linux-subsystem-under-windows-10-64-bit)

I personally use Arduino 1.8.10 with not issues. I compile the firmware from the Arduino IDE, the command line build.sh was not working for me.

Status
---
There are two branches:
 - klone-MK3 is following the MK3 branch with MKS GEN L mods
 - klone_gen_l is following the latest release 3.8.1 from the PrusaResearch tree

I was able to complete all calibration procedures and print Benchy and XYZ calibration cube. Firmware appears to be operational.

Note
---
The original README.md has been renamed [README.prusa](README.prusa)
