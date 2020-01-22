#!/bin/bash
c=21
sd=50000
swdl="20us"
#declare -a bwarr=(64 90 128 181 256 362 512 724 1024 1448 2048 2896 4096 5792 8192 11585 16384 23170 32768 46340 65536 92681 131072 185363 262144 370727 524288 741455 1048576)
#declare -a flarr=(1 5 10 15 20)
#declare -a dlarr=("40ns" "5us" "10us" "50us" "100us" "500us" "1000us")
#declare -a qlarr=("100p" "200p" "400p" "800p")
declare -a bwarr=(10 200 1000)
declare -a flarr=(1 5 10)
declare -a dlarr=("40ns")
declare -a qlarr=("800p")
for j in "${flarr[@]}"; do
  for i in "${bwarr[@]}"; do
    for q in "${qlarr[@]}"; do
      for d in "${dlarr[@]}"; do
        n=0
        while [ $(ps u | grep "/home/wew168/ns1" | wc -l) -ge $c ]; do
          sleep 5
        done
        nohup ./waf --run "scratch/tcplargetrans --HostPropDelay=$d --Bidir=True --TorPropDelay=$swdl --HostRate=40000000000 --MaxBytes=4000000000 --Nsd=$sd --QueueLength=$q --RWND=20000000 --NFlows=$j --Rjitter=10000 --BWP=5000000000,$i,1000000000,$i" >/dev/null &
        sleep 10
      done
    done
  done
done
