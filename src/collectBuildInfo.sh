#!/bin/bash
set -e

CC=$1

echo "Timestamp: $(date)" > info.log
echo "Hostname: $(hostname)" >> info.log
echo "uname: $(uname -a)" >> info.log
echo "lsb_release:" >> info.log
lsb_release -a 1>> info.log 2>>info.log
echo "GIT STATUS:" > git.log
git status >> git.log
echo "GIT LOG:" >> git.log
git log >> git.log
git diff > gitdiff.log
which ${CC} > compilerVersion.log
${CC} --version >> compilerVersion.log
