# Eddy v2

This is the second iteration of my text editor. Differences with v1 are that 
it's implemented in `C` (instead of `C++`) using the 
[raylib](https://github.com/raysan5/raylib) library (instead
of SDL).

### Why?

See [![justforfunnoreally.dev badge](https://img.shields.io/badge/justforfunnoreally-dev-9ff)](https://justforfunnoreally.dev)

## Goals

I'm from the `emacs` school (as opposed to the `vi` school), and while `vi` has
been reiterated a number of times over the years- first `vim` in the 90s, and 
now `neovim`, `emacs` is still a continuation of the original Stallman project.

I think the emacs way of thinking needs to be brought into the twentyfirst 
century. I'm not aiming at copying `emacs` functionality, but the whole
"everything is a macro" ethos appeals to me, and also the insert/command mode
separation that `vi` has doesn't work for me (and yes, I tried. I probably wrote
more code in `vi` than you ever have before you were even born).


## Installation

```shell
$ cd ~/projects
$ git clone git@github.com:JanDeVisser/eddy_v2.git
$ cd eddy_v2
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
$ cmake --install .
$ bin/eddy src/main.c 
```
