#!/bin/sh

#
# Test personnalisés
#

TEST=$(basename $0 .sh)

TMP=/tmp/$TEST
LOG=$TEST.log
V=${VALGRIND}       # appeler avec la var. VALGRIND à "" ou "valgrind -q"

exec 2> $LOG
set -x

rm -f *.tmp

fail ()
{
    echo "==> Échec du test '$TEST' sur '$1'."
    echo "==> Log : '$LOG'."
    echo "==> Exit"
    exit 1
}

DN=/dev/null
echo titi > toto.tmp

$V ./detecter -a cat $DN && fail "option -a inconnue"
$V ./detecter -t "" cat $DN && fail "format pour le temps vide !"
$V ./detecter commandeInconnue && fail "commande inconnue"
$V ./detecter -i1 -l5 cat toto.tmp > tata.tmp \
    && cmp tata.tmp toto.tmp \
    || fail "mauvaise sortie : le programme retourne trop de fois la sortie ?"

rm *.tmp

exit 0
