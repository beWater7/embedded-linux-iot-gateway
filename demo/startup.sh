#!/bin/sh
### BEGIN INIT INFO
# Provides:          LDY-gatWay project
# Required-Start:
# Required-Stop:
# Default-Start:     
# Default-Stop:
# Short-Description: init
# Description:       init br0 network\gateWap app 
#                    
### END INIT INFO
#1.br0 config
ifconfig eth0 0.0.0.0
ifconfig eth1 0.0.0.0

brctl addbr br0
brctl addif br0 eth0
brctl addif br0 eth1

ifconfig br0 192.168.137.100 up

#2.app startup
/home/root/daemonTask &