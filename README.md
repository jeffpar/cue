# Cue

OS/2 Command-line Utility/Editor

Copyright (c) 1988-2023 Jeff Parsons

## Requirements

OS/2 Version 1.0 or greater.

## Installation

Copy CUE.EXE to a directory in your path, and CUESUBS.DLL to a directory in your LIBPATH.  If you're not sure what a LIBPATH is, just copy CUESUBS.DLL to the directory that holds all your other OS/2 .DLL files.

If you want Cue to be loaded automatically in every CMD screen group you create, add (at a minimum) the following line:

    CUE

to the .CMD file specified on the PROTSHELL= line of your CONFIG.SYS, which usually looks something like:

    PROTSHELL=C:\OS2\SHELL.EXE C:\OS2\CMD.EXE /K C:\AUTOEXEC.CMD

Specifying CUE only in your STARTUP.CMD file will only have an effect in your first screen group, and is probably not what you want to do.

Cue is a lot like the memory-resident tools you are probably familiar with under DOS, but currently works only in OS/2's protected-mode.  Like most intelligent memory-resident programs, it does not consume any more memory when run more than once in a single screen group, and all of its code (and most of its data) is shared, meaning that you will incur little additional overhead when you run it in multiple screen groups.

## Summary of Existing Major Features

  1. Command-line recall (history)
  2. Enhanced command-line editing features
  3. Context-sensitive help, including on-line summary of usage and options
  4. User-defined internal commands (aliases)
  5. Simulated keyboard input
  6. Scroll memory
  7. Process tracking
  8. Miscellaneous features

## Summary of Planned Major Features

  1. Keyboard macros
  2. User configuration file
  3. Built-in "intelligent" aliases (ie, TO)
  4. Extended screen control (change colors, fonts, modes, timeout, etc)
  5. Extended process control (list, terminate, etc)

## Descriptions of Existing Major Features

  1. Command-line recall (history)

Cue records all your input lines in what is referred to here as a history buffer, and allows you to recall past lines with the UP arrow key, or more recent lines with the DOWN arrow key.  You can view a series of lines at once with the PGUP key, which displays a "history window" and can be sequentially scanned with the UP and DOWN arrow keys, or in window-sized increments by pressing PGUP and PGDN keys (TBA).  To select a line for editing from the history window, highlight it and press a right or left arrow; to select one for immediate entry, press ENTER instead.  To exit the history window, press ESCAPE.  ALT-F1 can be pressed at any time for help.

Note that the traditional DOS/OS2 command-line template is still supported, as are all the keys that manipulate it (F1, F3, etc), and that the template is NOT part of the history buffer.  This means that F3 and the UP arrow key may not always display the same thing:  the UP arrow displays the last line you typed, whereas F3 displays the current template, which is usually the last line received by an application, BUT may be different from what you typed, especially if it contained an alias that was expanded.  Cue allows this to occur, because some applications may wish to preset the template in certain ways that are more convenient.

Cue provides you with one history buffer per screen group, which can optionally be divided into as many as 4 partitions of equal size.  As different processes request input, each is assigned an unused partition;  if there are none, then the least recently used partition is cleared and re-assigned.  To the extent that Cue can detect them, all other instances of a previously encountered process (in that screen group) will use the partition assigned to the first instance (ie, a second copy of CMD).  The effect is much the same as that achieved by other popular command-line editors (for DOS).

The default history buffer size is 4kb, and the default number of partitions is 2, providing you with 2 2kb partitions (just as if you had specified /B 4096,2 when you loaded Cue).  Assuming CMD was the first process to request input, its commands would go into the first partition, and a subsequent process' commands (ie, FTP) into the second.  If you shelled from FTP to a second copy of CMD, the first partition would be selected again;  if instead you shelled to, say, MASM, the absence of a third partition would force Cue to clear the first partition and assign it to MASM.  When you later returned to CMD, the first partition would again be cleared and assigned back to CMD.

Cue doesn't create individual history buffers for every process, because history would be lost every time you exited and later restarted the same program (for example, repeatedly shelling to CMD from another program).	Cue's current approach also makes it easier for multiple instances of the same process in the same screen group to refer to the same history buffer, and reduces the amount of memory spent on non-interactive processes.  Private, per-process history buffers will be optional in a future version of Cue.

  2. Enhanced command-line editing features

  3. Context-sensitive help and on-line summary of usage and options

Run CUE /H for usage and a list of options.  Anytime after loading Cue, you can press Alt-F1 for immediate help (except...).

Cue options are case-insensitive, and can be specified with either a leading dash (-) or slash (/).  Options that do not require a subsequent parameter can be combined behind a single option specifier.  Options that DO require a subsequent parameter (like /A, /R, or /V) do not require space separating them.  In other words:

    CUE /Q43AALIAS.OS2

is equivalent to

    CUE /Q /43 /A ALIAS.OS2

  4. User-defined internal commands (aliases)

Aliases provide a means of turning tedious and/or lengthy command-lines into simple, short commands.  While there is little that an alias can accomplish that a batch file cannot, aliases execute faster and alleviate the need for lots of little batch files.

Aliases are defined through the use of an alias file, which is a simple text file containing one line per alias.  The first sequence of non-whitespace characters on a line defines the alias, and the rest of the line (following the alias and its trailing whitespace) becomes the value of the alias.  You can create multiple alias files, but only the aliases in the file you last loaded with the /A option will be available.  An option to append a set of aliases to your existing aliases will be provided in a future version.

