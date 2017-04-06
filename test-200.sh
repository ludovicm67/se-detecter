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

./detecter -m cat $DN && fail "option -m inconnue"

exit 0
