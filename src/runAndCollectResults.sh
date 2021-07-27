#!/bin/bash
set -e

RPTDIR=$1

mkdir $RPTDIR
./commCharaterize ./$RPTDIR/report

collectResults.sh $RPTDIR