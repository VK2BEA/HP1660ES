# IAdown

Program to transfer a compiled inverse assembler relocatable file to an HP logic analyzer via its telnet connection.

The HP `ASM` program *(from the HP 10391B Inverse Assembler Development Package)* will produce a '.R' *(relocatable)* file but this cannot be used directly on the logic analyzer.

The final linked program file is created using the 'IALDOWN' program *(from the HP 10391B Inverse Assembler Development Package)* in transferring the .R file vi RS-232.
It will also be created when the .R file is transferred to the logic analyzer using GPIB or telnet with the `:MMEMory:DOWNload` command.

The "HP 10391B Inverse Assembler Development Package", which includes the `ASM` program is available from Keysight at:

https://www.keysight.com/us/en/lib/software-detail/instrument-firmware-software/10391b-inverse-assembler-development-package-version-0200-sw575.html

The `ASM` program (inverse assembler compiler) will run under `dosbox` on Linux.

`$ sudo dnf install dosbox`

This program (`IAdown`) transfers the file created by `ASM` *(.R relocatable)* to the HP logic analyzer using telnet.

See P542 of HP 1660E/ES/EP and 1670E Series Logic Analyzer User's Guide (Publication 01660-97028) on connecting to the telnet port of the logic analyzer.

Compile with: `$ gcc -o IAdown IAdown.c`

Example execution:  `$ IAdown -a 192.168.1.16 -n I6809 -d \"MC6809 Inverse Assembler\" I6890.R`
