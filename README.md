# Entox-Detox - Yet Another Model 100 Tokenizer

Command-line tools `entox` and `detox` are used to convert between plain TEXT
and tokenized BASIC files for Tandy Model 100.

License: BSD

Copyright (C) 2024 tingtron

This product includes software developed by
- Copyright 2004-2008 by Ken Pettit, Jerome Vernet, James Hurd and John Hogerhuis

## Overview

Many Tandy Model 100 BASIC files found on the Internet are represented
in plain text, but with .BA extension. When copying such files to Tandy M100,
and opening in BASIC it may cause issues and posible memory corruption and cold boot, 
loosing storage contents.

Such plain text .BA files could be renamed to .DO and loaded into BASIC safely.
However, plain text BASIC files occupy more storage and take extra time loading in BASIC. 

So it makes sense to convert plain text BASIC into binary tokenized form.
We couldn't find any existing usable BASIC tokenizers usable from Windows command-line.
The routines used in VirtualT, which are tightly coupled with UI, 

## Features

The structure of tokenized BASIC files is a series of lines, without any common header or ending.
Each line has the following structure:

2 bytes Next Line Address (low-endian integer)
2 bytes Line Number (low-endian integer)
List of tokens (high ASCII) and plain text (low ASCII)
Zero Line Terminator

When loaded in memory, or for de-tokenization, after the end of the binary program,
2 zero bytes are placed to indicate the end of program.

Next Line Address in real BASIC files represents whatever position in memory the program had.
So all the lines will have a common offset of the beginning of the program in memory.
For our purposes this common memory offset is subtracted, and each line address indicates the
position from the beginning of the file.

`entox` converts plain text BASIC file to tokenized form
```
usage: entox [-d] [in.do [out.ba]]
             -d  debug
  in.do, out.ba  TEXT/BASIC files or stdin/stdout
```
`detox`  converts tokenized BASIC file to plain text form
```
usage: detox [-d] [in.ba [out.do]]
             -d  debug
  in.ba, out.do  BASIC/TEXT files or stdin/stdout
```
## References

Here are a few other methods of tokenizing BASIC for Tandy Model 100.

The next couple methods involve serial transfer, which gives the opportunity
of "tricking" Tandy to accept an original plain text .BA file and save it as a .DO file.

- Tandy TPDD-2 Backpack Drive Plus
  https://github.com/Jeff-Birt/Backpack
  https://www.soigeneris.com/tandy-tpdd-2-backpack-drive-2

  Serial interface with ability to transfer files using TS-DOS. TELCOM can be used
  to run CLI commands. Such download command can transfer plain text (including .BA) files.

- Tokenizer for Model 100 BASIC
  https://github.com/hackerb9/tokenize
  Interesting project based on Flex parsing.
  However, no binary release was offered for Windows and
  when compiled in MinGW, it produced wrong kind of BA files.
  An issue created for this project was not addressed:
  https://github.com/hackerb9/tokenize/issues/2

- The same author Hackerb9 documented the file format of tokenized BASIC
  http://fileformats.archiveteam.org/wiki/Tandy_200_BASIC_tokenized_file
  Note: The first two bytes of each line (PL,PH) are loosely defined
  as "never used" by Tandy BASIC.
  As a result, it generates random line offsets, which even
  occupy a different number of bytes in Windows (3 bytes because 0A is produced 
  as 0D 0A).

Emulators may provide a means to load plain text files.
But there wasn't any readily available method of saving tokenized files
from command line to be easily automated. 

 - VirtualT
   https://sourceforge.net/projects/virtualt/
   It has nice manual load/save and even drag-and-drop of files.
   It also has a socket client, which automates loading, but not saving.
