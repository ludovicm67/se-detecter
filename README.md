# `detecter`

## Compiler et lancer le programme

Pour pouvoir éxécuter le Makefile, il faut avoir le programme `ctags`, qui
s'installe rapidement avec la commande `sudo apt install ctags` sur les
distributions Ubuntu.

En suite il suffira de lancer `make` pour compiler le programme.

Pour l'éxécuter, par exemple :
    ` ./detecter -c -i 10000 -l 4 -t "%H:%M:%S" ls -l /tmp`
