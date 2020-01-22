#!/bin/bash
c=41
sd=50000
hdl="5us"
dljump=
bwjump=1
b=5500000000
declare -a bwarr=(64 90 128 181 256 362 512 724 1024 1448 2048 2896 4096 5792 8192 11585 16384 23170 32768 46340 65536 92681 131072 185363 262144 370727 524288 741455 1048576)
declare -a flarr=(100 200 400 800)
declare -a dlarr=(50000 100000 200000 500000 1000000 2000000)
declare -a qlarr=(100 200 400 800)
#declare -a bwarr=(1024)
#declare -a flarr=(10)
#declare -a dlarr=(1000000)
#declare -a qlarr=(800)
for j in "${flarr[@]}"; do
  for i in "${bwarr[@]}"; do
    for q in "${qlarr[@]}"; do
      for d in "${dlarr[@]}"; do
        n=0
        while [ $(ps u | grep "/home/wew168/ns1/ns-3.29/" | wc -l) -ge $c ]; do
          sleep 5
        done
        bh=$b
        bl=$((b/bwjump))
        nohup ./waf --run "scratch/http --HostPropDelay=$hdl --HostRate=40000000000 --Nsd=$sd --QueueLength=$q --RWND=20000000 --NFlows=$j --Rjitter=10000 --BWP=${bh},${d}ns,$i,1,${bl},$((d*5))ns,$i,1" > /dev/null &
        sleep 10
      done
    done
  done
done
