#!/bin/sh

############### EXPLICATIONS
# Commandes nécessaires :
#  - gcc pour compiler
#  - sed pour formater le retour de certaines commandes
#  - indent pour mettre rapidement en forme le code produit
#  - echo pour afficher du texte
#
# Ce que fait ce programme :
#   Il va tout simplement appliquer toutes les macros définies dans
#   detecter.c, retourner le résultat dans detecter.macroless.c et
#   dire s'il y a eu ou non des problèmes avec les macros
###############

# créer un nouveau fichier detecter.macroless.c, en récupérant les includes
# du fichier detecter.c
printf "$(sed '/^#include /!d' detecter.c)\n\n" > detecter.macroless.c

# ajoute au fichier créé précédemment le reste du code
# en appliquant les macros
gcc -E -nostdinc detecter.c 2> /dev/null \
| sed '/^# /d' \
| sed 'N;/^\n$/d;P;D' \
| indent -kr >> detecter.macroless.c \
&& gcc -Wall -Wextra -Werror detecter.macroless.c -o detecter.macroless \
&& echo "Aucune erreur détectée avec l'utilisation des macros !" \
|| echo "Une erreur est survenue avec l'utilisation des macros..."
