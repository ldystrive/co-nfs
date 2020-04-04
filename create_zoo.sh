#! /bin/sh

rm -rf ~/zookeeper-cluster/
mkdir -p ~/zookeeper-cluster/

node_num=3
for((i = 1; i <= $node_num; i++))
do
  mkdir -p ~/zookeeper-cluster/node$i
  docker run -d -p 2181:2181 --name zookeeper_node$i --privileged --restart always --network zoonet --ip 172.18.0. \
	  -v ~/zookeeper-cluster/node$i/volumes/data:/data \
	  -v ~/zookeeper-cluster/node$i/volumes/datalog:/datalog \
	  -v ~/zookeeper-cluster/node$i/volumes/logs:/logs \
	  -e ZOO_MY_ID=1 \
	  -e "ZOO_SERVERS=server.1=172.18.0.2:2888:3888:2181 server.2"
	  
done


