#! /bin/sh

docker network inspect zoonet >/dev/null 2>&1
if [ $? -eq 0 ] 
then
  echo "zoonet already exists."
else
  echo "creating docker network for zookeeper."
  zoonetRst=`docker network create --driver bridge --subnet=172.18.0.0/16 --gateway=172.18.0.1 zoonet`
  echo "zoonet id: $zoonetRst"
fi

