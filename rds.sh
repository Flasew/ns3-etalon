#!/bin/bash
./waf --run "scratch/tcpbidir --MaxBytes=10000000000 --Nsd=0 --QueueLength=60p --RWND=524288 --IndivLog=true --Static=true --NFlows=1 --BWP=5500000000,0"
