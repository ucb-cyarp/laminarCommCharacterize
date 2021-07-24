#!/bin/bash
set -e

reportDir=$1

mkdir $RPTDIR/src
cp *.c $RPTDIR/src/.
cp *.h $RPTDIR/src/.
cp *.log $RPTDIR/src/.
cp Makefile* $RPTDIR/src/.
cp commCharaterize $RPTDIR/src/.