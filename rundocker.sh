#!/usr/bin/env bash

set -e

if [ "$#" -eq 0 ] ; then
    COMMAND="bash"
else
    COMMAND="bash -c \"$*\""
fi

docker run \
       --interactive \
       --tty \
       --rm \
       --user $(id -u):$(id -g) \
       --workdir=$PWD \
       --net host \
       --volume="/home/$USER:/home/$USER" \
       --volume="/etc/group:/etc/group:ro" \
       --volume="/etc/passwd:/etc/passwd:ro" \
       --volume="/etc/shadow:/etc/shadow:ro" \
       eerimoq/monolinux-example-project:0.6 bash -c "source setup.sh && $COMMAND"
