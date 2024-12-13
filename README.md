# BextTool

It's like *ExifTool* but for the 'bext', broadcast extension chunk of a WAVE file. It reads the entire contents of the format and broadcast extension chunks (as I understand them) and primarily allows writing to the 'bext' originator fields. It updates the existing 'bext' chunk or adds a new chunk at the end of the file.

The next feature will to have a new 'bext' chunk replace any available 'JUNK' chunk space in the start of a file.

It has not been tested on 'RF64' type files.

## Cross Platform

The Wave.h header file uses the Boost endianess file so it is anticipated that this program can be compiled on platforms with different endianess and maintain the proper WAVE byte order.

The broadcast WAVE file specification is [here](https://tech.ebu.ch/docs/tech/tech3285.pdf) with more detailed explanations of the relavant fields.

## Original Build

This was originall built and working on a 2023 Mac Studio with an Apple M2 Max chip and running MacOS 15.1 (Sequoia). The Boost library was installed with [Mac Ports](https://www.macports.org).
