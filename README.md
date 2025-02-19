# IAdown

A Linux program to transfer a compiled inverse assembler relocatable file to an HP logic analyzer via its telnet connection.

The HP `ASM` program *(from the HP 10391B Inverse Assembler Development Package)* will produce a '.R' *(relocatable)* file but this cannot be used directly on the logic analyzer.

The final linked program file is created from the relocatable file using the DOS program 'IALDOWN' *(from the HP 10391B Inverse Assembler Development Package)* in transferring the .R file vi RS-232 from the computer to the logic analyzer.
Alternatively, it is created when the .R file is transferred to the logic analyzer using GPIB or telnet with the `:MMEMory:DOWNload` command.

The "HP 10391B Inverse Assembler Development Package", which includes the `ASM` program is available from Keysight at:

https://www.keysight.com/us/en/lib/software-detail/instrument-firmware-software/10391b-inverse-assembler-development-package-version-0200-sw575.html

The `ASM` program (inverse assembler compiler) will run under `dosbox` on Linux.

`$ sudo dnf install dosbox`

DOSBox can be configured to mount a directory and execute other commands when started. 

Edit the file `~/.config/dosbox/dosbox-staging.conf`. Add the startup commands to the `[autoexec]` section; e.g:

```
[autoexec]
# Lines in this section will be run at startup.
mount c "~/10391B Inverse Assembler Development Package_v2.0/IA_Development_Disk/"
c:
```

*Note that the file in the `TABLES` directory (created when unzipping the HP 10391B package) must be renamed from `AIAL.TXT` to `AIAL`*

Start `DOSBox`:

`$ dosbox`

In the `DOSBox` window:
```
Z:\> mount c "~/10391B Inverse Assembler Development Package_v2.0/IA_Development_Disk/"
Local directory /home/michael/10391B Inverse Assembler Development Package_v2.0/IA_Development_Disk/ mounted as C drive"
Z:\>c:
C:\> 
```

example compile of 68010 inverse assembly source: 
```
C:\>copy EXAMPLES\i68010.s .
 I68010.S
   1 File(s) copied.
C:\>asm -o i68010.s > i68010.lst
C:\>ls
EXAMPLES      TABLES       asm.exe      i68010.a     168010.lst   i68010.r
i68010.s      ialdown.exe
C:\>
```

This program (`IAdown`) transfers the file created by `ASM` *(.R relocatable)* to the HP logic analyzer using telnet.

See P542 of HP 1660E/ES/EP and 1670E Series Logic Analyzer User's Guide (Publication 01660-97028) on connecting to the telnet port of the logic analyzer.

Compile with: `$ gcc -o IAdown IAdown.c`

Example execution:  `$ IAdown -a 192.168.1.16 -n I68010 -d \"MC68010 Inverse Assembler\" i68010.r`

```
Usage: ./IAdown -a IP_ADDRESS -n IA_FILE [-f] [-d "Description"] IA_FILE.R
       -a | --address       IP address of HP logic analyzer
       -n | --name          File name on logic analyzer
       -d | --description   Descriptive string for the inverse assembler
       -f | --floppy        Create the file on the floppy drive
            --verbose       Show debugging information
```
