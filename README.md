# isscrolls - Simple player toolkit for the Ironsworn tabletop RPG

 isscrolls is a simple toolkit for players of the Ironsworn tabletop RPG.  It is intended for both solo and co-op player and allows to roll different dices such as action or oracle rolls.  It also provides results from the static oracle tables from the official rulebook.

Although there are several Ironsworn player toolkits available, there was none for the command line.  Since I prefer working in a terminal, I wrote isscrolls.  Think of it as the most Unix-like Ironsworn experience you'll ever see.  Besides that, you can play it over SSH or even in a shared terminal session (with tmux or screen).

![isscrolls screenshot](https://xosc.org/misc/is.png)

## Features

The following game mechanics are already implemented:

* All _Adventure moves_
* Automatic progress tracking for journey
* All _Combat moves_
* Automatic progress tracking for fights
* All _Quest moves_
* Most of the _Relationship moves_
* Support for various oracle tables such as names, locations, etc

## Installation

isscrolls is written in C and known to work on the operating systems listed in the table below.  To compile it you need the following things:

* A C compiler (tested with both clang and GCC)
* make (tested with both BSD and GNU make)
* [The GNU Readline library](https://tiswww.case.edu/php/chet/readline/rltop.html)
* [The JSON-C library](https://github.com/json-c/json-c)

### Dependencies

Install the dependencies as follows:

| Operating System | Commands and Notes |
| --- | --- |
| Arch Linux | Both depenencies should already be installed by default.  Otherwise, `pacman -Syu gcc make json-c readline` will install them |
| Fedora Linux | `dnf install readline-devel json-c-devel` |
| NetBSD | `pkgin install readline json-c` You also need to add  `-I/usr/pkg/include` to CFLAGS and `-L/usr/pkg/lib` to LDADD in the `Makefile` |
| OpenBSD | `pkg_add json-c` |
| Ubuntu Linux| `apt install libreadline-dev libjson-c-dev` |
| Void Linux| `xbps-install gcc make readline-devel json-c-devel` |

### Compilation and Installation

By default, the `Makefile` looks for external includes and libraries in `/usr/local/include` and `/usr/local/lib`.  If your distribution uses special path, you have to modify the Makefile accordingly.

Compile and install with the following commands:

```
$ make
# make install
```

## Usage

isscrolls presents the user with a command prompt and accepts various commands.  A built-in help can be seen by entering __help__ at isscrolls' command prompt.

**Example**

```
> action 3
D6: 3+3=6 D10: 5, 7 -> weak hit
> trollname
Slith (72)
> oracle
80
```

All commands including their usage patterns are described in the [man page](https://xosc.org/isscrolls.html).

## License

isscrolls was written by Matthias Schmidt and is licensed under the ISC license.  The Ironsworn material was written by [Shawn Tomkin](https://www.ironswornrpg.com) and is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International license.
