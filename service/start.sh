#!/bin/bash -x
NETIF=wlx3c46d84fc55e
CHANNEL=3
cd $(dirname $0)
[ -f /tmp/keg ] && mv -f /tmp/keg .
ip link set $NETIF down
iw $NETIF set type monitor
ip link set $NETIF up
iw $NETIF set channel $CHANNEL
[ "$1" == "debug" ] && DEBUG="gdbserver 192.168.6.1:4000"
$DEBUG ./keg -n 8 -h 127.0.0.1 -p 6379 -d $NETIF
