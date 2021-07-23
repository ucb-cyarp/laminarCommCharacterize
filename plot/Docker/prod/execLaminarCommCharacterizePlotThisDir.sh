#!/bin/bash

# Runs a command in a container with Latex installed.  Once complete, the container is distroyed
# This command will mount the current directory to /working in the container.  All paths should be relative to that base

#with help from https://www.digitalocean.com/community/tutorials/how-to-remove-docker-images-containers-and-volumes#:~:text=Remove%20a%20container%20upon%20exit,docker%20run%20%2D%2Drm%20image_name
# https://docs.docker.com/engine/reference/run/
# https://docs.docker.com/engine/reference/builder/
# https://docs.docker.com/storage/bind-mounts/
# https://stackoverflow.com/questions/40905761/how-do-i-mount-a-host-directory-as-a-volume-in-docker-compose
# https://forums.docker.com/t/run-command-in-stopped-container/343/14
# https://askubuntu.com/questions/294736/run-a-shell-script-as-another-user-that-has-no-password
# https://stackoverflow.com/questions/9057387/process-all-arguments-except-the-first-one-in-a-bash-script
# https://askubuntu.com/questions/393463/create-the-home-directory-while-creating-a-user
#
# and Paul Rigge

#Check if the docker image exists

#Get the working directory absolute path
srcDir=$PWD

#Get the home directory of the current user
homeDir=$HOME

#Get the username of the current user.  We will create this user and su into it
user=$(whoami)

#Get the actual command to run
cmd="${@:1}"

echo "docker run --rm --name LaminarCommCharacterizePlotExec -a stdin -a stdout -it -v \"$srcDir\":/working: laminar_comm_characterize_plot:1.0 bash -c \"useradd -d $homeDir $user; sudo -H -u $user bash -c \"cd /working; python3 /srv/plot/src/PlotLaminarChar.py --input-dir . --output-file-prefix ./rpt ${cmd}; python3 /srv/plot/src/PlotLaminarChar.py --input-dir . --output-file-prefix ./rpt_lbl --avg-lines ${cmd}\"\""

docker run --rm --name LaminarCommCharacterizePlotExec -a stdin -a stdout -it -v "$srcDir":/working: laminar_comm_characterize_plot:1.0 bash -c "useradd -d $homeDir $user; sudo -H -u $user bash -c \"cd /working; python3 /srv/plot/src/PlotLaminarChar.py --input-dir . --output-file-prefix ./rpt ${cmd}; python3 /srv/plot/src/PlotLaminarChar.py --input-dir . --output-file-prefix ./rpt_lbl --avg-lines ${cmd}\""