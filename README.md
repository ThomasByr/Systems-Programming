# Systems programming

> [crash course](https://moodle.unistra.fr/course/view.php?id=1588) by Pierre David

1. [What this is](#what-this-is)
2. [Table of contents](#table-of-contents)
   1. [gestion de fichiers](#gestion-de-fichiers)
   2. [gestion des processus](#gestion-des-processus)
   3. [gestion des signaux](#gestion-des-signaux)
   4. [tubes](#tubes)
   5. [TP 1](#tp-1)
   6. [TP 2](#tp-2)
   7. [TP 3](#tp-3)
   8. [sujet 2021](#sujet-2021)
   9. [sujet 2020](#sujet-2020)
3. [Contributing](#contributing)
4. [Changelog and TODO](#changelog-and-todo)

## What this is

This repo contains some elements of correction for diverse exercises and practicum (work placement). This **is not** some certified correction, neither does it provide one for all exercises. Tests are guaranteed to pass though. Some PDF are available.

## Table of contents

### gestion de fichiers

*   `copy.c` : 5.4
*   `getchar.c` : 5.5
*   `getchar_b.c` : 5.6 (same as previous but with buffer)
*   `listing.c` : 5.8
*   `dir.c` : 5.10
*   `seek.c` : 5.13 (bonus)
*   `which.c` : 5.14

### gestion des processus

*   `simple_fork.c` : 6.1
*   `multiple_fork.c` : 6.2
*   `open_fork.` : 6.4
*   `maxval.c` : last year's tp2 (currently unrevised)
*   `gettime.c` : 6.6
*   `rand.c` : 6.8

### gestion des signaux

*  `count_signint.c` : 9.1
*  `display_signal.c` : 9.3

### tubes

*  `copy.c` : 8.1
*  `ps_eaux.c` : 8.4

### TP 1

`compare.c` (similar behavior as `cmp` bash command)

### TP 2

`ordonnanceur.c` (dummy task scheduler for monothreaded cpu)

### TP 3

`reseau.c` (ethernet commuting simulation)

### sujet 2021

`xargexec.c` multi instance exec with optional arguments

### sujet 2020

`roulette.c` game

## Contributing

If you want some correction, simply submit a pull request with the name of the exercise as shown above. If you want to submit one, please be sure it was first seen in class and that the official submit deadline is passed. Also try to be consistent in the coding style.

## Changelog and TODO

1.  added some exercises and doc
2.  excluded archives and tar files

really should automate the upload and test case check...
