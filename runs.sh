#!/bin/bash
./waf --run "scratch/tcpltranhop --MaxBytes=10000000 --Nsd=50000 --Bidir=false --QueueLength=1000 --RWND=10000000 --NFlows=1 --HostPropDelay=5us --BWP=10000000000,50us,64,1,1000000000,100us,64,5"
