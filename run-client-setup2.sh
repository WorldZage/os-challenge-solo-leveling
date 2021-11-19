#!/bin/sh
#
# This is the configuration of setup2.

SERVER=192.168.101.10
PORT=5003
SEED=0
TOTAL=100
START=0
DIFFICULTY=3000000
REP_PROB_PERCENT=40
DELAY_US=100000
PRIO_LAMBDA=6

/home/vagrant/os-challenge-common/client $SERVER $PORT $SEED $TOTAL $START $DIFFICULTY $REP_PROB_PERCENT $DELAY_US $PRIO_LAMBDA
