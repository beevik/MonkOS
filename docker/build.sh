#!/bin/bash

args=$@

uid=$(id -u)
gid=$(id -g)
code=$(realpath .)

docker run --rm -it \
    -v ${code}:/code \
    -u ${uid}:${gid} \
    brett/monkos-build /bin/bash -c "cd /code && make ${args}"
