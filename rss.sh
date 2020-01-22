#!/bin/sh

./waf --run "scratch/tcpmcpsim --MaxBytes=500000 --QueueLength=2000p --HostRate=10000 --SimTime=600 --Static=true" >log_a_a_a_a_a 2>&1