Aliases are only substituted with their associated values when they appear as the first keyword on a command-line.  Also, they are case-insensitive; ie, if you have loaded aliases "abort" and "Abort" from a single alias file, only the first of those aliases will ever be used.

  5. Simulated keyboard input

  6. Scroll memory

  7. Process tracking

  8. Miscellaneous features

As soon as you run Cue for the first time within a given screen group, a portion of Cue will remain active in the screen group until you destroy it (ie, type EXIT from the base CMD shell and return to the Program Selector).  Running Cue repeatedly in a particular screen group will not consume any additional resources, and is in fact the only way to load a new set of aliases, list your aliases, etc.  In other words, Cue behaves as a memory-resident utility the FIRST time it is successfully run, and as an interface to the previously loaded copy on SUBSEQUENT runs.

If you only want to use Cue temporarily within a particular screen group, with a particular application, use Cue to run the application for you; ie,

    CUE FTP.EXE

If no extension is supplied, .EXE is assumed.  Arguments may also be added after the program name.  As soon as the program terminates, Cue itself will terminate, and will no longer be active in that screen group.  If Cue is already active when you attempt this, it will immediately terminate (after processing any other specified Cue options), since the intermediate copy of Cue would serve no purpose.

Normally, you will want to load Cue in each of your screen groups and leave it there, in which case there's no point specifying a program name.

A feature of the CMD shell is that it cannot be permanently loaded in a screen group (an option the DOS shell provides with /P).  To simulate that effect for CMD, specify /P when you load Cue, and the EXIT command will henceforth be disabled for the parent CMD process.  This only has effect from the keyboard however.  It is still possible to exit CMD from a batch file that uses EXIT, or via an alias that translates to EXIT.

On the so-called enhanced keyboards (those with 12 function keys instead of 10), both sets of cursor positioning keys are supported.  If you have an older keyboard (one with only 10 function keys), you may want to use the /+ option, which converts the PLUS key on your keypad into an ENTER key.  The /+ option also determines whether or not Cue will turn off Num-Lock for you.  If you use /+, Cue will NOT change your Num-Lock state;  if you don't use /+, it WILL change Num-Lock.

## Descriptions of Planned Major Features

  1. Keyboard macros

To be available globally, or per specific (ie, named) processes.

  2. User configuration file

  3. Built-in "intelligent" aliases (ie, TO)

  4. Extended screen control (change colors, fonts, modes, etc)

  5. Extended process control (list, terminate, etc)

## Planned Enhancements

  1. Complete emulation of all DOS/OS2 editing functions.

  2. Allow keyboard macros to replace editing keys.

  3. Add more editing functions (ie, Delete word right, Delete line left, etc).

  4. Add more history window functions (ie, Page backward, Page forward, Delete, etc).

  5. Add more scroll memory functions (ie, Page backward, Page forward, Cursor movement, Pick line, Execute line, etc).

  6. Provide alias argument substitution, and "alias characters".

  7. Allow history from one screen group to be linked to history in another screen group.

  8. Ability to "lock" a command in the history buffer (!)

## Other Planned Changes

  1. Reset screen immediately after exit from scroll-back mode.

  2. Change Ctrl-Home, perhaps to Ctrl-Del, and ask for confirmation.  I don't particularly like Ctrl-PgUp, even though it WAS	another app's choice.

  3. Ability to escape and page back inside help windows.

  4. New function (similar to F2) to select next matching line from	history buffer that begins with the next character typed.

  5. Option to filter out lines whose lengths are less than or equal to a user-defined limit.

## Cue examples

  1. This lists Cue usage and options.

    CUE /H

  2. This activates Cue for the life of the screen group.

    CUE

  3. This activates Cue for the life of CMD (ie, until you type EXIT).

    CUE CMD

  4. This loads Cue with a variety of options, like an alias file, new keyboard repeat delay and rate, etc.

    CUE /A ALIAS.OS2 /R250,30 /43

  5. This lists the aliases currently loaded, and increases your keyboard repeat rate to the maximum allowed.

    CUE /L /R0

  6. This reloads your current aliases.

    CUE /A ALIAS.OS2

  7. This is an example of feeding other programs some initial input (usually done from a batch file).  Any type-ahead will appear after all Cue-generated input has been exhausted.

    CUE /Q /S password\n
    WZMAIL

  8. This feeds a capital A and an up-arrow into a subsequent program, after a 2-second pause (the pause units are 1/4 sec).

    CUE /QS "\p8\d65\D72"

  9. This does the same thing as before, but specifies the keystrokes in hex.

    CUE /QS "\p8\x41\X48"

## Alias examples

If you put the following three lines in a file named ALIAS.OS2:

    ABORT EXIT
    25	  CLS && CUE /Q25
    43	  CLS && CUE /Q43

and ran CUE /A ALIAS.OS2, you would have 3 new internal commands at your disposal.  Note that the alias ABORT is useful when you've loaded Cue with /P and you want to be able to exit Cue's parent CMD (/P disables the EXIT command from the keyboard).

The aliases 25 and 43 demonstrate how aliases can be used to issue multiple commands, and turn individual features of Cue (or any other utility) into separate commands.

## Notes

Send suggestions, comments, and bugs to jeffpar.

## License

Cue is released under the terms of an [MIT License](LICENSE.txt).