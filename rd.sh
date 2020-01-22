#!/bin/bash
c=0
sd=50000
#declare -a bwarr=(51 64 81 102 128 161 203 255 321 405 509 641 807 1016 1279 1611 2028 2553 3214 4046 5093 6412 8072 10162 12794 16106 20277 25527 32137 40458 50933 64121 80724 101625 127938 161065 202768 255270 321366 404576 509331 641210 807235 1016249 1279381 1610646)
declare -a bwarr=(64 102 161 255 405 641 1016 1611 2553 4046 6412 10162 16106 25527 40458 64121 101625)
declare -a flarr=(2)
declare -a dlarr=("3000us")
declare -a qlarr=("60p" "100p" "140p")
for j in "${flarr[@]}"; do
  for i in "${bwarr[@]}"; do
    for q in "${qlarr[@]}"; do
      for d in "${dlarr[@]}"; do
        n=0
        while [ $n -lt 3 ]; do
          if [ $c -lt 40 ]; then
            c=$((c+1))
            nohup ./waf --run "scratch/tcpbidir --PropDelay=$d --MaxBytes=10000000000 --Nsd=$sd --QueueLength=$q --RWND=524288 --IndivLog=true --NFlows=$j --BWP=10000000000,$i,1000000000,$i" >/dev/null &
            sleep 30
          else
            sleep 450
            c=0
          fi
          n=$((n+1))
        done
      done
    done
  done
done
