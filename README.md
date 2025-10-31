# C90-BigBlueDisk-Paktool
Set of tools that can help you working with PAK format that using in Big Blue Disk disk magazine

## Disclaimer

Softdisk release IBM PC disk magazine in 1986 and its content writing in very interesting way as mix of QBASIC and Turbo Pascal compiled sources. Disk from start support graphics in form of full screen CGA image.
First disk only have has 3 images that provide in compressed pak formt with embedded CGA palette byte.

This repository targetted to create the cross-platform tools that help pack, unpack and show content that stored in pak format. Source can be compiled on any C90 compatible compiler. For MS-DOS it can be build with Borland Turbo C.

## PAK format structure

Header contained 3 bytes - color and two marker for custom realization of RLE packer. There is no any kind of data then help to identify file format instead of extension.

| Offset (bytes) | Size (bytes) | Description |
| -------- | ------- | ------- |
| 0 | 1 | Color and palette that direct set up CGA color palette and background color, this value actually write to 0x3D9 port of IBM PC |
| 1 | 1 | marker A |
| 2 | 1 | marker B |

All content after header is interpret by following rules Then all data is decoded by following rules:

- byte equal marker A

| Offset (bytes) | Size (bytes) | Description |
| -------- | ------- | ------- |
| 0 | 1 | marker A |
| 1 | 1 | repeating count - 4 |
| 2 | 1 | byte that should repeated |

- byte equal marker B

| Offset (bytes) | Size (bytes) | Description |
| -------- | ------- | ------- |
| 0 | 1 | marker B |
| 1 | 2 | repeating count |
| 3 | 1 | byte that should repeated |

- if byte not equal marker A or marker B then it just outputter 1 times

## CMD Tools Description

### PAKTOOL.EXE

Allow user to show pak file header information and unpack the content into file

`paktool`

