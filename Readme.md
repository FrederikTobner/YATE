# YATE

[![Build](https://github.com/FrederikTobner/YATE/actions/workflows/build.yml/badge.svg)](https://github.com/FrederikTobner/YATE/actions/workflows/build.yml)
[![Analyze](https://github.com/FrederikTobner/YATE/actions/workflows/codeql.yml/badge.svg)](https://github.com/FrederikTobner/YATE/actions/workflows/codeql.yml)

YATE (Yet another text editor) is a extremely simple text editor for the terminal.
Runs on Linux, Windows (using Cygwin or the WSL), FreeBSD, macOS, and more.

Yate is based on [Kilo](https://github.com/antirez/kilo) - a small text editor that is written in less than 1K lines of code, by [antirez](https://github.com/antirez).

## Table of Contents

* [Overview](#overview)
* [Syntax Highlighting](#syntax-highlighting)
* [Settings](#settings)
* [Building](#building)
* [License](#license)

## Overview

[![asciicast](https://asciinema.org/a/546304.svg)](https://asciinema.org/a/546304)

Usage:

    yate <filename>

Hot-Keys:

    ctrl-d = Yanks and deletes the current line
    ctrl-f = Find occurences in file
    ctrl-h = Shows help
    ctrl-o = Opens file
    ctrl-p = Paste last yanked content
    ctrl-q = Exit the editor
    ctrl-s = Saves the currently opened file
    ctrl-x = Yank the current line
    ctrl-y = Yank the current line

## Syntax Highlighting

Rudimentary syntax highlighting for the following languages is provided:

* C
* C++
* Cellox
* Go
* Lua
* Python

## Settings

The settings of the editor can be configured with a file named '.yaterc', that has to be located at the users home directory.

The available settings are:

    STATUS_MESSAGE_DURATION = Duration in seconds for how long a status message will be displayed
    TAB_STOP_SIZE = Size of a tabstop converted to white space's

## Building

Dependecies:

* A C compiler that is compatible with the C99 standard
* [CMake](https://cmake.org/)

There is a prewritten scripts provided to build and install the editor in the scripts folder called 'install.sh'. The specified compiler and generator should probably be altered to fit your environment.

## License

This project is licensed under the [GNU General Public License](LICENSE)