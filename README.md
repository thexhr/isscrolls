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

isscrolls is written in C and tested on OpenBSD, and Linux.  To compile it you need the following things:

* A C compiler (tested with both clang and GCC)
* make (tested with both BSD and GNU make)
* [The GNU Readline library](https://tiswww.case.edu/php/chet/readline/rltop.html)
* [JSON-C](https://github.com/json-c/json-c)

### Dependencies

On most Unix systems, __readline__ is installed by default.  Otherwise, __readline__ and __json-c__ can be installed from the package manager of your distribution or compiled from source.  By default, the `Makefile` looks for external includes and libraries in `/usr/local/include` and `/usr/local/lib`.  If you use a special path, modify the Makefile accordingly.

Install the dependencies as follows:

| Operating System | Command |
| --- | --- |
| Ubuntu Linux| `apt install libreadline-dev libjson-c-dev` |
| OpenBSD | `pkg_add json-c` |

### Compilation and Installation

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
