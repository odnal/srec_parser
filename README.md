# SREC Parser

The idea is to simulate a bootloader process that loads a
Motorola S-record file format on a platform such as a micro-
controller, Raspberry Pi or whatever in a baremetal environment.

This can be done over UART and requires very little overhead.
Hence, the UART macro names for transparency.

SREC is a file format that is converted from a binary image into
ascii text. More [here](https://en.wikipedia.org/wiki/SREC_(file_format)).

This is purely an exercise.
