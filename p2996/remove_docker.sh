#!/usr/bin/env bash
tuser=$(echo $USER | tr -dc 'a-z')
container_name=${CONTAINER_NAME:-"debian12-clang-p2996-programming_station-for-$tuser-simdjson"}
echo $container_name
echo " Removing the container "
docker ps -a | awk '{ print $1,$2 }' | grep $container_name| awk '{print $1 }' | xargs -I {} docker stop -t 1 {} | xargs docker rm
# removing image
docker image rm $container_name
