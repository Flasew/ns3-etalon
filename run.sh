#!/bin/bash
c=41
sd=50000
hdl="10ns"
dljump=6
bwjump=8
b=80000000000
declare -a bwarr=(8 11 16 22 32 45 64 90 128 181 256 362 512 724 1024 1448 2048 2896 4096 5792 8192 11585 16384 23170 32768 46340 65536 92681 131072 185363 262144 370727 524288 741455 1048576)
declare -a flarr=(1 5 10 15)
declare -a dlarr=(30000)
declare -a qlarr=(4 8 16 32 64 128)
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
        dh=$d
        dl=$((d/dljump))
        dack=3
        ./waf --run "scratch/tcpltdelay --HostPropDelay=$hdl --Bidir=false --HostRate=100000000000 --MaxBytes=25000000000 --Nsd=$sd --QueueLength=$q --RWND=20000000 --NFlows=$j --DupAckTh=${dack} --BWP=${bh},${dh}ns,$i,${bl},${dl}ns,$i" &
        sleep 10
      done
    done
  done
done
