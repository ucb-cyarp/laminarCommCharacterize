#!/bin/bash
set -e

RPTDIR=$1

mkdir $RPTDIR/src
cp *.c $RPTDIR/src/.
cp *.h $RPTDIR/src/.
cp *.log $RPTDIR/src/.
rsync --exclude=secretkey.sh *.sh $RPTDIR/src/.
cp *.py $RPTDIR/src/.
cp Makefile* $RPTDIR/src/.
cp commCharaterize $RPTDIR/src/.