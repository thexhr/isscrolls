# isscrolls - Player toolkit for the Ironsworn family tabletop RPG

isscrolls is a toolkit for players of the [Ironsworn](https://www.ironswornrpg.com/) tabletop RPG.  It supports classic Ironsworn, Delve, **Starforged** and its extension **Sundered Isles**.

It is intended for both solo and co-op play and allows to track your character's progress including vows, roll on adventure and exploration moves, tracks progress for fights, journeys and expeditions and provides results from the static oracle tables from the official rulebook.

Although there are several Ironsworn player toolkits available, there was none for the command line.  Since I prefer working in a terminal, I wrote isscrolls.  Think of it as the most Unix-like Ironsworn experience you'll ever see.  Besides that, you can play it over SSH or even in a shared terminal session (with tmux or screen).

Visually impaired players should run isscrolls with the `-b` option to suppress the banner on startup.  By default, very little extra characters or symbols are shown to ease playing with a screen reader or a braille display.  In case you want colors, bold fonts and more symbols start isscrolls with the `-c` option.  This can be seen in the screenshot below.

![isscrolls screenshot](https://xosc.org/misc/is-202209.png)

## Features

The following game mechanics are implemented.  Some moves are omitted on purpose since it makes little sense to implement them in software, they have to be played by the player.

* Tracking of vows
* Adventure and Exploration moves
* Automatic progress tracking for journeys and expeditions
* Combat moves
* Automatic progress tracking for fights
* Quest moves
* Relationship moves
* Delve moves
* Support for various oracle tables such as names, locations, etc

## Installation

isscrolls is written in C and known to work on the operating systems listed in the table below.  To compile it you need the following things:

* A recent C compiler (tested with both clang >= 11 and GCC >= 8)
* make (tested with both BSD and GNU make)
* [The GNU Readline library](https://tiswww.case.edu/php/chet/readline/rltop.html)
* [The JSON-C library](https://github.com/json-c/json-c) >= Version 13

### Dependencies

Install the dependencies as follows:

| Operating System | Commands and Notes |
| --- | --- |
| Arch Linux | Both dependencies should already be installed by default.  Otherwise, `pacman -Syu gcc make json-c readline` will install them |
| Debian Linux| `apt install libreadline-dev libjson-c-dev` |
| DragonFly BSD | `pkg install json-c` |
| Fedora Linux | `dnf install readline-devel json-c-devel` |
| FreeBSD | `pkg install readline json-c` |
| macOS | `brew install json-c` |
| NetBSD | `pkgin install readline json-c` |
| OpenBSD | `pkg_add json-c` |
| Ubuntu Linux| `apt install libreadline-dev libjson-c-dev` |
| Void Linux| `xbps-install gcc make readline-devel json-c-devel` |
| Windows | There is no native version, just use WSL |

If your operating system does not have `pkg-config` installed by default, you have to install it as well.

### Compilation and Installation

By default, the `Makefile` looks for external includes and libraries in `/usr/local/include` and `/usr/local/lib`.  If your distribution uses special path, you have to modify the Makefile accordingly.

Compile and install with the following commands:

```
$ make
# make install
```

## Usage

isscrolls presents the user with a command prompt and accepts various commands.  A built-in help can be seen by entering __help__ at isscrolls' command prompt.
All commands including their usage patterns are described in the [man page](https://xosc.org/isscrolls.html).

**Examples**

Create a new character:

```
> create
Enter a name for your character: Jorrun
Now distribute the following values to your attributes:
 - Challenging difficulty      : 4,3,3,2,2
 - Perilous difficulty         : 3,2,2,1,1
 - Grim difficulty             : 3,2,1,1,0
Edge   : 3
Heart  : 2
Iron   : 2
Wits   : 1
Shadow : 1
Name: Jorrun (Exp: 0/30) Exp spent: 0

Edge: 3 Heart: 2 Iron: 2 Shadow: 1 Wits: 1

Momentum: 2/10 [2] Health: 5/5 Spirit: 5/5 Supply: 5/5

Wounded:        0 Unprepared:   0 Encumbered:   0 Shaken:       0
Corrupted:      0 Tormented:    0 Cursed:       0 Maimed:       0

Armed with a simple weapon

Bonds: 0.00
```
Now engage in a fight with the new character:
```
Jorrun > enterthefray wits
Please set a rank for your fight

1        - Troublesome foe (3 progress per harm)
2        - Dangerous foe (2 progress per harm)
3        - Formidable foe (2 progress per harm)
4        - Extreme foe (2 ticks per harm)
5        - Epic foe (1 tick per harm)

Enter a value between 1 and 5: 2
<1> + 1 = 2 vs <6><3> -> miss
Pay the price -> Rulebook
```
Since the last roll was a _strong hit_ you can see how the progress is advanced according to the foe's rank and how the prompt changes to show that _Jorrun_ now has initiative.

```
Jorrun > Fight 0 > clash edge
<1> + 3 = 4 vs <2><1> -> strong hit
You inflict harm, regain initiative and can choose one option -> Rulebook
Jorrun > Fight 2 [I] >
```

## FAQ

**Why does isscrolls so often refers to the official rulebook?** The program should help you in keeping track of your character's progress.  However, it cannot replace the GM in your campaign.  In order to give the player as much freedom as possible, I refrain from doing everything automatically and instead redirect you to the rulebook.

## License

isscrolls was written by Matthias Schmidt and is licensed under the ISC license.  The Ironsworn material was written by [Shawn Tomkin](https://www.ironswornrpg.com) and is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International license. The [Ironsworn Starforged Playkit](https://www.ironswornrpg.com/downloads) was written by Shawn Tomkin in 2022 and is licensed under the Creative Commons Attribution 4.0 International License.  No changes to the content were made.
